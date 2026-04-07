/**
 * @file test_triplet_store.cpp
 * @brief Unit tests for the relation triplet store C API using gtest/gmock.
 *
 * Written in C++ using the GoogleTest framework to verify that the C API
 * header is self-contained and callable from C++ translation units.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "triplet_store_c.h"  /* already has extern "C" guards */

/* -------------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */

TEST(TripletStoreTest, CreateDestroy)
{
    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);
    EXPECT_EQ(triplet_store_count(store), 0u);
    triplet_store_destroy(store);
}

TEST(TripletStoreTest, AddAndCount)
{
    TripletStore *store = triplet_store_create();

    size_t id1 = triplet_store_add(store,
                                   "REQ-SW-001", "derives-from", "REQ-SYS-005");
    EXPECT_NE(id1, TRIPLE_ID_INVALID);

    size_t id2 = triplet_store_add(store,
                                   "REQ-SW-001", "implemented-in",
                                   "src/auth/login.c");
    EXPECT_NE(id2, TRIPLE_ID_INVALID);
    EXPECT_NE(id1, id2);
    EXPECT_EQ(triplet_store_count(store), 2u);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, DuplicateRejected)
{
    TripletStore *store = triplet_store_create();

    size_t id1 = triplet_store_add(store,
                                   "REQ-SW-001", "derives-from", "REQ-SYS-005");
    EXPECT_NE(id1, TRIPLE_ID_INVALID);

    /* Exact duplicate must be rejected. */
    size_t id2 = triplet_store_add(store,
                                   "REQ-SW-001", "derives-from", "REQ-SYS-005");
    EXPECT_EQ(id2, TRIPLE_ID_INVALID);

    /* Count must remain 1. */
    EXPECT_EQ(triplet_store_count(store), 1u);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, RemoveById)
{
    TripletStore *store = triplet_store_create();

    size_t id1 = triplet_store_add(store,
                                   "REQ-SW-001", "derives-from", "REQ-SYS-005");
    size_t id2 = triplet_store_add(store,
                                   "TC-SW-001",  "verifies",     "REQ-SW-001");
    EXPECT_EQ(triplet_store_count(store), 2u);

    int removed = triplet_store_remove(store, id1);
    EXPECT_EQ(removed, 1);
    EXPECT_EQ(triplet_store_count(store), 1u);

    /* Removing the same ID again must return 0. */
    removed = triplet_store_remove(store, id1);
    EXPECT_EQ(removed, 0);
    EXPECT_EQ(triplet_store_count(store), 1u);

    triplet_store_destroy(store);
    (void)id2;
}

TEST(TripletStoreTest, TripleContent)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "TC-SW-001", "verifies", "REQ-SW-001");

    CTripleList list = triplet_store_find_by_subject(store, "TC-SW-001");
    ASSERT_EQ(list.count, 1u);
    EXPECT_STREQ(list.triples[0].subject,   "TC-SW-001");
    EXPECT_STREQ(list.triples[0].predicate, "verifies");
    EXPECT_STREQ(list.triples[0].object,    "REQ-SW-001");

    triplet_store_list_free(list);
    triplet_store_destroy(store);
}

TEST(TripletStoreTest, FindBySubject)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from",   "REQ-SYS-005");
    triplet_store_add(store, "REQ-SW-001", "implemented-in", "src/auth/login.c");
    triplet_store_add(store, "TC-SW-001",  "verifies",       "REQ-SW-001");

    CTripleList list = triplet_store_find_by_subject(store, "REQ-SW-001");
    EXPECT_EQ(list.count, 2u);
    triplet_store_list_free(list);

    /* Unknown subject must return empty list. */
    list = triplet_store_find_by_subject(store, "REQ-UNKNOWN");
    EXPECT_EQ(list.count, 0u);
    triplet_store_list_free(list);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, FindByObject)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "REQ-SW-002", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "TC-SW-001",  "verifies",     "REQ-SW-001");

    CTripleList list = triplet_store_find_by_object(store, "REQ-SYS-005");
    EXPECT_EQ(list.count, 2u);
    triplet_store_list_free(list);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, FindByPredicate)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "REQ-SW-002", "derives-from", "REQ-SYS-006");
    triplet_store_add(store, "TC-SW-001",  "verifies",     "REQ-SW-001");

    CTripleList list = triplet_store_find_by_predicate(store, "derives-from");
    EXPECT_EQ(list.count, 2u);
    triplet_store_list_free(list);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, FindAll)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "TC-SW-001",  "verifies",     "REQ-SW-001");

    CTripleList list = triplet_store_find_all(store);
    EXPECT_EQ(list.count, 2u);
    triplet_store_list_free(list);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, RemoveBySubject)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from",   "REQ-SYS-005");
    triplet_store_add(store, "REQ-SW-001", "implemented-in", "src/auth/login.c");
    triplet_store_add(store, "TC-SW-001",  "verifies",       "REQ-SW-001");

    size_t n = triplet_store_remove_by_subject(store, "REQ-SW-001");
    EXPECT_EQ(n, 2u);
    EXPECT_EQ(triplet_store_count(store), 1u);

    /* The remaining triple must still be queryable. */
    CTripleList list = triplet_store_find_by_subject(store, "TC-SW-001");
    EXPECT_EQ(list.count, 1u);
    triplet_store_list_free(list);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, RemoveByObject)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "REQ-SW-002", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "TC-SW-001",  "verifies",     "REQ-SW-001");

    size_t n = triplet_store_remove_by_object(store, "REQ-SYS-005");
    EXPECT_EQ(n, 2u);
    EXPECT_EQ(triplet_store_count(store), 1u);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, RemoveByPredicate)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "REQ-SW-002", "derives-from", "REQ-SYS-006");
    triplet_store_add(store, "TC-SW-001",  "verifies",     "REQ-SW-001");

    size_t n = triplet_store_remove_by_predicate(store, "derives-from");
    EXPECT_EQ(n, 2u);
    EXPECT_EQ(triplet_store_count(store), 1u);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, Clear)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "TC-SW-001",  "verifies",     "REQ-SW-001");
    triplet_store_clear(store);

    EXPECT_EQ(triplet_store_count(store), 0u);

    CTripleList list = triplet_store_find_all(store);
    EXPECT_EQ(list.count, 0u);
    triplet_store_list_free(list);

    /* Adding after clear must work normally. */
    size_t id = triplet_store_add(store, "REQ-SW-001", "derives-from",
                                  "REQ-SYS-005");
    EXPECT_NE(id, TRIPLE_ID_INVALID);
    EXPECT_EQ(triplet_store_count(store), 1u);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, IndexesConsistentAfterRemove)
{
    TripletStore *store = triplet_store_create();

    size_t id1 = triplet_store_add(store,
                                   "REQ-SW-001", "derives-from", "REQ-SYS-005");
    triplet_store_add(store, "REQ-SW-002", "derives-from", "REQ-SYS-005");

    /* Remove the first triple by ID; indexes must be updated. */
    triplet_store_remove(store, id1);

    CTripleList by_subj = triplet_store_find_by_subject(store, "REQ-SW-001");
    EXPECT_EQ(by_subj.count, 0u);
    triplet_store_list_free(by_subj);

    CTripleList by_obj = triplet_store_find_by_object(store, "REQ-SYS-005");
    EXPECT_EQ(by_obj.count, 1u);
    triplet_store_list_free(by_obj);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, NullSafety)
{
    /* NULL store must not crash. */
    EXPECT_EQ(triplet_store_count(nullptr), 0u);
    EXPECT_EQ(triplet_store_add(nullptr, "a", "b", "c"), TRIPLE_ID_INVALID);
    EXPECT_EQ(triplet_store_remove(nullptr, 0), 0);

    TripletStore *store = triplet_store_create();

    /* NULL arguments must not crash. */
    EXPECT_EQ(triplet_store_add(store, nullptr, "b", "c"), TRIPLE_ID_INVALID);
    EXPECT_EQ(triplet_store_add(store, "a", nullptr, "c"), TRIPLE_ID_INVALID);
    EXPECT_EQ(triplet_store_add(store, "a", "b", nullptr), TRIPLE_ID_INVALID);

    triplet_store_destroy(store);
}
