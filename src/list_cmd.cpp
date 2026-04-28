/**
 * @file list_cmd.cpp
 * @brief Entity/relation table rendering and trace helpers (C++ edition).
 */

#include "list_cmd.h"
#include "yaml_simple.h"
#include <cstdio>
#include <cstring>
#include "entity.h"

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

void list_relations(const vibe::TripletStore *store)
{
    auto all = store->find_all();

    if (all.empty()) {
        puts("No relations found.");
        return;
    }

    int subj_w = (int)strlen("Subject");
    int pred_w = (int)strlen("Relation");
    int obj_w  = (int)strlen("Object");
    int src_w  = (int)strlen("Source");

    for (const auto *t : all) {
        int len;
        len = (int)t->subject.size();   if (len > subj_w) subj_w = len;
        len = (int)t->predicate.size(); if (len > pred_w) pred_w = len;
        len = (int)t->object.size();    if (len > obj_w)  obj_w  = len;
    }

    if (subj_w > SUBJ_MAX_DISPLAY) subj_w = SUBJ_MAX_DISPLAY;
    if (obj_w  > OBJ_MAX_DISPLAY)  obj_w  = OBJ_MAX_DISPLAY;

    print_rel_rule(subj_w, pred_w, obj_w, src_w);
    printf("| %-*s | %-*s | %-*s | %-*s |\n",
           subj_w, "Subject", pred_w, "Relation", obj_w, "Object",
           src_w, "Source");
    print_rel_rule(subj_w, pred_w, obj_w, src_w);

    for (const auto *t : all) {
        char sbuf[SUBJ_MAX_DISPLAY + 1];
        strncpy(sbuf, t->subject.c_str(), sizeof(sbuf) - 1);
        sbuf[sizeof(sbuf) - 1] = '\0';
        if (subj_w >= 3 && (int)t->subject.size() > subj_w) {
            sbuf[subj_w - 3] = sbuf[subj_w - 2] = sbuf[subj_w - 1] = '.';
            sbuf[subj_w] = '\0';
        }

        char obuf[OBJ_MAX_DISPLAY + 1];
        strncpy(obuf, t->object.c_str(), sizeof(obuf) - 1);
        obuf[sizeof(obuf) - 1] = '\0';
        if (obj_w >= 3 && (int)t->object.size() > obj_w) {
            obuf[obj_w - 3] = obuf[obj_w - 2] = obuf[obj_w - 1] = '.';
            obuf[obj_w] = '\0';
        }

        const char *src = t->inferred ? "inferred" : "declared";
        printf("| %-*s | %-*s | %-*s | %-*s |\n",
               subj_w, sbuf, pred_w, t->predicate.c_str(), obj_w, obuf,
               src_w, src);
    }

    print_rel_rule(subj_w, pred_w, obj_w, src_w);
    printf("\nTotal: %zu relation(s)\n", all.size());
}

int check_strict_links(const vibe::TripletStore *store)
{
    auto all = store->find_all();
    int warnings = 0;

    for (const auto *t : all) {
        if (t->inferred) continue;

        auto by_subj = store->find_by_subject(t->object);

        std::string inv_pred;

        for (const auto *cand : by_subj) {
            if (cand->inferred && cand->object == t->subject) {
                inv_pred = cand->predicate;
                break;
            }
        }

        if (inv_pred.empty())
            continue;

        bool found_declared = false;
        for (const auto *cand : by_subj) {
            if (!cand->inferred &&
                cand->predicate == inv_pred &&
                cand->object == t->subject) {
                found_declared = true;
                break;
            }
        }

        if (!found_declared) {
            fprintf(stderr,
                "warning: strict-links: '%s -[%s]-> %s' — "
                "inverse '[%s]' not explicitly declared by '%s'\n",
                t->subject.c_str(), t->predicate.c_str(), t->object.c_str(),
                inv_pred.c_str(), t->object.c_str());
            warnings++;
        }
    }

    return warnings;
}

static void trace_subject(const vibe::TripletStore *store,
                           const std::string &subject,
                           int depth, int max_depth)
{
    auto list = store->find_by_subject(subject);

    for (const auto *t : list) {
        if (t->inferred)
            continue;

        for (int d = 0; d < depth; d++)
            printf("  ");
        printf("-[%s]-> %s\n", t->predicate.c_str(), t->object.c_str());

        if (depth < max_depth)
            trace_subject(store, t->object, depth + 1, max_depth);
    }
}

void cmd_trace_entity(const EntityList *elist,
                      const vibe::TripletStore *store, const char *id)
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
    auto out_links = store->find_by_subject(id);
    int out_count = 0;
    for (const auto *t : out_links) {
        if (!t->inferred) out_count++;
    }
    if (out_count == 0) {
        printf("  (none)\n");
    } else {
        trace_subject(store, id, 1, 2);
    }

    printf("\nIncoming links:\n");
    auto in_links = store->find_by_object(id);
    int in_count = 0;
    for (const auto *t : in_links) {
        if (t->inferred)
            continue;
        printf("  %s -[%s]->\n", t->subject.c_str(), t->predicate.c_str());
        in_count++;
    }
    if (in_count == 0)
        printf("  (none)\n");
}

vibe::TripletStore *build_entity_relation_store(const EntityList *list)
{
    vibe::TripletStore *store = new vibe::TripletStore();

    for (const auto &e : *list) {
        entity_traceability_to_triplets(&e, store);
        entity_doc_membership_to_triplets(&e, store);
    }

    store->infer_inverses();
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

static int entity_is_member_of_document(const vibe::TripletStore *store,
                                        const char *entity_id,
                                        const char *doc_id)
{
    auto by_subject = store->find_by_subject(entity_id);

    for (const auto *t : by_subject) {
        if (t->predicate == "part-of" && t->object == doc_id)
            return 1;
    }

    return 0;
}

int collect_document_entities(const EntityList *all,
                               const vibe::TripletStore *store,
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
