/**
 * @file list_cmd.cpp
 * @brief Entity/relation table rendering and trace helpers (C++ edition).
 */

#include "list_cmd.h"
#include <cstdio>
#include <cstring>
#include "entity.h"
#include "triplet_store_c.h"
#include "yaml_simple.h"

#define SUBJ_MAX_DISPLAY 32
#define OBJ_MAX_DISPLAY  48
#define ENTITY_TITLE_MAX  48

static void print_rel_rule(int subj_w, int pred_w, int obj_w, int src_w)
{
    int widths[] = { subj_w, pred_w, obj_w, src_w };
    int ncols    = (int)(sizeof(widths) / sizeof(widths[0]));
    for (int c = 0; c < ncols; c++) {
        putchar('+');
        for (int i = 0; i < widths[c] + 2; i++)
            putchar('-');
    }
    puts("+");
}

void list_relations(const TripletStore *store)
{
    CTripleList all = triplet_store_find_all(store);

    if (all.count == 0) {
        puts("No relations found.");
        triplet_store_list_free(all);
        return;
    }

    int subj_w = (int)strlen("Subject");
    int pred_w = (int)strlen("Relation");
    int obj_w  = (int)strlen("Object");
    int src_w  = (int)strlen("Source");

    for (size_t i = 0; i < all.count; i++) {
        int len;
        len = (int)strlen(all.triples[i].subject);
        if (len > subj_w) subj_w = len;
        len = (int)strlen(all.triples[i].predicate);
        if (len > pred_w) pred_w = len;
        len = (int)strlen(all.triples[i].object);
        if (len > obj_w)  obj_w  = len;
    }

    if (subj_w > SUBJ_MAX_DISPLAY) subj_w = SUBJ_MAX_DISPLAY;
    if (obj_w  > OBJ_MAX_DISPLAY)  obj_w  = OBJ_MAX_DISPLAY;

    print_rel_rule(subj_w, pred_w, obj_w, src_w);
    printf("| %-*s | %-*s | %-*s | %-*s |\n",
           subj_w, "Subject", pred_w, "Relation", obj_w, "Object",
           src_w, "Source");
    print_rel_rule(subj_w, pred_w, obj_w, src_w);

    for (size_t i = 0; i < all.count; i++) {
        const CTriple *t = &all.triples[i];

        char sbuf[SUBJ_MAX_DISPLAY + 1];
        strncpy(sbuf, t->subject, sizeof(sbuf) - 1);
        sbuf[sizeof(sbuf) - 1] = '\0';
        if (subj_w >= 3 && (int)strlen(t->subject) > subj_w) {
            sbuf[subj_w - 3] = sbuf[subj_w - 2] = sbuf[subj_w - 1] = '.';
            sbuf[subj_w] = '\0';
        }

        char obuf[OBJ_MAX_DISPLAY + 1];
        strncpy(obuf, t->object, sizeof(obuf) - 1);
        obuf[sizeof(obuf) - 1] = '\0';
        if (obj_w >= 3 && (int)strlen(t->object) > obj_w) {
            obuf[obj_w - 3] = obuf[obj_w - 2] = obuf[obj_w - 1] = '.';
            obuf[obj_w] = '\0';
        }

        const char *src = t->inferred ? "inferred" : "declared";
        printf("| %-*s | %-*s | %-*s | %-*s |\n",
               subj_w, sbuf, pred_w, t->predicate, obj_w, obuf,
               src_w, src);
    }

    print_rel_rule(subj_w, pred_w, obj_w, src_w);
    printf("\nTotal: %zu relation(s)\n", all.count);
    triplet_store_list_free(all);
}

int check_strict_links(const TripletStore *store)
{
    CTripleList all = triplet_store_find_all(store);
    int warnings = 0;

    for (size_t i = 0; i < all.count; i++) {
        const CTriple *t = &all.triples[i];
        if (t->inferred) continue;

        CTripleList by_subj = triplet_store_find_by_subject(store, t->object);

        char inv_pred_buf[256] = {0};
        int  found_declared    = 0;

        for (size_t j = 0; j < by_subj.count; j++) {
            const CTriple *cand = &by_subj.triples[j];
            if (cand->inferred && strcmp(cand->object, t->subject) == 0) {
                strncpy(inv_pred_buf, cand->predicate,
                        sizeof(inv_pred_buf) - 1);
                break;
            }
        }

        if (inv_pred_buf[0] == '\0') {
            triplet_store_list_free(by_subj);
            continue;
        }

        for (size_t j = 0; j < by_subj.count; j++) {
            const CTriple *cand = &by_subj.triples[j];
            if (!cand->inferred &&
                strcmp(cand->predicate, inv_pred_buf) == 0 &&
                strcmp(cand->object, t->subject) == 0) {
                found_declared = 1;
                break;
            }
        }
        triplet_store_list_free(by_subj);

        if (!found_declared) {
            fprintf(stderr,
                "warning: strict-links: '%s -[%s]-> %s' — "
                "inverse '[%s]' not explicitly declared by '%s'\n",
                t->subject, t->predicate, t->object,
                inv_pred_buf, t->object);
            warnings++;
        }
    }

    triplet_store_list_free(all);
    return warnings;
}

static void trace_subject(const TripletStore *store, const char *subject,
                           int depth, int max_depth)
{
    CTripleList list = triplet_store_find_by_subject(store, subject);

    for (size_t i = 0; i < list.count; i++) {
        const CTriple *t = &list.triples[i];
        if (t->inferred)
            continue;

        for (int d = 0; d < depth; d++)
            printf("  ");
        printf("-[%s]-> %s\n", t->predicate, t->object);

        if (depth < max_depth)
            trace_subject(store, t->object, depth + 1, max_depth);
    }

    triplet_store_list_free(list);
}

void cmd_trace_entity(const EntityList *elist,
                      const TripletStore *store, const char *id)
{
    const Entity *found = NULL;
    for (const auto &e : *elist) {
        if (e.identity.id == id) {
            found = &e;
            break;
        }
    }

    printf("Traceability chain for: %s\n", id);

    if (found) {
        printf("  Kind:   %s\n", entity_kind_label(found->identity.kind));
        if (!found->identity.title.empty())
            printf("  Title:  %s\n", found->identity.title.c_str());
        if (!found->lifecycle.status.empty())
            printf("  Status: %s\n", found->lifecycle.status.c_str());
    } else {
        printf("  (entity not found in scanned files)\n");
    }

    printf("\nOutgoing links:\n");
    CTripleList out_links = triplet_store_find_by_subject(store, id);
    int out_count = 0;
    for (size_t i = 0; i < out_links.count; i++) {
        if (!out_links.triples[i].inferred)
            out_count++;
    }
    triplet_store_list_free(out_links);
    if (out_count == 0) {
        printf("  (none)\n");
    } else {
        trace_subject(store, id, 1, 2);
    }

    printf("\nIncoming links:\n");
    CTripleList in_links = triplet_store_find_by_object(store, id);
    int in_count = 0;
    for (size_t i = 0; i < in_links.count; i++) {
        const CTriple *t = &in_links.triples[i];
        if (t->inferred)
            continue;
        printf("  %s -[%s]->\n", t->subject, t->predicate);
        in_count++;
    }
    if (in_count == 0)
        printf("  (none)\n");
    triplet_store_list_free(in_links);
}

TripletStore *build_entity_relation_store(const EntityList *list)
{
    TripletStore *store = triplet_store_create();
    if (!store)
        return NULL;

    for (const auto &e : *list) {
        entity_traceability_to_triplets(&e, store);
        entity_doc_membership_to_triplets(&e, store);
    }

    triplet_store_infer_inverses(store);
    return store;
}

static const Entity *find_entity_by_id(const EntityList *list, const char *id)
{
    for (const auto &e : *list) {
        if (e.identity.id == id)
            return &e;
    }
    return NULL;
}

static int list_contains_entity_id(const EntityList *list, const char *id)
{
    return find_entity_by_id(list, id) != NULL;
}

static int entity_is_member_of_document(const TripletStore *store,
                                        const char *entity_id,
                                        const char *doc_id)
{
    CTripleList by_subject = triplet_store_find_by_subject(store, entity_id);
    int is_member = 0;

    for (size_t i = 0; i < by_subject.count; i++) {
        const CTriple *triple = &by_subject.triples[i];
        if (strcmp(triple->predicate, "part-of") == 0 &&
            strcmp(triple->object, doc_id) == 0) {
            is_member = 1;
            break;
        }
    }

    triplet_store_list_free(by_subject);
    return is_member;
}

int collect_document_entities(const EntityList *all, const TripletStore *store,
                               const char *doc_id, EntityList *out)
{
    const Entity *doc = find_entity_by_id(all, doc_id);
    if (!doc)
        return -1;
    if (doc->identity.kind != ENTITY_KIND_DOCUMENT)
        return -2;

    out->push_back(*doc);

    for (const auto &entity : *all) {
        if (entity.identity.id == doc_id)
            continue;

        if (!entity_is_member_of_document(store, entity.identity.id.c_str(),
                                           doc_id))
            continue;

        if (list_contains_entity_id(out, entity.identity.id.c_str()))
            continue;

        out->push_back(entity);
    }

    return 0;
}

static void print_entity_rule(int id_w, int title_w, int kind_w,
                               int status_w, int prio_w)
{
    int widths[] = { id_w, title_w, kind_w, status_w, prio_w };
    int ncols    = (int)(sizeof(widths) / sizeof(widths[0]));
    for (int c = 0; c < ncols; c++) {
        putchar('+');
        for (int i = 0; i < widths[c] + 2; i++)
            putchar('-');
    }
    puts("+");
}

static void print_entity_row(int id_w, int title_w, int kind_w,
                              int status_w, int prio_w,
                              const char *id, const char *title,
                              const char *kind, const char *status,
                              const char *prio)
{
    char tbuf[ENTITY_TITLE_MAX + 1];
    strncpy(tbuf, title, sizeof(tbuf) - 1);
    tbuf[sizeof(tbuf) - 1] = '\0';
    if ((int)strlen(title) > title_w) {
        tbuf[title_w - 3] = '.';
        tbuf[title_w - 2] = '.';
        tbuf[title_w - 1] = '.';
        tbuf[title_w]     = '\0';
    }

    printf("| %-*s | %-*s | %-*s | %-*s | %-*s |\n",
           id_w,     id,
           title_w,  tbuf,
           kind_w,   kind,
           status_w, status,
           prio_w,   prio);
}

void list_entities(const EntityList *list)
{
    if (list->empty()) {
        puts("No entities found.");
        return;
    }

    int id_w     = (int)strlen("ID");
    int title_w  = (int)strlen("Title");
    int kind_w   = (int)strlen("Kind");
    int status_w = (int)strlen("Status");
    int prio_w   = (int)strlen("Priority");

    for (const auto &e : *list) {
        int len;
        len = (int)e.identity.id.size();                         if (len > id_w)     id_w     = len;
        len = (int)e.identity.title.size();                      if (len > title_w)  title_w  = len;
        len = (int)strlen(entity_kind_label(e.identity.kind));   if (len > kind_w)   kind_w   = len;
        len = (int)e.lifecycle.status.size();                    if (len > status_w) status_w = len;
        len = (int)e.lifecycle.priority.size();                  if (len > prio_w)   prio_w   = len;
    }

    if (title_w > ENTITY_TITLE_MAX) title_w = ENTITY_TITLE_MAX;

    print_entity_rule(id_w, title_w, kind_w, status_w, prio_w);
    print_entity_row(id_w, title_w, kind_w, status_w, prio_w,
                     "ID", "Title", "Kind", "Status", "Priority");
    print_entity_rule(id_w, title_w, kind_w, status_w, prio_w);

    for (const auto &e : *list) {
        print_entity_row(id_w, title_w, kind_w, status_w, prio_w,
                         e.identity.id.c_str(),
                         e.identity.title.c_str(),
                         entity_kind_label(e.identity.kind),
                         e.lifecycle.status.c_str(),
                         e.lifecycle.priority.c_str());
    }

    print_entity_rule(id_w, title_w, kind_w, status_w, prio_w);
    int count = (int)list->size();
    printf("\nTotal: %d %s\n", count, count == 1 ? "entity" : "entities");
}
