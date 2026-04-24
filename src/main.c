#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "requirement.h"
#include "entity.h"
#include "discovery.h"
#include "config.h"
#include "yaml_simple.h"
#include "triplet_store_c.h"
#include "report.h"

/* ------------------------------------------------------------------ */
/* Comparison helper for qsort — sort requirements by ID.             */
/* ------------------------------------------------------------------ */

static int cmp_by_id(const void *a, const void *b)
{
    const Requirement *ra = (const Requirement *)a;
    const Requirement *rb = (const Requirement *)b;
    return strcmp(ra->id, rb->id);
}

/* ------------------------------------------------------------------ */
/* Table rendering — requirements                                      */
/* ------------------------------------------------------------------ */

#define TITLE_MAX_DISPLAY 52  /* truncate long titles to this width */

static void print_rule(int id_w, int title_w, int type_w,
                        int status_w, int priority_w)
{
    int widths[] = { id_w, title_w, type_w, status_w, priority_w };
    int ncols    = (int)(sizeof(widths) / sizeof(widths[0]));
    for (int c = 0; c < ncols; c++) {
        putchar('+');
        for (int i = 0; i < widths[c] + 2; i++)
            putchar('-');
    }
    puts("+");
}

static void print_row(int id_w, int title_w, int type_w,
                       int status_w, int priority_w,
                       const char *id, const char *title,
                       const char *type, const char *status,
                       const char *priority)
{
    /* Truncate title if it exceeds display width. */
    char tbuf[TITLE_MAX_DISPLAY + 1];
    strncpy(tbuf, title, sizeof(tbuf) - 1);
    tbuf[sizeof(tbuf) - 1] = '\0';
    if ((int)strlen(title) > title_w) {
        tbuf[title_w - 3] = '.';
        tbuf[title_w - 2] = '.';
        tbuf[title_w - 1] = '.';
        tbuf[title_w]     = '\0';
    }

    printf("| %-*s | %-*s | %-*s | %-*s | %-*s |\n",
           id_w,       id,
           title_w,    tbuf,
           type_w,     type,
           status_w,   status,
           priority_w, priority);
}

static void list_requirements(const RequirementList *list)
{
    if (list->count == 0) {
        puts("No requirements found.");
        return;
    }

    /* Determine column widths from data. */
    int id_w       = (int)strlen("ID");
    int title_w    = (int)strlen("Title");
    int type_w     = (int)strlen("Type");
    int status_w   = (int)strlen("Status");
    int priority_w = (int)strlen("Priority");

    for (int i = 0; i < list->count; i++) {
        const Requirement *r = &list->items[i];
        int len;
        len = (int)strlen(r->id);       if (len > id_w)       id_w       = len;
        len = (int)strlen(r->title);    if (len > title_w)    title_w    = len;
        len = (int)strlen(r->type);     if (len > type_w)     type_w     = len;
        len = (int)strlen(r->status);   if (len > status_w)   status_w   = len;
        len = (int)strlen(r->priority); if (len > priority_w) priority_w = len;
    }

    /* Cap title column. */
    if (title_w > TITLE_MAX_DISPLAY)
        title_w = TITLE_MAX_DISPLAY;

    print_rule(id_w, title_w, type_w, status_w, priority_w);
    print_row(id_w, title_w, type_w, status_w, priority_w,
              "ID", "Title", "Type", "Status", "Priority");
    print_rule(id_w, title_w, type_w, status_w, priority_w);

    for (int i = 0; i < list->count; i++) {
        const Requirement *r = &list->items[i];
        print_row(id_w, title_w, type_w, status_w, priority_w,
                  r->id, r->title, r->type, r->status, r->priority);
    }

    print_rule(id_w, title_w, type_w, status_w, priority_w);
    printf("\nTotal: %d requirement(s)\n", list->count);
}

/* ------------------------------------------------------------------ */
/* Table rendering — relations                                         */
/* ------------------------------------------------------------------ */

#define SUBJ_MAX_DISPLAY 32
#define OBJ_MAX_DISPLAY  48

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

static void list_relations(const TripletStore *store)
{
    CTripleList all = triplet_store_find_all(store);

    if (all.count == 0) {
        puts("No relations found.");
        triplet_store_list_free(all);
        return;
    }

    /* Determine column widths from data. */
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

    /* Cap column widths. */
    if (subj_w > SUBJ_MAX_DISPLAY) subj_w = SUBJ_MAX_DISPLAY;
    if (obj_w  > OBJ_MAX_DISPLAY)  obj_w  = OBJ_MAX_DISPLAY;

    print_rel_rule(subj_w, pred_w, obj_w, src_w);
    printf("| %-*s | %-*s | %-*s | %-*s |\n",
           subj_w, "Subject", pred_w, "Relation", obj_w, "Object",
           src_w, "Source");
    print_rel_rule(subj_w, pred_w, obj_w, src_w);

    for (size_t i = 0; i < all.count; i++) {
        const CTriple *t = &all.triples[i];

        /* Truncate subject if needed. */
        char sbuf[SUBJ_MAX_DISPLAY + 1];
        strncpy(sbuf, t->subject, sizeof(sbuf) - 1);
        sbuf[sizeof(sbuf) - 1] = '\0';
        if (subj_w >= 3 && (int)strlen(t->subject) > subj_w) {
            sbuf[subj_w - 3] = sbuf[subj_w - 2] = sbuf[subj_w - 1] = '.';
            sbuf[subj_w] = '\0';
        }

        /* Truncate object if needed. */
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

/* ------------------------------------------------------------------ */
/* Build the triplet store from the parsed requirement list            */
/* ------------------------------------------------------------------ */

static TripletStore *build_relation_store(const RequirementList *list)
{
    TripletStore *store = triplet_store_create();
    if (!store)
        return NULL;

    for (int i = 0; i < list->count; i++) {
        const Requirement *r = &list->items[i];
        if (yaml_parse_links(r->file_path, r->id, store) < 0)
            fprintf(stderr, "warning: could not parse links from '%s'\n",
                    r->file_path);
    }

    /* Generate inferred inverse triples for all known relation pairs. */
    triplet_store_infer_inverses(store);

    return store;
}

/* ------------------------------------------------------------------ */
/* --strict-links validation                                          */
/* ------------------------------------------------------------------ */

/*
 * For every user-declared triple (A, rel, B) where rel has a known
 * inverse inv(rel), check that (B, inv(rel), A) is ALSO user-declared.
 * If only the inferred direction exists, emit a warning to stderr.
 *
 * The inferred triple (B, inv_pred, A) — already present in the store
 * after triplet_store_infer_inverses() was called — is used as a proxy
 * to determine inv_pred without duplicating the relation-pair table in C.
 *
 * Note: this function is O(n * m) where n is the number of declared
 * triples and m is the average out-degree per subject.  For typical
 * repositories this is acceptable; a hash-based approach can be added
 * later if profiling shows it to be a bottleneck.
 *
 * Returns the number of one-sided link warnings found.
 */
static int check_strict_links(const TripletStore *store)
{
    CTripleList all = triplet_store_find_all(store);
    int warnings = 0;

    for (size_t i = 0; i < all.count; i++) {
        const CTriple *t = &all.triples[i];
        if (t->inferred) continue; /* only inspect user-declared triples */

        /*
         * Look for the inferred inverse (t->object, inv_pred, t->subject).
         * Its existence confirms that t->predicate is in the known-pair
         * table and tells us the expected inverse predicate name.
         */
        CTripleList by_subj = triplet_store_find_by_subject(store, t->object);

        /* Copy the inverse predicate into a local buffer before freeing
         * by_subj, to avoid a use-after-free when writing the warning. */
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
            /* No inferred inverse — relation is unknown; skip (Option C). */
            triplet_store_list_free(by_subj);
            continue;
        }

        /* Check whether a user-declared (t->object, inv_pred, t->subject)
         * triple already exists. */
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

/* ------------------------------------------------------------------ */
/* Trace — subject-focused traceability chain traversal               */
/* ------------------------------------------------------------------ */

/*
 * Print all triples with the given subject at the specified indent level.
 * Recurse one level deeper for each object that has outgoing triples of
 * its own, up to max_depth hops.
 */
static void trace_subject(const TripletStore *store, const char *subject,
                           int depth, int max_depth)
{
    CTripleList list = triplet_store_find_by_subject(store, subject);

    for (size_t i = 0; i < list.count; i++) {
        const CTriple *t = &list.triples[i];
        if (t->inferred)
            continue; /* show only user-declared triples */

        for (int d = 0; d < depth; d++)
            printf("  ");
        printf("-[%s]-> %s\n", t->predicate, t->object);

        if (depth < max_depth) {
            trace_subject(store, t->object, depth + 1, max_depth);
        }
    }

    triplet_store_list_free(list);
}

/* ------------------------------------------------------------------ */
/* Entity table constants (used across multiple functions below)      */
/* ------------------------------------------------------------------ */

#define ENTITY_TITLE_MAX  48
#define ENTITY_KIND_MAX   12
#define ENTITY_STATUS_MAX 16
#define ENTITY_PRIO_MAX   12

/* ------------------------------------------------------------------ */
/* Entity-aware trace — shows entity info + outgoing + incoming links */
/* ------------------------------------------------------------------ */

static void cmd_trace_entity(const EntityList *elist,
                              const TripletStore *store, const char *id)
{
    /* Locate the entity record if available. */
    const Entity *found = NULL;
    for (int i = 0; i < elist->count; i++) {
        if (strcmp(elist->items[i].identity.id, id) == 0) {
            found = &elist->items[i];
            break;
        }
    }

    printf("Traceability chain for: %s\n", id);

    if (found) {
        printf("  Kind:   %s\n", entity_kind_label(found->identity.kind));
        if (found->identity.title[0] != '\0')
            printf("  Title:  %s\n", found->identity.title);
        if (found->lifecycle.status[0] != '\0')
            printf("  Status: %s\n", found->lifecycle.status);
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
        /* Use trace_subject for consistent indented recursive output. */
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

/* ------------------------------------------------------------------ */
/* Build entity triplet store from ECS EntityList                     */
/* ------------------------------------------------------------------ */

static TripletStore *build_entity_relation_store(const EntityList *list)
{
    TripletStore *store = triplet_store_create();
    if (!store)
        return NULL;

    for (int i = 0; i < list->count; i++) {
        entity_traceability_to_triplets(&list->items[i], store);
    }

    triplet_store_infer_inverses(store);
    return store;
}

/* ------------------------------------------------------------------ */
/* Coverage — requirement coverage report                             */
/* ------------------------------------------------------------------ */

/*
 * Return 1 if the predicate string is considered a coverage-indicating
 * relation (connects a requirement to a test or code implementation).
 */
static int is_coverage_predicate(const char *pred)
{
    static const char *preds[] = {
        "verifies", "verified-by",
        "implements", "implemented-by", "implemented-in",
        "implemented-by-test",
        "tests", "tested-by",
        "satisfies", "satisfied-by",
        NULL
    };
    for (int i = 0; preds[i]; i++) {
        if (strcmp(pred, preds[i]) == 0)
            return 1;
    }
    return 0;
}

/*
 * Return 1 if entity *e has at least one traceability link (outgoing or
 * incoming) in *store that uses a coverage-indicating predicate.
 */
static int entity_is_covered(const TripletStore *store, const char *id)
{
    /* Outgoing links from this entity. */
    CTripleList out = triplet_store_find_by_subject(store, id);
    for (size_t i = 0; i < out.count; i++) {
        if (!out.triples[i].inferred &&
            is_coverage_predicate(out.triples[i].predicate)) {
            triplet_store_list_free(out);
            return 1;
        }
    }
    triplet_store_list_free(out);

    /* Incoming links to this entity. */
    CTripleList in = triplet_store_find_by_object(store, id);
    for (size_t i = 0; i < in.count; i++) {
        if (!in.triples[i].inferred &&
            is_coverage_predicate(in.triples[i].predicate)) {
            triplet_store_list_free(in);
            return 1;
        }
    }
    triplet_store_list_free(in);

    return 0;
}

static void cmd_coverage(const EntityList *elist, const TripletStore *store)
{
    int total   = 0;
    int covered = 0;

    /* First pass: count. */
    for (int i = 0; i < elist->count; i++) {
        const Entity *e = &elist->items[i];
        if (e->identity.kind != ENTITY_KIND_REQUIREMENT)
            continue;
        total++;
        if (entity_is_covered(store, e->identity.id))
            covered++;
    }

    int uncovered = total - covered;
    int pct_cov   = (total > 0) ? (covered   * 100 / total) : 0;
    int pct_unc   = (total > 0) ? (100 - pct_cov)           : 0;

    printf("Coverage Report\n");
    printf("===============\n");
    printf("Total requirements:    %d\n", total);
    printf("Linked requirements:   %d (%d%%)\n", covered,   pct_cov);
    printf("Unlinked requirements: %d (%d%%)\n", uncovered, pct_unc);

    if (uncovered > 0) {
        /* Column widths */
        int id_w     = (int)strlen("ID");
        int status_w = (int)strlen("Status");
        int title_w  = (int)strlen("Title");

        for (int i = 0; i < elist->count; i++) {
            const Entity *e = &elist->items[i];
            if (e->identity.kind != ENTITY_KIND_REQUIREMENT)
                continue;
            if (entity_is_covered(store, e->identity.id))
                continue;
            int len;
            len = (int)strlen(e->identity.id);
            if (len > id_w) id_w = len;
            len = (int)strlen(e->lifecycle.status);
            if (len > status_w) status_w = len;
            len = (int)strlen(e->identity.title);
            if (len > title_w) title_w = len;
        }
        if (title_w > ENTITY_TITLE_MAX) title_w = ENTITY_TITLE_MAX;

        /* Print 3-column table: ID | Title | Status */
        int w3[3] = { id_w, title_w, status_w };
        printf("\nUnlinked requirements:\n");
        for (int c = 0; c < 3; c++) {
            putchar('+');
            for (int k = 0; k < w3[c] + 2; k++) putchar('-');
        }
        puts("+");
        printf("| %-*s | %-*s | %-*s |\n",
               id_w, "ID", title_w, "Title", status_w, "Status");
        for (int c = 0; c < 3; c++) {
            putchar('+');
            for (int k = 0; k < w3[c] + 2; k++) putchar('-');
        }
        puts("+");

        for (int i = 0; i < elist->count; i++) {
            const Entity *e = &elist->items[i];
            if (e->identity.kind != ENTITY_KIND_REQUIREMENT)
                continue;
            if (entity_is_covered(store, e->identity.id))
                continue;

            char tbuf[ENTITY_TITLE_MAX + 1];
            strncpy(tbuf, e->identity.title, sizeof(tbuf) - 1);
            tbuf[sizeof(tbuf) - 1] = '\0';
            if ((int)strlen(e->identity.title) > title_w) {
                tbuf[title_w - 3] = '.';
                tbuf[title_w - 2] = '.';
                tbuf[title_w - 1] = '.';
                tbuf[title_w]     = '\0';
            }

            printf("| %-*s | %-*s | %-*s |\n",
                   id_w,     e->identity.id,
                   title_w,  tbuf,
                   status_w, e->lifecycle.status);
        }
        for (int c = 0; c < 3; c++) {
            putchar('+');
            for (int k = 0; k < w3[c] + 2; k++) putchar('-');
        }
        puts("+");
    }
}

/* ------------------------------------------------------------------ */
/* Orphan — entities with no traceability links in either direction   */
/* ------------------------------------------------------------------ */

/*
 * Return 1 if entity with given *id has at least one declared (non-inferred)
 * traceability link in the store — either outgoing or incoming.
 */
static int entity_has_any_link(const TripletStore *store, const char *id)
{
    CTripleList out = triplet_store_find_by_subject(store, id);
    for (size_t i = 0; i < out.count; i++) {
        if (!out.triples[i].inferred) {
            triplet_store_list_free(out);
            return 1;
        }
    }
    triplet_store_list_free(out);

    CTripleList in = triplet_store_find_by_object(store, id);
    for (size_t i = 0; i < in.count; i++) {
        if (!in.triples[i].inferred) {
            triplet_store_list_free(in);
            return 1;
        }
    }
    triplet_store_list_free(in);

    return 0;
}

static void cmd_orphan(const EntityList *elist, const TripletStore *store)
{
    /* Count orphans (requirements and test-cases only). */
    int orphan_count = 0;
    for (int i = 0; i < elist->count; i++) {
        const Entity *e = &elist->items[i];
        if (e->identity.kind != ENTITY_KIND_REQUIREMENT &&
            e->identity.kind != ENTITY_KIND_TEST_CASE)
            continue;
        if (!entity_has_any_link(store, e->identity.id))
            orphan_count++;
    }

    if (orphan_count == 0) {
        printf("No orphaned requirements or test cases found.\n");
        return;
    }

    /* Column widths. */
    int id_w     = (int)strlen("ID");
    int kind_w   = (int)strlen("Kind");
    int status_w = (int)strlen("Status");
    int title_w  = (int)strlen("Title");

    for (int i = 0; i < elist->count; i++) {
        const Entity *e = &elist->items[i];
        if (e->identity.kind != ENTITY_KIND_REQUIREMENT &&
            e->identity.kind != ENTITY_KIND_TEST_CASE)
            continue;
        if (entity_has_any_link(store, e->identity.id))
            continue;
        int len;
        len = (int)strlen(e->identity.id);
        if (len > id_w) id_w = len;
        len = (int)strlen(entity_kind_label(e->identity.kind));
        if (len > kind_w) kind_w = len;
        len = (int)strlen(e->lifecycle.status);
        if (len > status_w) status_w = len;
        len = (int)strlen(e->identity.title);
        if (len > title_w) title_w = len;
    }
    if (title_w > ENTITY_TITLE_MAX) title_w = ENTITY_TITLE_MAX;

    printf("Orphaned requirements and test cases (no traceability links):\n");
    /* Print 4-column table: ID | Kind | Title | Status */
    int w4[4] = { id_w, kind_w, title_w, status_w };
    for (int c = 0; c < 4; c++) {
        putchar('+');
        for (int k = 0; k < w4[c] + 2; k++) putchar('-');
    }
    puts("+");
    printf("| %-*s | %-*s | %-*s | %-*s |\n",
           id_w, "ID", kind_w, "Kind", title_w, "Title", status_w, "Status");
    for (int c = 0; c < 4; c++) {
        putchar('+');
        for (int k = 0; k < w4[c] + 2; k++) putchar('-');
    }
    puts("+");

    for (int i = 0; i < elist->count; i++) {
        const Entity *e = &elist->items[i];
        if (e->identity.kind != ENTITY_KIND_REQUIREMENT &&
            e->identity.kind != ENTITY_KIND_TEST_CASE)
            continue;
        if (entity_has_any_link(store, e->identity.id))
            continue;

        char tbuf[ENTITY_TITLE_MAX + 1];
        strncpy(tbuf, e->identity.title, sizeof(tbuf) - 1);
        tbuf[sizeof(tbuf) - 1] = '\0';
        if ((int)strlen(e->identity.title) > title_w) {
            tbuf[title_w - 3] = '.';
            tbuf[title_w - 2] = '.';
            tbuf[title_w - 1] = '.';
            tbuf[title_w]     = '\0';
        }

        printf("| %-*s | %-*s | %-*s | %-*s |\n",
               id_w,     e->identity.id,
               kind_w,   entity_kind_label(e->identity.kind),
               title_w,  tbuf,
               status_w, e->lifecycle.status);
    }
    for (int c = 0; c < 4; c++) {
        putchar('+');
        for (int k = 0; k < w4[c] + 2; k++) putchar('-');
    }
    puts("+");
    printf("\nTotal: %d orphan(s)\n", orphan_count);
}

/* ------------------------------------------------------------------ */
/* Table rendering — entities (ECS)                                   */
/* ------------------------------------------------------------------ */

static int cmp_entity_by_id(const void *a, const void *b)
{
    const Entity *ea = (const Entity *)a;
    const Entity *eb = (const Entity *)b;
    return strcmp(ea->identity.id, eb->identity.id);
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

static void list_entities(const EntityList *list)
{
    if (list->count == 0) {
        puts("No entities found.");
        return;
    }

    int id_w     = (int)strlen("ID");
    int title_w  = (int)strlen("Title");
    int kind_w   = (int)strlen("Kind");
    int status_w = (int)strlen("Status");
    int prio_w   = (int)strlen("Priority");

    for (int i = 0; i < list->count; i++) {
        const Entity *e = &list->items[i];
        int len;
        len = (int)strlen(e->identity.id);         if (len > id_w)     id_w     = len;
        len = (int)strlen(e->identity.title);       if (len > title_w)  title_w  = len;
        len = (int)strlen(entity_kind_label(e->identity.kind));
                                                    if (len > kind_w)   kind_w   = len;
        len = (int)strlen(e->lifecycle.status);     if (len > status_w) status_w = len;
        len = (int)strlen(e->lifecycle.priority);   if (len > prio_w)   prio_w   = len;
    }

    if (title_w > ENTITY_TITLE_MAX) title_w = ENTITY_TITLE_MAX;

    print_entity_rule(id_w, title_w, kind_w, status_w, prio_w);
    print_entity_row(id_w, title_w, kind_w, status_w, prio_w,
                     "ID", "Title", "Kind", "Status", "Priority");
    print_entity_rule(id_w, title_w, kind_w, status_w, prio_w);

    for (int i = 0; i < list->count; i++) {
        const Entity *e = &list->items[i];
        print_entity_row(id_w, title_w, kind_w, status_w, prio_w,
                         e->identity.id,
                         e->identity.title,
                         entity_kind_label(e->identity.kind),
                         e->lifecycle.status,
                         e->lifecycle.priority);
    }

    print_entity_rule(id_w, title_w, kind_w, status_w, prio_w);
    printf("\nTotal: %d %s\n", list->count,
           list->count == 1 ? "entity" : "entities");
}

/* ------------------------------------------------------------------ */
/* Entity list filtering                                              */
/* ------------------------------------------------------------------ */

/*
 * Build a filtered view of *src into *dst (caller owns dst and must call
 * entity_list_free when done).  Pass NULL for any filter to disable it.
 *
 * filter_kind      — EntityKind label, e.g. "user-story", "requirement"
 * filter_comp      — component name, e.g. "assumption", "traceability"
 * filter_status    — lifecycle status, e.g. "draft", "approved"
 * filter_priority  — lifecycle priority, e.g. "must", "should"
 */
static void entity_apply_filter(const EntityList *src, EntityList *dst,
                                 const char *filter_kind,
                                 const char *filter_comp,
                                 const char *filter_status,
                                 const char *filter_priority)
{
    /* Resolve kind filter once up-front. */
    int has_kind = (filter_kind && filter_kind[0] != '\0');
    EntityKind kind_val = ENTITY_KIND_UNKNOWN;
    if (has_kind) {
        kind_val = entity_kind_from_string(filter_kind);
        if (kind_val == ENTITY_KIND_UNKNOWN &&
            strcmp(filter_kind, "unknown") != 0) {
            fprintf(stderr, "warning: unrecognised kind '%s'\n", filter_kind);
        }
    }

    for (int i = 0; i < src->count; i++) {
        const Entity *e = &src->items[i];

        if (has_kind && e->identity.kind != kind_val)
            continue;

        if (filter_comp && filter_comp[0] != '\0' &&
            !entity_has_component(e, filter_comp)) {
            continue;
        }

        if (filter_status && filter_status[0] != '\0' &&
            strcmp(e->lifecycle.status, filter_status) != 0) {
            continue;
        }

        if (filter_priority && filter_priority[0] != '\0' &&
            strcmp(e->lifecycle.priority, filter_priority) != 0) {
            continue;
        }

        entity_list_add(dst, e);
    }
}

/* ------------------------------------------------------------------ */
/* Discovery for entities                                              */
/* ------------------------------------------------------------------ */

static void walk_entities(const char *dir, EntityList *list,
                           const VibeConfig *cfg)
{
    DIR *d = opendir(dir);
    if (!d)
        return;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        const char *name = entry->d_name;

        if (name[0] == '.')
            continue;

        char path[512];
        int n = snprintf(path, sizeof(path), "%s/%s", dir, name);
        if (n < 0 || (size_t)n >= sizeof(path)) {
            fprintf(stderr, "warning: path too long, skipping: %s/%s\n",
                    dir, name);
            continue;
        }

        struct stat st;
        if (stat(path, &st) != 0)
            continue;

        if (S_ISDIR(st.st_mode)) {
            if (config_is_ignored_dir(cfg, name))
                continue;
            walk_entities(path, list, cfg);
        } else if (S_ISREG(st.st_mode)) {
            const char *dot = strrchr(name, '.');
            if (!dot)
                continue;
            if (strcmp(dot, ".yaml") != 0 && strcmp(dot, ".yml") != 0)
                continue;
            if (yaml_parse_entities(path, list) < 0)
                fprintf(stderr, "warning: could not parse: %s\n", path);
        }
    }

    closedir(d);
}

static int discover_entities(const char *root_dir, EntityList *list,
                              const VibeConfig *cfg)
{
    DIR *probe = opendir(root_dir);
    if (!probe)
        return -1;
    closedir(probe);

    walk_entities(root_dir, list, cfg);
    return list->count;
}

int main(int argc, char *argv[])
{
    /* Parse optional subcommand and directory arguments. */
    int         show_links    = 0;
    int         strict_links  = 0;
    int         show_entities = 0;
    int         show_coverage = 0;
    int         show_orphan   = 0;
    int         show_report   = 0;
    const char *trace_id      = NULL;
    const char *root          = ".";

    /* Filter options (used with 'entities' / 'list' / 'report' subcommands). */
    const char *filter_kind     = NULL;
    const char *filter_comp     = NULL;
    const char *filter_status   = NULL;
    const char *filter_priority = NULL;

    /* Report-specific options. */
    const char  *report_output = NULL; /* NULL = stdout */

    int arg_idx = 1;

    if (argc >= 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("Usage: %s [command] [options] [directory]\n\n",
                   argv[0]);
            printf("Commands:\n");
            printf("  (default)       List all requirements found in the directory tree.\n");
            printf("  list            List all entities (all kinds) with optional filters.\n");
            printf("  entities        Alias for 'list'.\n");
            printf("  links           List all relations parsed from requirement files.\n");
            printf("  trace <id>      Show full traceability chain for an entity.\n");
            printf("                  Displays entity info, outgoing links, and incoming links.\n");
            printf("                  Example: %s trace REQ-001\n", argv[0]);
            printf("  coverage        Report how many requirements have traceability links\n");
            printf("                  to tests or code (verified-by, implemented-by, etc.).\n");
            printf("                  Example: %s coverage\n", argv[0]);
            printf("  orphan          List requirements and test cases with no traceability\n");
            printf("                  links in either direction.\n");
            printf("                  Example: %s orphan\n", argv[0]);
            printf("  report          Generate a Markdown report of all entities.\n");
            printf("                  Use --output and filter flags to customise output.\n");
            printf("                  Example: %s report --output report.md\n\n",
                   argv[0]);
            printf("Filter options (for 'list' / 'entities' / 'report'):\n");
            printf("  --kind <kind>        Show only entities of the given kind.\n");
            printf("                       Kinds: requirement, group, story, design-note,\n");
            printf("                              section, assumption, constraint, test-case,\n");
            printf("                              external, document\n");
            printf("  --component <comp>   Show only entities that carry the named component.\n");
            printf("                       Components: user-story, acceptance-criteria, epic,\n");
            printf("                                  assumption, constraint, doc-meta,\n");
            printf("                                  doc-membership, doc-body, traceability,\n");
            printf("                                  tags, test-procedure, clause-collection,\n");
            printf("                                  attachment\n");
            printf("  --status <status>    Show only entities with the given lifecycle status.\n");
            printf("  --priority <prio>    Show only entities with the given priority.\n\n");
            printf("Report options (for 'report'):\n");
            printf("  --output <file> Write report to <file> instead of stdout.\n\n");
            printf("Other options:\n");
            printf("  --strict-links  Warn when a known relation is declared in only one\n");
            printf("                  direction (inverse not explicitly present in YAML).\n");
            printf("                  Exits with a non-zero code if any warnings are found.\n");
            printf("  directory       Root directory to scan (default: current directory).\n\n");
            printf("  YAML files without a top-level 'id' field are silently ignored.\n");
            printf("  Traceability links may be declared under either 'traceability:' or\n");
            printf("  'links:' YAML keys; both formats are recognised.\n");
            return 0;
        }
        if (strcmp(argv[1], "links") == 0) {
            show_links = 1;
            arg_idx    = 2;
        } else if (strcmp(argv[1], "entities") == 0 ||
                   strcmp(argv[1], "list") == 0) {
            show_entities = 1;
            arg_idx       = 2;
        } else if (strcmp(argv[1], "trace") == 0) {
            if (argc < 3) {
                fprintf(stderr, "error: 'trace' requires an entity ID argument\n");
                return 1;
            }
            trace_id = argv[2];
            arg_idx  = 3;
        } else if (strcmp(argv[1], "coverage") == 0) {
            show_coverage = 1;
            arg_idx       = 2;
        } else if (strcmp(argv[1], "orphan") == 0) {
            show_orphan = 1;
            arg_idx     = 2;
        } else if (strcmp(argv[1], "report") == 0) {
            show_report = 1;
            arg_idx     = 2;
        }
    }

    /* Scan remaining arguments for flags and directory. */
    for (int i = arg_idx; i < argc; i++) {
        if (strcmp(argv[i], "--strict-links") == 0) {
            strict_links = 1;
        } else if (strcmp(argv[i], "--kind") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: '--kind' requires a value\n");
                return 1;
            }
            filter_kind = argv[++i];
        } else if (strcmp(argv[i], "--component") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: '--component' requires a value\n");
                return 1;
            }
            filter_comp = argv[++i];
        } else if (strcmp(argv[i], "--status") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: '--status' requires a value\n");
                return 1;
            }
            filter_status = argv[++i];
        } else if (strcmp(argv[i], "--priority") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: '--priority' requires a value\n");
                return 1;
            }
            filter_priority = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: '--output' requires a file path\n");
                return 1;
            }
            report_output = argv[++i];
        } else {
            root = argv[i];
        }
    }

    /* ------------------------------------------------------------------
     * Entity-path subcommands: trace, coverage, orphan, list/entities,
     * report.  All use the ECS EntityList.
     * ------------------------------------------------------------------ */
    if (show_entities || trace_id || show_coverage || show_orphan ||
        show_report) {
        EntityList elist;
        entity_list_init(&elist);

        VibeConfig cfg;
        config_load(root, &cfg);

        int found = discover_entities(root, &elist, &cfg);
        if (found < 0) {
            fprintf(stderr, "error: cannot open directory '%s'\n", root);
            entity_list_free(&elist);
            return 1;
        }

        if (elist.count > 1)
            qsort(elist.items, (size_t)elist.count, sizeof(Entity),
                  cmp_entity_by_id);

        if (show_entities) {
            /* Apply filters if any were specified. */
            if (filter_kind || filter_comp || filter_status || filter_priority) {
                EntityList filtered;
                entity_list_init(&filtered);
                entity_apply_filter(&elist, &filtered,
                                    filter_kind, filter_comp,
                                    filter_status, filter_priority);
                list_entities(&filtered);
                entity_list_free(&filtered);
            } else {
                list_entities(&elist);
            }
            entity_list_free(&elist);
            return 0;
        }

        if (show_report) {
            /* Build relation store for traceability links in the report. */
            TripletStore *store = build_entity_relation_store(&elist);
            if (!store) {
                fprintf(stderr, "error: failed to create relation store\n");
                entity_list_free(&elist);
                return 1;
            }

            /* Apply filters if any were specified. */
            EntityList *src = &elist;
            EntityList  filtered;
            entity_list_init(&filtered);
            if (filter_kind || filter_comp || filter_status || filter_priority) {
                entity_apply_filter(&elist, &filtered,
                                    filter_kind, filter_comp,
                                    filter_status, filter_priority);
                src = &filtered;
            }

            /* Open output destination. */
            FILE *out = stdout;
            if (report_output) {
                out = fopen(report_output, "w");
                if (!out) {
                    fprintf(stderr,
                            "error: cannot open output file '%s'\n",
                            report_output);
                    entity_list_free(&filtered);
                    triplet_store_destroy(store);
                    entity_list_free(&elist);
                    return 1;
                }
            }

            report_write(out, src, store);

            if (report_output)
                fclose(out);

            entity_list_free(&filtered);
            triplet_store_destroy(store);
            entity_list_free(&elist);
            return 0;
        }

        /* trace / coverage / orphan need the relation store. */
        TripletStore *store = build_entity_relation_store(&elist);
        if (!store) {
            fprintf(stderr, "error: failed to create relation store\n");
            entity_list_free(&elist);
            return 1;
        }

        if (trace_id)
            cmd_trace_entity(&elist, store, trace_id);
        if (show_coverage)
            cmd_coverage(&elist, store);
        if (show_orphan)
            cmd_orphan(&elist, store);

        triplet_store_destroy(store);
        entity_list_free(&elist);
        return 0;
    }

    /* ------------------------------------------------------------------
     * Default / links / strict-links — legacy RequirementList path.
     * ------------------------------------------------------------------ */
    RequirementList list;
    req_list_init(&list);

    VibeConfig cfg;
    /* config_load() returns -1 when .vibe-req.yaml is absent or unparseable,
     * which is perfectly valid — cfg is zeroed so discovery runs unfiltered. */
    config_load(root, &cfg);

    int found = discover_requirements(root, &list, &cfg);
    if (found < 0) {
        fprintf(stderr, "error: cannot open directory '%s'\n", root);
        req_list_free(&list);
        return 1;
    }

    /* Sort alphabetically by ID before displaying. */
    if (list.count > 1)
        qsort(list.items, (size_t)list.count, sizeof(Requirement), cmp_by_id);

    int exit_code = 0;

    if (show_links || strict_links) {
        TripletStore *store = build_relation_store(&list);
        if (!store) {
            fprintf(stderr, "error: failed to create relation store\n");
            req_list_free(&list);
            return 1;
        }
        if (show_links)
            list_relations(store);
        if (strict_links) {
            int warnings = check_strict_links(store);
            if (warnings > 0) {
                fprintf(stderr, "%d strict-links warning(s) found.\n", warnings);
                exit_code = 1;
            }
        }
        triplet_store_destroy(store);
    } else {
        list_requirements(&list);
    }

    req_list_free(&list);
    return exit_code;
}
