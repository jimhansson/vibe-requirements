/**
 * @file test_triplet_store.cpp
 * @brief Unit tests for the relation triplet store C++ API using gtest/gmock.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "triplet_store.hpp"

/* -------------------------------------------------------------------------
 * Helper
 * ---------------------------------------------------------------------- */

/** Find a triple in a result vector by predicate and object. */
static bool find_triple(const std::vector<const vibe::Triple*> &list,
                         const std::string &pred, const std::string &obj,
                         bool inferred = false)
{
    for (const auto *t : list) {
        if (t->predicate == pred && t->object == obj &&
            t->inferred == inferred)
            return true;
    }
    return false;
}

/* -------------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */

TEST(TripletStoreTest, DefaultConstructed_IsEmpty)
{
    vibe::TripletStore store;
    EXPECT_EQ(store.count(), 0u);
}

TEST(TripletStoreTest, AddAndCount)
{
    vibe::TripletStore store;

    vibe::TripleId id1 = store.add("REQ-SW-001", "derives-from", "REQ-SYS-005");
    EXPECT_NE(id1, vibe::INVALID_TRIPLE_ID);

    vibe::TripleId id2 = store.add("REQ-SW-001", "implemented-in",
                                   "src/auth/login.c");
    EXPECT_NE(id2, vibe::INVALID_TRIPLE_ID);
    EXPECT_NE(id1, id2);
    EXPECT_EQ(store.count(), 2u);
}

TEST(TripletStoreTest, DuplicateRejected)
{
    vibe::TripletStore store;

    vibe::TripleId id1 = store.add("REQ-SW-001", "derives-from", "REQ-SYS-005");
    EXPECT_NE(id1, vibe::INVALID_TRIPLE_ID);

    /* Exact duplicate must be rejected. */
    vibe::TripleId id2 = store.add("REQ-SW-001", "derives-from", "REQ-SYS-005");
    EXPECT_EQ(id2, vibe::INVALID_TRIPLE_ID);

    EXPECT_EQ(store.count(), 1u);
}

TEST(TripletStoreTest, RemoveById)
{
    vibe::TripletStore store;

    vibe::TripleId id1 = store.add("REQ-SW-001", "derives-from", "REQ-SYS-005");
    vibe::TripleId id2 = store.add("TC-SW-001",  "verifies",     "REQ-SW-001");
    EXPECT_EQ(store.count(), 2u);

    bool removed = store.remove(id1);
    EXPECT_TRUE(removed);
    EXPECT_EQ(store.count(), 1u);

    removed = store.remove(id1);
    EXPECT_FALSE(removed);
    EXPECT_EQ(store.count(), 1u);
    (void)id2;
}

TEST(TripletStoreTest, TripleContent)
{
    vibe::TripletStore store;
    store.add("TC-SW-001", "verifies", "REQ-SW-001");

    auto list = store.find_by_subject("TC-SW-001");
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0]->subject,   "TC-SW-001");
    EXPECT_EQ(list[0]->predicate, "verifies");
    EXPECT_EQ(list[0]->object,    "REQ-SW-001");
}

TEST(TripletStoreTest, FindBySubject)
{
    vibe::TripletStore store;

    store.add("REQ-SW-001", "derives-from",   "REQ-SYS-005");
    store.add("REQ-SW-001", "implemented-in", "src/auth/login.c");
    store.add("TC-SW-001",  "verifies",       "REQ-SW-001");

    auto list = store.find_by_subject("REQ-SW-001");
    EXPECT_EQ(list.size(), 2u);

    /* Unknown subject must return empty list. */
    list = store.find_by_subject("REQ-UNKNOWN");
    EXPECT_EQ(list.size(), 0u);
}

TEST(TripletStoreTest, FindByObject)
{
    vibe::TripletStore store;

    store.add("REQ-SW-001", "derives-from", "REQ-SYS-005");
    store.add("REQ-SW-002", "derives-from", "REQ-SYS-005");
    store.add("TC-SW-001",  "verifies",     "REQ-SW-001");

    auto list = store.find_by_object("REQ-SYS-005");
    EXPECT_EQ(list.size(), 2u);
}

TEST(TripletStoreTest, FindByPredicate)
{
    vibe::TripletStore store;

    store.add("REQ-SW-001", "derives-from", "REQ-SYS-005");
    store.add("REQ-SW-002", "derives-from", "REQ-SYS-006");
    store.add("TC-SW-001",  "verifies",     "REQ-SW-001");

    auto list = store.find_by_predicate("derives-from");
    EXPECT_EQ(list.size(), 2u);
}

TEST(TripletStoreTest, FindAll)
{
    vibe::TripletStore store;

    store.add("REQ-SW-001", "derives-from", "REQ-SYS-005");
    store.add("TC-SW-001",  "verifies",     "REQ-SW-001");

    auto list = store.find_all();
    EXPECT_EQ(list.size(), 2u);
}

TEST(TripletStoreTest, RemoveBySubject)
{
    vibe::TripletStore store;

    store.add("REQ-SW-001", "derives-from",   "REQ-SYS-005");
    store.add("REQ-SW-001", "implemented-in", "src/auth/login.c");
    store.add("TC-SW-001",  "verifies",       "REQ-SW-001");

    size_t n = store.remove_by_subject("REQ-SW-001");
    EXPECT_EQ(n, 2u);
    EXPECT_EQ(store.count(), 1u);

    auto list = store.find_by_subject("TC-SW-001");
    EXPECT_EQ(list.size(), 1u);
}

TEST(TripletStoreTest, RemoveByObject)
{
    vibe::TripletStore store;

    store.add("REQ-SW-001", "derives-from", "REQ-SYS-005");
    store.add("REQ-SW-002", "derives-from", "REQ-SYS-005");
    store.add("TC-SW-001",  "verifies",     "REQ-SW-001");

    size_t n = store.remove_by_object("REQ-SYS-005");
    EXPECT_EQ(n, 2u);
    EXPECT_EQ(store.count(), 1u);
}

TEST(TripletStoreTest, RemoveByPredicate)
{
    vibe::TripletStore store;

    store.add("REQ-SW-001", "derives-from", "REQ-SYS-005");
    store.add("REQ-SW-002", "derives-from", "REQ-SYS-006");
    store.add("TC-SW-001",  "verifies",     "REQ-SW-001");

    size_t n = store.remove_by_predicate("derives-from");
    EXPECT_EQ(n, 2u);
    EXPECT_EQ(store.count(), 1u);
}

TEST(TripletStoreTest, Clear)
{
    vibe::TripletStore store;

    store.add("REQ-SW-001", "derives-from", "REQ-SYS-005");
    store.add("TC-SW-001",  "verifies",     "REQ-SW-001");
    store.clear();

    EXPECT_EQ(store.count(), 0u);
    EXPECT_TRUE(store.find_all().empty());

    /* Adding after clear must work normally. */
    vibe::TripleId id = store.add("REQ-SW-001", "derives-from", "REQ-SYS-005");
    EXPECT_NE(id, vibe::INVALID_TRIPLE_ID);
    EXPECT_EQ(store.count(), 1u);
}

TEST(TripletStoreTest, IndexesConsistentAfterRemove)
{
    vibe::TripletStore store;

    vibe::TripleId id1 = store.add("REQ-SW-001", "derives-from", "REQ-SYS-005");
    store.add("REQ-SW-002", "derives-from", "REQ-SYS-005");

    store.remove(id1);

    auto by_subj = store.find_by_subject("REQ-SW-001");
    EXPECT_EQ(by_subj.size(), 0u);

    auto by_obj = store.find_by_object("REQ-SYS-005");
    EXPECT_EQ(by_obj.size(), 1u);
}

/* -------------------------------------------------------------------------
 * Inferred-inverse tests
 * ---------------------------------------------------------------------- */

TEST(TripletStoreTest, InferredFlagFalseByDefault)
{
    vibe::TripletStore store;

    store.add("REQ-SW-001", "derives-from", "REQ-SYS-005");

    auto list = store.find_by_subject("REQ-SW-001");
    ASSERT_EQ(list.size(), 1u);
    EXPECT_FALSE(list[0]->inferred);
}

TEST(TripletStoreTest, InferInversesKnownRelation)
{
    vibe::TripletStore store;

    store.add("REQ-SW-001", "derives-from", "REQ-SYS-005");
    EXPECT_EQ(store.count(), 1u);

    size_t added = store.infer_inverses();
    EXPECT_EQ(added, 1u);
    EXPECT_EQ(store.count(), 2u);

    auto list = store.find_by_subject("REQ-SYS-005");
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0]->predicate, "derived-to");
    EXPECT_EQ(list[0]->object,    "REQ-SW-001");
    EXPECT_TRUE(list[0]->inferred);

    auto orig = store.find_by_subject("REQ-SW-001");
    ASSERT_EQ(orig.size(), 1u);
    EXPECT_FALSE(orig[0]->inferred);
}

TEST(TripletStoreTest, InferInversesUnknownRelation)
{
    vibe::TripletStore store;

    store.add("REQ-SW-001", "custom-link", "REQ-SYS-005");
    size_t added = store.infer_inverses();
    EXPECT_EQ(added, 0u);
    EXPECT_EQ(store.count(), 1u);
}

TEST(TripletStoreTest, InferInversesSymmetricRelation)
{
    vibe::TripletStore store;

    store.add("REQ-SW-001", "conflicts-with", "REQ-SW-002");
    size_t added = store.infer_inverses();
    EXPECT_EQ(added, 1u);
    EXPECT_EQ(store.count(), 2u);

    auto list = store.find_by_subject("REQ-SW-002");
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0]->predicate, "conflicts-with");
    EXPECT_EQ(list[0]->object,    "REQ-SW-001");
    EXPECT_TRUE(list[0]->inferred);
}

TEST(TripletStoreTest, InferInversesNoDuplicateWhenBothDeclared)
{
    vibe::TripletStore store;

    store.add("REQ-SW-001", "verifies",     "REQ-SYS-005");
    store.add("REQ-SYS-005", "verified-by", "REQ-SW-001");
    EXPECT_EQ(store.count(), 2u);

    size_t added = store.infer_inverses();
    EXPECT_EQ(added, 0u);
    EXPECT_EQ(store.count(), 2u);
}

TEST(TripletStoreTest, InferInversesMultipleRelations)
{
    vibe::TripletStore store;

    store.add("A", "parent",   "B");
    store.add("A", "verifies", "C");
    store.add("A", "custom",   "D"); /* unknown */

    size_t added = store.infer_inverses();
    EXPECT_EQ(added, 2u);
    EXPECT_EQ(store.count(), 5u);
}

TEST(TripletStoreTest, InferInversesIdempotent)
{
    vibe::TripletStore store;

    store.add("REQ-SW-001", "derives-from", "REQ-SYS-005");

    store.infer_inverses();
    size_t count_after_first = store.count();

    size_t added_second = store.infer_inverses();
    EXPECT_EQ(added_second, 0u);
    EXPECT_EQ(store.count(), count_after_first);
}

TEST(TripletStoreTest, AllBuiltInPairsHaveInverse)
{
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
        { "part-of",               "contains"               },
        { "contains",              "part-of"                },
        { "satisfies",             "satisfied-by"           },
        { "satisfied-by",          "satisfies"              },
        { "tests",                 "tested-by"              },
        { "tested-by",             "tests"                  },
    };

    const size_t n = sizeof(pairs) / sizeof(pairs[0]);

    for (size_t i = 0; i < n; ++i) {
        vibe::TripletStore store;
        store.add("A", pairs[i].fwd, "B");
        size_t added = store.infer_inverses();
        EXPECT_EQ(added, 1u) << "pair index " << i
                             << " (" << pairs[i].fwd << ")";

        auto list = store.find_by_subject("B");
        EXPECT_TRUE(find_triple(list, pairs[i].inv, "A", true))
            << "inverse not found for " << pairs[i].fwd;
    }
}

/* -------------------------------------------------------------------------
 * Compaction tests
 * ---------------------------------------------------------------------- */

TEST(TripletStoreTest, SlotCountEqualsCountWhenNoRemovals)
{
    vibe::TripletStore store;

    store.add("A", "verifies", "B");
    store.add("B", "verifies", "C");

    EXPECT_EQ(store.slot_count(), 2u);
    EXPECT_EQ(store.count(),      2u);
}

TEST(TripletStoreTest, SlotCountExceedsCountAfterRemoval)
{
    vibe::TripletStore store;

    for (int i = 0; i < 10; ++i) {
        std::string s = "S" + std::to_string(i);
        store.add(s, "rel", "O");
    }
    EXPECT_EQ(store.slot_count(), 10u);

    for (size_t i = 0; i < 5u; ++i)
        store.remove(i);

    EXPECT_EQ(store.count(),      5u);
    EXPECT_GT(store.slot_count(), 5u);
}

TEST(TripletStoreTest, ExplicitCompactReclaimsDeadSlots)
{
    vibe::TripletStore store;

    for (int i = 0; i < 10; ++i) {
        std::string s = "S" + std::to_string(i);
        store.add(s, "rel", "O");
    }
    for (size_t i = 0; i < 5u; ++i)
        store.remove(i);

    size_t dead_before = store.slot_count() - store.count();
    ASSERT_GT(dead_before, 0u);

    size_t reclaimed = store.compact();
    EXPECT_EQ(reclaimed, dead_before);
    EXPECT_EQ(store.slot_count(), store.count());
}

TEST(TripletStoreTest, CompactPreservesQueryCorrectness)
{
    vibe::TripletStore store;

    store.add("A", "verifies", "B");
    vibe::TripleId id_to_remove = store.add("X", "rel", "Y");
    store.add("C", "verifies", "D");

    store.remove(id_to_remove);
    store.compact();

    auto list_a = store.find_by_subject("A");
    EXPECT_EQ(list_a.size(), 1u);
    EXPECT_EQ(list_a[0]->predicate, "verifies");
    EXPECT_EQ(list_a[0]->object,    "B");

    auto list_c = store.find_by_subject("C");
    EXPECT_EQ(list_c.size(), 1u);

    auto list_x = store.find_by_subject("X");
    EXPECT_EQ(list_x.size(), 0u);

    EXPECT_EQ(store.count(),      2u);
    EXPECT_EQ(store.slot_count(), 2u);
}

TEST(TripletStoreTest, CompactOnEmptyOrNoDeadSlotsIsNoop)
{
    vibe::TripletStore store;

    EXPECT_EQ(store.compact(), 0u);

    store.add("A", "rel", "B");
    EXPECT_EQ(store.compact(), 0u);
    EXPECT_EQ(store.slot_count(), 1u);

    EXPECT_EQ(store.compact(), 0u);
}

TEST(TripletStoreTest, AutoCompactFiresDuringChurn)
{
    vibe::TripletStore store;

    for (int i = 0; i < 20; ++i) {
        std::string s = "CHURN-" + std::to_string(i);
        vibe::TripleId id = store.add(s, "rel", "OBJ");
        store.remove(id);
    }

    EXPECT_LT(store.slot_count(), 20u);
    EXPECT_EQ(store.count(), 0u);
}

TEST(TripletStoreTest, ChurnWithLiveTriplesQueryStillCorrect)
{
    vibe::TripletStore store;

    store.add("SENTINEL", "verifies", "STABLE");

    for (int i = 0; i < 20; ++i) {
        std::string s = "CHURN-" + std::to_string(i);
        vibe::TripleId id = store.add(s, "rel", "OBJ");
        store.remove(id);
    }

    auto list = store.find_by_subject("SENTINEL");
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0]->object, "STABLE");
    EXPECT_FALSE(list[0]->inferred);
}
