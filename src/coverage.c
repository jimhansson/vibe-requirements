/**
 * @file coverage.c
 * @brief Coverage and orphan query helpers for the vibe-req CLI.
 *
 * Implements the predicates and report renderers declared in coverage.h.
 * The table-printing helpers use the same column-width calculation pattern
 * as list_entities() in main.c so that output style is consistent.
 */

#include "coverage.h"

#include <stdio.h>
#include <string.h>

/* Maximum display width for a title column before truncation. */
#define COVERAGE_TITLE_MAX 48

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

/*
 * Print a horizontal rule for a table whose column widths are given in
 * widths[ncols].  Each column is rendered as "+---…---" with ncols+1 '+'
 * characters; the final '+' is printed by puts("+").
 */
static void print_rule(const int *widths, int ncols)
{
    for (int c = 0; c < ncols; c++) {
        putchar('+');
        for (int k = 0; k < widths[c] + 2; k++)
            putchar('-');
    }
    puts("+");
}

/*
 * Copy src into dst (size dst_size) and truncate to max_w characters with
 * a "..." suffix when the source is longer than max_w.
 */
static void truncate_copy(char *dst, size_t dst_size,
                           const char *src, int max_w)
{
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
    if (max_w >= 3 && (int)strlen(src) > max_w) {
        dst[max_w - 3] = '.';
        dst[max_w - 2] = '.';
        dst[max_w - 1] = '.';
        dst[max_w]     = '\0';
    }
}

/* -------------------------------------------------------------------------
 * Public API — predicates
 * ---------------------------------------------------------------------- */

int is_coverage_predicate(const char *pred)
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

int entity_is_covered(const TripletStore *store, const char *id)
{
    /* Check outgoing links from this entity. */
    CTripleList out = triplet_store_find_by_subject(store, id);
    for (size_t i = 0; i < out.count; i++) {
        if (!out.triples[i].inferred &&
            is_coverage_predicate(out.triples[i].predicate)) {
            triplet_store_list_free(out);
            return 1;
        }
    }
    triplet_store_list_free(out);

    /* Check incoming links to this entity. */
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

int entity_has_any_link(const TripletStore *store, const char *id)
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

/* -------------------------------------------------------------------------
 * Public API — report renderers
 * ---------------------------------------------------------------------- */

void cmd_coverage(const EntityList *elist, const TripletStore *store)
{
    int total   = 0;
    int covered = 0;

    /* Count covered and uncovered requirements. */
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

    if (uncovered == 0)
        return;

    /* Compute column widths for the unlinked-requirements table. */
    int id_w     = (int)strlen("ID");
    int title_w  = (int)strlen("Title");
    int status_w = (int)strlen("Status");

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
    if (title_w > COVERAGE_TITLE_MAX) title_w = COVERAGE_TITLE_MAX;

    /* Print 3-column table: ID | Title | Status */
    int w3[3] = { id_w, title_w, status_w };
    printf("\nUnlinked requirements:\n");
    print_rule(w3, 3);
    printf("| %-*s | %-*s | %-*s |\n",
           id_w, "ID", title_w, "Title", status_w, "Status");
    print_rule(w3, 3);

    for (int i = 0; i < elist->count; i++) {
        const Entity *e = &elist->items[i];
        if (e->identity.kind != ENTITY_KIND_REQUIREMENT)
            continue;
        if (entity_is_covered(store, e->identity.id))
            continue;

        char tbuf[COVERAGE_TITLE_MAX + 1];
        truncate_copy(tbuf, sizeof(tbuf), e->identity.title, title_w);

        printf("| %-*s | %-*s | %-*s |\n",
               id_w,     e->identity.id,
               title_w,  tbuf,
               status_w, e->lifecycle.status);
    }
    print_rule(w3, 3);
}

void cmd_orphan(const EntityList *elist, const TripletStore *store)
{
    /* Count orphaned requirements and test cases. */
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

    /* Compute column widths for the orphan table. */
    int id_w     = (int)strlen("ID");
    int kind_w   = (int)strlen("Kind");
    int title_w  = (int)strlen("Title");
    int status_w = (int)strlen("Status");

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
    if (title_w > COVERAGE_TITLE_MAX) title_w = COVERAGE_TITLE_MAX;

    /* Print 4-column table: ID | Kind | Title | Status */
    int w4[4] = { id_w, kind_w, title_w, status_w };
    printf("Orphaned requirements and test cases (no traceability links):\n");
    print_rule(w4, 4);
    printf("| %-*s | %-*s | %-*s | %-*s |\n",
           id_w, "ID", kind_w, "Kind", title_w, "Title", status_w, "Status");
    print_rule(w4, 4);

    for (int i = 0; i < elist->count; i++) {
        const Entity *e = &elist->items[i];
        if (e->identity.kind != ENTITY_KIND_REQUIREMENT &&
            e->identity.kind != ENTITY_KIND_TEST_CASE)
            continue;
        if (entity_has_any_link(store, e->identity.id))
            continue;

        char tbuf[COVERAGE_TITLE_MAX + 1];
        truncate_copy(tbuf, sizeof(tbuf), e->identity.title, title_w);

        printf("| %-*s | %-*s | %-*s | %-*s |\n",
               id_w,     e->identity.id,
               kind_w,   entity_kind_label(e->identity.kind),
               title_w,  tbuf,
               status_w, e->lifecycle.status);
    }
    print_rule(w4, 4);
    printf("\nTotal: %d orphan(s)\n", orphan_count);
}
