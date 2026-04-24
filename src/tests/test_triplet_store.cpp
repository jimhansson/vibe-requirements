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

TEST(TripletStoreTest, DestroyNullSafe)
{
    /* triplet_store_destroy(NULL) must be a silent no-op. */
    triplet_store_destroy(nullptr);
}

TEST(TripletStoreTest, NullSafetyRemoveBy)
{
    TripletStore *store = triplet_store_create();

    /* NULL store returns 0. */
    EXPECT_EQ(triplet_store_remove_by_subject(nullptr, "s"), 0u);
    EXPECT_EQ(triplet_store_remove_by_object(nullptr, "o"), 0u);
    EXPECT_EQ(triplet_store_remove_by_predicate(nullptr, "p"), 0u);

    /* NULL key returns 0. */
    EXPECT_EQ(triplet_store_remove_by_subject(store, nullptr), 0u);
    EXPECT_EQ(triplet_store_remove_by_object(store, nullptr), 0u);
    EXPECT_EQ(triplet_store_remove_by_predicate(store, nullptr), 0u);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, NullSafetyClear)
{
    /* triplet_store_clear(NULL) must be a silent no-op. */
    triplet_store_clear(nullptr);
}

TEST(TripletStoreTest, NullSafetyQueryFunctions)
{
    TripletStore *store = triplet_store_create();

    /* NULL store returns empty list. */
    CTripleList l1 = triplet_store_find_by_subject(nullptr, "s");
    EXPECT_EQ(l1.count, 0u);
    EXPECT_EQ(l1.triples, nullptr);
    triplet_store_list_free(l1);

    CTripleList l2 = triplet_store_find_by_object(nullptr, "o");
    EXPECT_EQ(l2.count, 0u);
    triplet_store_list_free(l2);

    CTripleList l3 = triplet_store_find_by_predicate(nullptr, "p");
    EXPECT_EQ(l3.count, 0u);
    triplet_store_list_free(l3);

    CTripleList l4 = triplet_store_find_all(nullptr);
    EXPECT_EQ(l4.count, 0u);
    triplet_store_list_free(l4);

    /* NULL key returns empty list. */
    CTripleList l5 = triplet_store_find_by_subject(store, nullptr);
    EXPECT_EQ(l5.count, 0u);
    triplet_store_list_free(l5);

    CTripleList l6 = triplet_store_find_by_object(store, nullptr);
    EXPECT_EQ(l6.count, 0u);
    triplet_store_list_free(l6);

    CTripleList l7 = triplet_store_find_by_predicate(store, nullptr);
    EXPECT_EQ(l7.count, 0u);
    triplet_store_list_free(l7);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, ListFreeEmptyListIsNoOp)
{
    /* Freeing a zero-initialised list must be a silent no-op. */
    CTripleList empty{};
    triplet_store_list_free(empty);

    /* Freeing a list with triples == NULL and count == 0 must also be safe. */
    CTripleList zero_list;
    zero_list.triples = nullptr;
    zero_list.count   = 0;
    triplet_store_list_free(zero_list);
}

/* -------------------------------------------------------------------------
 * Inferred-inverse tests
 * ---------------------------------------------------------------------- */

TEST(TripletStoreTest, InferredFlagFalseByDefault)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from", "REQ-SYS-005");

    CTripleList list = triplet_store_find_by_subject(store, "REQ-SW-001");
    ASSERT_EQ(list.count, 1u);
    EXPECT_EQ(list.triples[0].inferred, 0);

    triplet_store_list_free(list);
    triplet_store_destroy(store);
}

TEST(TripletStoreTest, InferInversesKnownRelation)
{
    TripletStore *store = triplet_store_create();

    /* User declares one direction. */
    triplet_store_add(store, "REQ-SW-001", "derives-from", "REQ-SYS-005");
    EXPECT_EQ(triplet_store_count(store), 1u);

    /* After inference the inverse should be added. */
    size_t added = triplet_store_infer_inverses(store);
    EXPECT_EQ(added, 1u);
    EXPECT_EQ(triplet_store_count(store), 2u);

    /* The inferred triple must be (REQ-SYS-005, derived-to, REQ-SW-001). */
    CTripleList list = triplet_store_find_by_subject(store, "REQ-SYS-005");
    ASSERT_EQ(list.count, 1u);
    EXPECT_STREQ(list.triples[0].predicate, "derived-to");
    EXPECT_STREQ(list.triples[0].object,    "REQ-SW-001");
    EXPECT_EQ(list.triples[0].inferred, 1);
    triplet_store_list_free(list);

    /* The original triple must still be marked as declared (not inferred). */
    CTripleList orig = triplet_store_find_by_subject(store, "REQ-SW-001");
    ASSERT_EQ(orig.count, 1u);
    EXPECT_EQ(orig.triples[0].inferred, 0);
    triplet_store_list_free(orig);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, InferInversesUnknownRelation)
{
    TripletStore *store = triplet_store_create();

    /* Custom / unknown relation — no inverse should be added. */
    triplet_store_add(store, "REQ-SW-001", "custom-link", "REQ-SYS-005");
    size_t added = triplet_store_infer_inverses(store);
    EXPECT_EQ(added, 0u);
    EXPECT_EQ(triplet_store_count(store), 1u);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, InferInversesSymmetricRelation)
{
    TripletStore *store = triplet_store_create();

    /* "conflicts-with" is symmetric: inverse is also "conflicts-with". */
    triplet_store_add(store, "REQ-SW-001", "conflicts-with", "REQ-SW-002");
    size_t added = triplet_store_infer_inverses(store);
    EXPECT_EQ(added, 1u);
    EXPECT_EQ(triplet_store_count(store), 2u);

    CTripleList list = triplet_store_find_by_subject(store, "REQ-SW-002");
    ASSERT_EQ(list.count, 1u);
    EXPECT_STREQ(list.triples[0].predicate, "conflicts-with");
    EXPECT_STREQ(list.triples[0].object,    "REQ-SW-001");
    EXPECT_EQ(list.triples[0].inferred, 1);
    triplet_store_list_free(list);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, InferInversesNoDuplicateWhenBothDeclared)
{
    TripletStore *store = triplet_store_create();

    /* User declares both directions explicitly. */
    triplet_store_add(store, "REQ-SW-001", "verifies",     "REQ-SYS-005");
    triplet_store_add(store, "REQ-SYS-005", "verified-by", "REQ-SW-001");
    EXPECT_EQ(triplet_store_count(store), 2u);

    /* Inference must not add a duplicate. */
    size_t added = triplet_store_infer_inverses(store);
    EXPECT_EQ(added, 0u);
    EXPECT_EQ(triplet_store_count(store), 2u);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, InferInversesMultipleRelations)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "A", "parent",    "B");
    triplet_store_add(store, "A", "verifies",  "C");
    triplet_store_add(store, "A", "custom",    "D"); /* unknown */

    size_t added = triplet_store_infer_inverses(store);
    EXPECT_EQ(added, 2u); /* only 2 known inverses */
    EXPECT_EQ(triplet_store_count(store), 5u); /* 3 declared + 2 inferred */

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, InferInversesIdempotent)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "REQ-SW-001", "derives-from", "REQ-SYS-005");

    triplet_store_infer_inverses(store);
    size_t count_after_first = triplet_store_count(store);

    /* Calling a second time must not add more triples. */
    size_t added_second = triplet_store_infer_inverses(store);
    EXPECT_EQ(added_second, 0u);
    EXPECT_EQ(triplet_store_count(store), count_after_first);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, InferInversesNullSafe)
{
    EXPECT_EQ(triplet_store_infer_inverses(nullptr), 0u);
}

TEST(TripletStoreTest, AllBuiltInPairsHaveInverse)
{
    /* Verify that every pair in the built-in table round-trips correctly
     * by checking inference for each known forward relation. */
    TripletStore *store = triplet_store_create();

    static const struct { const char *fwd; const char *inv; } pairs[] = {
        { "derives-from",          "derived-to"             },
        { "derived-to",            "derives-from"           },
        { "parent",                "child"                  },
        { "child",                 "parent"                 },
        { "verifies",              "verified-by"            },
        { "verified-by",           "verifies"               },
        { "implements",            "implemented-by"         },
        { "implemented-by",        "implements"             },
        { "implemented-in",        "implemented-by-artefact"},
        { "implemented-by-artefact","implemented-in"        },
        { "conflicts-with",        "conflicts-with"         },
        { "refines",               "refined-by"             },
        { "refined-by",            "refines"                },
        { "traces-to",             "traced-from"            },
        { "traced-from",           "traces-to"              },
    };

    const size_t n = sizeof(pairs) / sizeof(pairs[0]);

    for (size_t i = 0; i < n; ++i) {
        triplet_store_clear(store);
        triplet_store_add(store, "A", pairs[i].fwd, "B");
        size_t added = triplet_store_infer_inverses(store);
        EXPECT_EQ(added, 1u) << "pair index " << i
                             << " (" << pairs[i].fwd << ")";

        CTripleList list = triplet_store_find_by_subject(store, "B");
        bool found = false;
        for (size_t j = 0; j < list.count; ++j) {
            if (std::string(list.triples[j].predicate) == pairs[i].inv &&
                std::string(list.triples[j].object) == "A" &&
                list.triples[j].inferred) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "inverse not found for " << pairs[i].fwd;
        triplet_store_list_free(list);
    }

    triplet_store_destroy(store);
}

/* -------------------------------------------------------------------------
 * Compaction tests
 * ---------------------------------------------------------------------- */

TEST(TripletStoreTest, SlotCountEqualsCountWhenNoRemovals)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "A", "verifies", "B");
    triplet_store_add(store, "B", "verifies", "C");

    /* No removals: slots used == active count. */
    EXPECT_EQ(triplet_store_slot_count(store), 2u);
    EXPECT_EQ(triplet_store_count(store),      2u);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, SlotCountExceedsCountAfterRemoval)
{
    TripletStore *store = triplet_store_create();

    /* Add enough triples so they fall below the auto-compact threshold. */
    for (int i = 0; i < 10; ++i) {
        std::string s = "S" + std::to_string(i);
        triplet_store_add(store, s.c_str(), "rel", "O");
    }
    EXPECT_EQ(triplet_store_slot_count(store), 10u);

    /* Remove half; count drops but slots remain (below auto-compact threshold). */
    for (int i = 0; i < 5; ++i) {
        triplet_store_remove(store, static_cast<size_t>(i));
    }

    EXPECT_EQ(triplet_store_count(store),      5u);
    EXPECT_GT(triplet_store_slot_count(store), 5u);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, ExplicitCompactReclaimsDeadSlots)
{
    TripletStore *store = triplet_store_create();

    /* Add and then remove some triples, staying below auto-compact threshold. */
    for (int i = 0; i < 10; ++i) {
        std::string s = "S" + std::to_string(i);
        triplet_store_add(store, s.c_str(), "rel", "O");
    }
    for (int i = 0; i < 5; ++i) {
        triplet_store_remove(store, static_cast<size_t>(i));
    }

    size_t dead_before = triplet_store_slot_count(store) - triplet_store_count(store);
    ASSERT_GT(dead_before, 0u);

    size_t reclaimed = triplet_store_compact(store);
    EXPECT_EQ(reclaimed, dead_before);
    EXPECT_EQ(triplet_store_slot_count(store), triplet_store_count(store));

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, CompactPreservesQueryCorrectness)
{
    TripletStore *store = triplet_store_create();

    triplet_store_add(store, "A", "verifies", "B");
    size_t id_to_remove = triplet_store_add(store, "X", "rel", "Y");
    triplet_store_add(store, "C", "verifies", "D");

    triplet_store_remove(store, id_to_remove);

    triplet_store_compact(store);

    /* After compact, surviving triples must still be found. */
    CTripleList list_a = triplet_store_find_by_subject(store, "A");
    EXPECT_EQ(list_a.count, 1u);
    EXPECT_STREQ(list_a.triples[0].predicate, "verifies");
    EXPECT_STREQ(list_a.triples[0].object,    "B");
    triplet_store_list_free(list_a);

    CTripleList list_c = triplet_store_find_by_subject(store, "C");
    EXPECT_EQ(list_c.count, 1u);
    triplet_store_list_free(list_c);

    /* Removed triple must not appear. */
    CTripleList list_x = triplet_store_find_by_subject(store, "X");
    EXPECT_EQ(list_x.count, 0u);
    triplet_store_list_free(list_x);

    EXPECT_EQ(triplet_store_count(store), 2u);
    EXPECT_EQ(triplet_store_slot_count(store), 2u);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, CompactOnEmptyOrNoDeadSlotsIsNoop)
{
    TripletStore *store = triplet_store_create();

    /* No triples at all. */
    EXPECT_EQ(triplet_store_compact(store), 0u);

    triplet_store_add(store, "A", "rel", "B");

    /* One triple, no removals. */
    EXPECT_EQ(triplet_store_compact(store), 0u);
    EXPECT_EQ(triplet_store_slot_count(store), 1u);

    /* Second compact is still a no-op. */
    EXPECT_EQ(triplet_store_compact(store), 0u);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, CompactNullSafe)
{
    EXPECT_EQ(triplet_store_compact(nullptr), 0u);
    EXPECT_EQ(triplet_store_slot_count(nullptr), 0u);
}

TEST(TripletStoreTest, AutoCompactFiresDuringChurn)
{
    TripletStore *store = triplet_store_create();

    /* Repeatedly add one triple and remove it, creating dead slots.
     * The auto-compact threshold fires when dead >= k_compact_threshold
     * AND dead >= count.  After each remove, count == 0 so the condition
     * dead >= count is always satisfied once the dead threshold is reached.
     *
     * We do 20 add/remove cycles; somewhere before cycle 20 the store should
     * auto-compact and keep slot_count bounded. */
    for (int i = 0; i < 20; ++i) {
        std::string s = "CHURN-" + std::to_string(i);
        size_t id = triplet_store_add(store, s.c_str(), "rel", "OBJ");
        triplet_store_remove(store, id);
    }

    /* After auto-compaction the slot vector must not grow without bound;
     * it should be much smaller than 20. */
    EXPECT_LT(triplet_store_slot_count(store), 20u);
    EXPECT_EQ(triplet_store_count(store), 0u);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, ChurnWithLiveTriplesQueryStillCorrect)
{
    TripletStore *store = triplet_store_create();

    /* Add a "sentinel" triple that must survive all churn. */
    triplet_store_add(store, "SENTINEL", "rel", "OBJ");

    /* Generate enough churn to trigger auto-compaction at least once. */
    for (int i = 0; i < 40; ++i) {
        std::string s = "TMP-" + std::to_string(i);
        size_t id = triplet_store_add(store, s.c_str(), "rel", "OBJ");
        triplet_store_remove(store, id);
    }

    /* Sentinel must still be queryable. */
    CTripleList list = triplet_store_find_by_subject(store, "SENTINEL");
    EXPECT_EQ(list.count, 1u);
    if (list.count > 0) {
        EXPECT_STREQ(list.triples[0].predicate, "rel");
        EXPECT_STREQ(list.triples[0].object,    "OBJ");
    }
    triplet_store_list_free(list);

    EXPECT_EQ(triplet_store_count(store), 1u);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, SlotCountAfterClearIsZero)
{
    TripletStore *store = triplet_store_create();

    for (int i = 0; i < 10; ++i) {
        std::string s = "S" + std::to_string(i);
        triplet_store_add(store, s.c_str(), "rel", "O");
    }

    triplet_store_clear(store);

    EXPECT_EQ(triplet_store_count(store),     0u);
    EXPECT_EQ(triplet_store_slot_count(store), 0u);

    triplet_store_destroy(store);
}

TEST(TripletStoreTest, CompactAfterBulkRemoveThenAddWorks)
{
    TripletStore *store = triplet_store_create();

    /* Add 5 triples, remove 4, compact, then add more. */
    for (int i = 0; i < 5; ++i) {
        std::string s = "X" + std::to_string(i);
        triplet_store_add(store, s.c_str(), "rel", "O");
    }
    for (int i = 0; i < 4; ++i)
        triplet_store_remove(store, static_cast<size_t>(i));

    triplet_store_compact(store);

    /* Now add new triples; they should get low consecutive IDs. */
    size_t new_id = triplet_store_add(store, "NEW-A", "rel", "O");
    EXPECT_NE(new_id, TRIPLE_ID_INVALID);
    /* slot_count should be count (compact), plus 1 for the new one */
    EXPECT_EQ(triplet_store_slot_count(store), triplet_store_count(store));

    CTripleList list = triplet_store_find_by_subject(store, "NEW-A");
    EXPECT_EQ(list.count, 1u);
    triplet_store_list_free(list);

    triplet_store_destroy(store);
}

