/**
 * @file coverage.cpp
 * @brief Coverage and orphan query helpers for the vibe-req CLI (C++ edition).
 */

#include "coverage.h"
#include <cstdio>
#include <cstring>

#define COVERAGE_TITLE_MAX 48

static void print_rule(const int *widths, int ncols)
{
    for (int c = 0; c < ncols; c++) {
        putchar('+');
        for (int k = 0; k < widths[c] + 2; k++)
            putchar('-');
    }
    puts("+");
}

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
    CTripleList out = triplet_store_find_by_subject(store, id);
    for (size_t i = 0; i < out.count; i++) {
        if (!out.triples[i].inferred &&
            is_coverage_predicate(out.triples[i].predicate)) {
            triplet_store_list_free(out);
            return 1;
        }
    }
    triplet_store_list_free(out);

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

void cmd_coverage(const EntityList *elist, const TripletStore *store)
{
    int total   = 0;
    int covered = 0;

    for (const auto &e : *elist) {
        if (e.identity.kind != ENTITY_KIND_REQUIREMENT)
            continue;
        total++;
        if (entity_is_covered(store, e.identity.id.c_str()))
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

    int id_w     = (int)strlen("ID");
    int title_w  = (int)strlen("Title");
    int status_w = (int)strlen("Status");

    for (const auto &e : *elist) {
        if (e.identity.kind != ENTITY_KIND_REQUIREMENT)
            continue;
        if (entity_is_covered(store, e.identity.id.c_str()))
            continue;
        int len;
        len = (int)e.identity.id.size();     if (len > id_w)     id_w     = len;
        len = (int)e.lifecycle.status.size(); if (len > status_w) status_w = len;
        len = (int)e.identity.title.size();   if (len > title_w)  title_w  = len;
    }
    if (title_w > COVERAGE_TITLE_MAX) title_w = COVERAGE_TITLE_MAX;

    int w3[3] = { id_w, title_w, status_w };
    printf("\nUnlinked requirements:\n");
    print_rule(w3, 3);
    printf("| %-*s | %-*s | %-*s |\n",
           id_w, "ID", title_w, "Title", status_w, "Status");
    print_rule(w3, 3);

    for (const auto &e : *elist) {
        if (e.identity.kind != ENTITY_KIND_REQUIREMENT)
            continue;
        if (entity_is_covered(store, e.identity.id.c_str()))
            continue;

        char tbuf[COVERAGE_TITLE_MAX + 1];
        truncate_copy(tbuf, sizeof(tbuf), e.identity.title.c_str(), title_w);

        printf("| %-*s | %-*s | %-*s |\n",
               id_w,     e.identity.id.c_str(),
               title_w,  tbuf,
               status_w, e.lifecycle.status.c_str());
    }
    print_rule(w3, 3);
}

void cmd_orphan(const EntityList *elist, const TripletStore *store)
{
    int orphan_count = 0;
    for (const auto &e : *elist) {
        if (e.identity.kind != ENTITY_KIND_REQUIREMENT &&
            e.identity.kind != ENTITY_KIND_TEST_CASE)
            continue;
        if (!entity_has_any_link(store, e.identity.id.c_str()))
            orphan_count++;
    }

    if (orphan_count == 0) {
        printf("No orphaned requirements or test cases found.\n");
        return;
    }

    int id_w     = (int)strlen("ID");
    int kind_w   = (int)strlen("Kind");
    int title_w  = (int)strlen("Title");
    int status_w = (int)strlen("Status");

    for (const auto &e : *elist) {
        if (e.identity.kind != ENTITY_KIND_REQUIREMENT &&
            e.identity.kind != ENTITY_KIND_TEST_CASE)
            continue;
        if (entity_has_any_link(store, e.identity.id.c_str()))
            continue;
        int len;
        len = (int)e.identity.id.size();                         if (len > id_w)     id_w     = len;
        len = (int)strlen(entity_kind_label(e.identity.kind));   if (len > kind_w)   kind_w   = len;
        len = (int)e.lifecycle.status.size();                    if (len > status_w) status_w = len;
        len = (int)e.identity.title.size();                      if (len > title_w)  title_w  = len;
    }
    if (title_w > COVERAGE_TITLE_MAX) title_w = COVERAGE_TITLE_MAX;

    int w4[4] = { id_w, kind_w, title_w, status_w };
    printf("Orphaned requirements and test cases (no traceability links):\n");
    print_rule(w4, 4);
    printf("| %-*s | %-*s | %-*s | %-*s |\n",
           id_w, "ID", kind_w, "Kind", title_w, "Title", status_w, "Status");
    print_rule(w4, 4);

    for (const auto &e : *elist) {
        if (e.identity.kind != ENTITY_KIND_REQUIREMENT &&
            e.identity.kind != ENTITY_KIND_TEST_CASE)
            continue;
        if (entity_has_any_link(store, e.identity.id.c_str()))
            continue;

        char tbuf[COVERAGE_TITLE_MAX + 1];
        truncate_copy(tbuf, sizeof(tbuf), e.identity.title.c_str(), title_w);

        printf("| %-*s | %-*s | %-*s | %-*s |\n",
               id_w,     e.identity.id.c_str(),
               kind_w,   entity_kind_label(e.identity.kind),
               title_w,  tbuf,
               status_w, e.lifecycle.status.c_str());
    }
    print_rule(w4, 4);
    printf("\nTotal: %d orphan(s)\n", orphan_count);
}
