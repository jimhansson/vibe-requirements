/**
 * @file test_triplet_store.c
 * @brief Unit tests for the relation triplet store C API.
 *
 * Written in C to verify that the C API header is self-contained and
 * callable from plain C translation units.
 */

#include "triplet_store_c.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Tiny test framework
 * ---------------------------------------------------------------------- */

static int g_tests_run    = 0;
static int g_tests_failed = 0;

#define ASSERT(cond)                                                        \
    do {                                                                    \
        ++g_tests_run;                                                      \
        if (!(cond)) {                                                      \
            fprintf(stderr, "FAIL  %s:%d  %s\n",                           \
                    __FILE__, __LINE__, #cond);                             \
            ++g_tests_failed;                                               \
        }                                                                   \
    } while (0)

#define ASSERT_EQ(a, b)                                                     \
    do {                                                                    \
        ++g_tests_run;                                                      \
        if ((size_t)(a) != (size_t)(b)) {                                   \
            fprintf(stderr, "FAIL  %s:%d  expected %zu, got %zu\n",        \
                    __FILE__, __LINE__, (size_t)(b), (size_t)(a));          \
            ++g_tests_failed;                                               \
        }                                                                   \
    } while (0)

#define ASSERT_STREQ(a, b)                                                  \
    do {                                                                    \
        ++g_tests_run;                                                      \
        if (strcmp((a), (b)) != 0) {                                        \
            fprintf(stderr, "FAIL  %s:%d  expected \"%s\", got \"%s\"\n",  \
                    __FILE__, __LINE__, (b), (a));                          \
            ++g_tests_failed;                                               \
        }                                                                   \
    } while (0)

/* -------------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */

static void test_create_destroy(void)
{
    TripletStore *store = triplet_store_create();
    ASSERT(store != NULL);
    ASSERT_EQ(triplet_store_count(store), 0);
    triplet_store_destroy(store);
}

static void test_add_and_count(void)
{
    TripletStore *store = triplet_store_create();

    size_t id1 = triplet_store_add(store,
                                   "REQ-SW-001", "derives-from", "REQ-SYS-005");
    ASSERT(id1 != TRIPLE_ID_INVALID);

    size_t id2 = triplet_store_add(store,
                                   "REQ-SW-001", "implemented-in",
                                   "src/auth/login.c");
    ASSERT(id2 != TRIPLE_ID_INVALID);
    ASSERT(id1 != id2);
    ASSERT_EQ(triplet_store_count(store), 2);

    triplet_store_destroy(store);
}

static void test_duplicate_rejected(void)
{
    TripletStore *store = triplet_store_create();

    size_t id1 = triplet_store_add(store,
                                   "REQ-SW-001", "derives-from", "REQ-SYS-005");
    ASSERT(id1 != TRIPLE_ID_INVALID);

    /* Exact duplicate must be rejected. */
    size_t id2 = triplet_store_add(store,
                                   "REQ-SW-001", "derives-from", "REQ-SYS-005");
    ASSERT_EQ(id2, TRIPLE_ID_INVALID);

    /* Count must remain 1. */
    ASSERT_EQ(triplet_store_count(store), 1);

    triplet_store_destroy(store);
}

static void test_remove_by_id(void)
{
    TripletStore *store = triplet_store_create();

    size_t id1 = triplet_store_add(store,
                                   "REQ-SW-001", "derives-from", "REQ-SYS-005");
    size_t id2 = triplet_store_add(store,
                                   "TC-SW-001",  "verifies",     "REQ-SW-001");
    ASSERT_EQ(triplet_store_count(store), 2);

    int removed = triplet_store_remove(store, id1);
    ASSERT_EQ(removed, 1);
    ASSERT_EQ(triplet_store_count(store), 1);

    /* Removing the same ID again must return 0. */
    removed = triplet_store_remove(store, id1);
    ASSERT_EQ(removed, 0);
    ASSERT_EQ(triplet_store_count(store), 1);

    triplet_store_destroy(store);
    (void)id2;
}

static void test_triple_content(void)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "TC-SW-001", "verifies", "REQ-SW-001");

    CTripleList list = triplet_store_find_by_subject(store, "TC-SW-001");
    ASSERT_EQ(list.count, 1);
    ASSERT_STREQ(list.triples[0].subject,   "TC-SW-001");
    ASSERT_STREQ(list.triples[0].predicate, "verifies");
    ASSERT_STREQ(list.triples[0].object,    "REQ-SW-001");

    triplet_store_list_free(list);
    triplet_store_destroy(store);
}

static void test_find_by_subject(void)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from",   "REQ-SYS-005");
    triplet_store_add(store, "REQ-SW-001", "implemented-in", "src/auth/login.c");
    triplet_store_add(store, "TC-SW-001",  "verifies",       "REQ-SW-001");

    CTripleList list = triplet_store_find_by_subject(store, "REQ-SW-001");
    ASSERT_EQ(list.count, 2);
    triplet_store_list_free(list);

    /* Unknown subject must return empty list. */
    list = triplet_store_find_by_subject(store, "REQ-UNKNOWN");
    ASSERT_EQ(list.count, 0);
    triplet_store_list_free(list);

    triplet_store_destroy(store);
}

static void test_find_by_object(void)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "REQ-SW-002", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "TC-SW-001",  "verifies",     "REQ-SW-001");

    CTripleList list = triplet_store_find_by_object(store, "REQ-SYS-005");
    ASSERT_EQ(list.count, 2);
    triplet_store_list_free(list);

    triplet_store_destroy(store);
}

static void test_find_by_predicate(void)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "REQ-SW-002", "derives-from", "REQ-SYS-006");
    triplet_store_add(store, "TC-SW-001",  "verifies",     "REQ-SW-001");

    CTripleList list = triplet_store_find_by_predicate(store, "derives-from");
    ASSERT_EQ(list.count, 2);
    triplet_store_list_free(list);

    triplet_store_destroy(store);
}

static void test_find_all(void)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "TC-SW-001",  "verifies",     "REQ-SW-001");

    CTripleList list = triplet_store_find_all(store);
    ASSERT_EQ(list.count, 2);
    triplet_store_list_free(list);

    triplet_store_destroy(store);
}

static void test_remove_by_subject(void)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from",   "REQ-SYS-005");
    triplet_store_add(store, "REQ-SW-001", "implemented-in", "src/auth/login.c");
    triplet_store_add(store, "TC-SW-001",  "verifies",       "REQ-SW-001");

    size_t n = triplet_store_remove_by_subject(store, "REQ-SW-001");
    ASSERT_EQ(n, 2);
    ASSERT_EQ(triplet_store_count(store), 1);

    /* The remaining triple must still be queryable. */
    CTripleList list = triplet_store_find_by_subject(store, "TC-SW-001");
    ASSERT_EQ(list.count, 1);
    triplet_store_list_free(list);

    triplet_store_destroy(store);
}

static void test_remove_by_object(void)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "REQ-SW-002", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "TC-SW-001",  "verifies",     "REQ-SW-001");

    size_t n = triplet_store_remove_by_object(store, "REQ-SYS-005");
    ASSERT_EQ(n, 2);
    ASSERT_EQ(triplet_store_count(store), 1);

    triplet_store_destroy(store);
}

static void test_remove_by_predicate(void)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "REQ-SW-002", "derives-from", "REQ-SYS-006");
    triplet_store_add(store, "TC-SW-001",  "verifies",     "REQ-SW-001");

    size_t n = triplet_store_remove_by_predicate(store, "derives-from");
    ASSERT_EQ(n, 2);
    ASSERT_EQ(triplet_store_count(store), 1);

    triplet_store_destroy(store);
}

static void test_clear(void)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "TC-SW-001",  "verifies",     "REQ-SW-001");
    triplet_store_clear(store);

    ASSERT_EQ(triplet_store_count(store), 0);

    CTripleList list = triplet_store_find_all(store);
    ASSERT_EQ(list.count, 0);
    triplet_store_list_free(list);

    /* Adding after clear must work normally. */
    size_t id = triplet_store_add(store, "REQ-SW-001", "derives-from",
                                  "REQ-SYS-005");
    ASSERT(id != TRIPLE_ID_INVALID);
    ASSERT_EQ(triplet_store_count(store), 1);

    triplet_store_destroy(store);
}

static void test_indexes_consistent_after_remove(void)
{
    TripletStore *store = triplet_store_create();

    size_t id1 = triplet_store_add(store,
                                   "REQ-SW-001", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "REQ-SW-002", "derives-from", "REQ-SYS-005");

    /* Remove the first triple by ID; indexes must be updated. */
    triplet_store_remove(store, id1);

    CTripleList by_subj = triplet_store_find_by_subject(store, "REQ-SW-001");
    ASSERT_EQ(by_subj.count, 0);
    triplet_store_list_free(by_subj);

    CTripleList by_obj = triplet_store_find_by_object(store, "REQ-SYS-005");
    ASSERT_EQ(by_obj.count, 1);
    triplet_store_list_free(by_obj);

    triplet_store_destroy(store);
}

static void test_null_safety(void)
{
    /* NULL store must not crash. */
    ASSERT_EQ(triplet_store_count(NULL), 0);
    ASSERT_EQ(triplet_store_add(NULL, "a", "b", "c"), TRIPLE_ID_INVALID);
    ASSERT_EQ(triplet_store_remove(NULL, 0), 0);

    TripletStore *store = triplet_store_create();

    /* NULL arguments must not crash. */
    ASSERT_EQ(triplet_store_add(store, NULL, "b", "c"), TRIPLE_ID_INVALID);
    ASSERT_EQ(triplet_store_add(store, "a", NULL, "c"), TRIPLE_ID_INVALID);
    ASSERT_EQ(triplet_store_add(store, "a", "b", NULL), TRIPLE_ID_INVALID);

    triplet_store_destroy(store);
}

/* -------------------------------------------------------------------------
 * Entry point
 * ---------------------------------------------------------------------- */

int main(void)
{
    test_create_destroy();
    test_add_and_count();
    test_duplicate_rejected();
    test_remove_by_id();
    test_triple_content();
    test_find_by_subject();
    test_find_by_object();
    test_find_by_predicate();
    test_find_all();
    test_remove_by_subject();
    test_remove_by_object();
    test_remove_by_predicate();
    test_clear();
    test_indexes_consistent_after_remove();
    test_null_safety();

    printf("\n%d test(s) run, %d failed.\n", g_tests_run, g_tests_failed);
    return (g_tests_failed > 0) ? 1 : 0;
}
