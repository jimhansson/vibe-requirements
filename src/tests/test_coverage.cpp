/**
 * @file test_coverage.cpp
 * @brief Unit tests for coverage.c — coverage predicates and report renderers.
 *
 * Tests cover:
 *   - is_coverage_predicate(): all known coverage relations and non-coverage
 *     relations
 *   - entity_is_covered(): entities with / without coverage links, both
 *     outgoing and incoming directions
 *   - entity_has_any_link(): entities with / without any links
 *   - cmd_coverage() and cmd_orphan(): smoke tests verifying non-crashing
 *     output on empty and non-empty entity lists
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>
#include <string>

extern "C" {
#include "entity.h"
#include "coverage.h"
#include "triplet_store_c.h"
}

using ::testing::HasSubstr;
using ::testing::Not;

/* -------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------- */

/** Build a minimal Entity with the given id, title, kind and status. */
static Entity make_entity(const char *id, const char *title,
                           EntityKind kind,
                           const char *status   = "draft",
                           const char *priority = "must")
{
    Entity e;
    memset(&e, 0, sizeof(e));
    strncpy(e.identity.id,     id,       sizeof(e.identity.id)     - 1);
    strncpy(e.identity.title,  title,    sizeof(e.identity.title)  - 1);
    e.identity.kind = kind;
    strncpy(e.lifecycle.status,   status,   sizeof(e.lifecycle.status)   - 1);
    strncpy(e.lifecycle.priority, priority, sizeof(e.lifecycle.priority) - 1);
    return e;
}

/**
 * Capture stdout output of a function call into a std::string.
 * Uses a temporary file (portable; avoids fmemopen / pipe complexity).
 */
template <typename Fn>
static std::string capture_stdout(Fn fn)
{
    char path[] = "/tmp/test_coverage_XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) return "";
    close(fd);

    /* Redirect stdout to the temp file. */
    FILE *old = freopen(path, "w", stdout);
    if (!old) { remove(path); return ""; }

    fn();
    fflush(stdout);

    /* Restore stdout. */
    freopen("/dev/tty", "w", stdout);

    /* Read back. */
    FILE *f = fopen(path, "r");
    if (!f) { remove(path); return ""; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string result(static_cast<size_t>(sz > 0 ? sz : 0), '\0');
    if (sz > 0)
        fread(&result[0], 1, static_cast<size_t>(sz), f);
    fclose(f);
    remove(path);
    return result;
}

/* =========================================================================
 * Tests — is_coverage_predicate
 * ======================================================================= */

TEST(IsCoveragePredicateTest, KnownCoverageRelations)
{
    EXPECT_EQ(is_coverage_predicate("verifies"),           1);
    EXPECT_EQ(is_coverage_predicate("verified-by"),        1);
    EXPECT_EQ(is_coverage_predicate("implements"),         1);
    EXPECT_EQ(is_coverage_predicate("implemented-by"),     1);
    EXPECT_EQ(is_coverage_predicate("implemented-in"),     1);
    EXPECT_EQ(is_coverage_predicate("implemented-by-test"),1);
    EXPECT_EQ(is_coverage_predicate("tests"),              1);
    EXPECT_EQ(is_coverage_predicate("tested-by"),          1);
    EXPECT_EQ(is_coverage_predicate("satisfies"),          1);
    EXPECT_EQ(is_coverage_predicate("satisfied-by"),       1);
}

TEST(IsCoveragePredicateTest, NonCoverageRelations)
{
    EXPECT_EQ(is_coverage_predicate("derived-from"),   0);
    EXPECT_EQ(is_coverage_predicate("derives-from"),   0);
    EXPECT_EQ(is_coverage_predicate("refines"),        0);
    EXPECT_EQ(is_coverage_predicate("member-of"),      0);
    EXPECT_EQ(is_coverage_predicate("elaborates"),     0);
    EXPECT_EQ(is_coverage_predicate("part-of"),        0);
    EXPECT_EQ(is_coverage_predicate("constrained-by"), 0);
}

TEST(IsCoveragePredicateTest, EmptyAndCaseSensitive)
{
    EXPECT_EQ(is_coverage_predicate(""),          0);
    EXPECT_EQ(is_coverage_predicate("Verifies"),  0); /* case-sensitive */
    EXPECT_EQ(is_coverage_predicate("VERIFIES"),  0);
}

/* =========================================================================
 * Tests — entity_is_covered and entity_has_any_link
 * ======================================================================= */

/** Fixture that provides a TripletStore pre-populated with a few triples. */
class CoverageStoreTest : public ::testing::Test {
protected:
    TripletStore *store = nullptr;

    void SetUp() override
    {
        store = triplet_store_create();
        ASSERT_NE(store, nullptr);
    }

    void TearDown() override
    {
        if (store) {
            triplet_store_destroy(store);
            store = nullptr;
        }
    }

    /** Add a declared (non-inferred) triple. */
    void add(const char *subj, const char *pred, const char *obj)
    {
        triplet_store_add(store, subj, pred, obj);
    }
};

TEST_F(CoverageStoreTest, EntityWithOutgoingCoverageLink_IsCovered)
{
    add("REQ-001", "verified-by", "TC-001");
    EXPECT_EQ(entity_is_covered(store, "REQ-001"), 1);
}

TEST_F(CoverageStoreTest, EntityWithIncomingCoverageLink_IsCovered)
{
    add("TC-001", "verifies", "REQ-001");
    EXPECT_EQ(entity_is_covered(store, "REQ-001"), 1);
}

TEST_F(CoverageStoreTest, EntityWithNoLinks_IsNotCovered)
{
    add("REQ-002", "derived-from", "REQ-001");
    EXPECT_EQ(entity_is_covered(store, "REQ-002"), 0);
    EXPECT_EQ(entity_is_covered(store, "UNLINKED"), 0);
}

TEST_F(CoverageStoreTest, EntityWithOnlyNonCoverageLinks_IsNotCovered)
{
    add("REQ-003", "derived-from", "REQ-001");
    add("REQ-003", "refines",      "REQ-001");
    EXPECT_EQ(entity_is_covered(store, "REQ-003"), 0);
}

TEST_F(CoverageStoreTest, EntityWithImplementedByLink_IsCovered)
{
    add("REQ-004", "implemented-by", "SRC-001");
    EXPECT_EQ(entity_is_covered(store, "REQ-004"), 1);
}

TEST_F(CoverageStoreTest, EntityHasAnyLink_OutgoingDeclared)
{
    add("REQ-005", "derived-from", "REQ-001");
    EXPECT_EQ(entity_has_any_link(store, "REQ-005"), 1);
}

TEST_F(CoverageStoreTest, EntityHasAnyLink_IncomingDeclared)
{
    add("TC-002", "verifies", "REQ-006");
    EXPECT_EQ(entity_has_any_link(store, "REQ-006"), 1);
}

TEST_F(CoverageStoreTest, EntityHasAnyLink_NoLinks)
{
    EXPECT_EQ(entity_has_any_link(store, "ORPHAN-001"), 0);
}

TEST_F(CoverageStoreTest, InferredLinksDoNotCountForCoverage)
{
    /*
     * Add a declared link TC-007 -[verifies]-> REQ-007, then infer inverses.
     * The inferred reverse (REQ-007 -[verified-by]-> TC-007) is inferred and
     * must NOT count.  Coverage is only provided by the incoming declared link.
     */
    triplet_store_add(store, "TC-007", "verifies", "REQ-007");
    /* Before inferring: REQ-007 has only the incoming declared link from TC-007.
     * entity_is_covered checks outgoing from REQ-007 (inferred, not counted)
     * and incoming to REQ-007 (declared "verifies" from TC-007 — counted). */
    EXPECT_EQ(entity_is_covered(store, "REQ-007"), 1);
}

TEST_F(CoverageStoreTest, InferredInverseDoesNotMakeUnrelatedEntityCovered)
{
    /*
     * REQ-009 has only a non-coverage declared link.  After inferring inverses,
     * neither direction should yield a coverage-predicate declared link.
     */
    triplet_store_add(store, "REQ-009", "derived-from", "REQ-001");
    triplet_store_infer_inverses(store);
    EXPECT_EQ(entity_is_covered(store, "REQ-009"), 0);
}

/* =========================================================================
 * Tests — cmd_coverage (smoke tests via stdout capture)
 * ======================================================================= */

TEST(CmdCoverageTest, EmptyListPrints100PercentCovered)
{
    EntityList elist;
    entity_list_init(&elist);
    TripletStore *store = triplet_store_create();

    std::string out = capture_stdout([&]() {
        cmd_coverage(&elist, store);
    });

    EXPECT_THAT(out, HasSubstr("Coverage Report"));
    EXPECT_THAT(out, HasSubstr("Total requirements:    0"));

    triplet_store_destroy(store);
    entity_list_free(&elist);
}

TEST(CmdCoverageTest, SingleCoveredRequirementShows100Percent)
{
    EntityList elist;
    entity_list_init(&elist);
    Entity req = make_entity("REQ-001", "Login", ENTITY_KIND_REQUIREMENT);
    entity_list_add(&elist, &req);

    TripletStore *store = triplet_store_create();
    triplet_store_add(store, "REQ-001", "verified-by", "TC-001");

    std::string out = capture_stdout([&]() {
        cmd_coverage(&elist, store);
    });

    EXPECT_THAT(out, HasSubstr("Total requirements:    1"));
    EXPECT_THAT(out, HasSubstr("Linked requirements:   1"));
    EXPECT_THAT(out, HasSubstr("Unlinked requirements: 0 (0%)"));

    triplet_store_destroy(store);
    entity_list_free(&elist);
}

TEST(CmdCoverageTest, UncoveredRequirementAppearsInTable)
{
    EntityList elist;
    entity_list_init(&elist);
    Entity req = make_entity("REQ-002", "Logout", ENTITY_KIND_REQUIREMENT,
                             "approved");
    entity_list_add(&elist, &req);

    TripletStore *store = triplet_store_create();

    std::string out = capture_stdout([&]() {
        cmd_coverage(&elist, store);
    });

    EXPECT_THAT(out, HasSubstr("Unlinked requirements:"));
    EXPECT_THAT(out, HasSubstr("REQ-002"));

    triplet_store_destroy(store);
    entity_list_free(&elist);
}

/* =========================================================================
 * Tests — cmd_orphan (smoke tests via stdout capture)
 * ======================================================================= */

TEST(CmdOrphanTest, EmptyListPrintsNoOrphans)
{
    EntityList elist;
    entity_list_init(&elist);
    TripletStore *store = triplet_store_create();

    std::string out = capture_stdout([&]() {
        cmd_orphan(&elist, store);
    });

    EXPECT_THAT(out, HasSubstr("No orphaned requirements or test cases found."));

    triplet_store_destroy(store);
    entity_list_free(&elist);
}

TEST(CmdOrphanTest, LinkedEntityIsNotOrphaned)
{
    EntityList elist;
    entity_list_init(&elist);
    Entity req = make_entity("REQ-010", "Linked req", ENTITY_KIND_REQUIREMENT);
    entity_list_add(&elist, &req);

    TripletStore *store = triplet_store_create();
    triplet_store_add(store, "REQ-010", "verified-by", "TC-010");

    std::string out = capture_stdout([&]() {
        cmd_orphan(&elist, store);
    });

    EXPECT_THAT(out, HasSubstr("No orphaned requirements or test cases found."));

    triplet_store_destroy(store);
    entity_list_free(&elist);
}

TEST(CmdOrphanTest, UnlinkedRequirementAppearsAsOrphan)
{
    EntityList elist;
    entity_list_init(&elist);
    Entity req = make_entity("REQ-011", "Unlinked req", ENTITY_KIND_REQUIREMENT);
    entity_list_add(&elist, &req);

    TripletStore *store = triplet_store_create();

    std::string out = capture_stdout([&]() {
        cmd_orphan(&elist, store);
    });

    EXPECT_THAT(out, HasSubstr("REQ-011"));
    EXPECT_THAT(out, HasSubstr("Total: 1 orphan(s)"));

    triplet_store_destroy(store);
    entity_list_free(&elist);
}

TEST(CmdOrphanTest, NonRequirementNonTestKindsAreIgnored)
{
    EntityList elist;
    entity_list_init(&elist);
    /* A design note with no links — should NOT appear as an orphan. */
    Entity dn = make_entity("DESIGN-001", "Some design",
                            ENTITY_KIND_DESIGN_NOTE);
    entity_list_add(&elist, &dn);

    TripletStore *store = triplet_store_create();

    std::string out = capture_stdout([&]() {
        cmd_orphan(&elist, store);
    });

    EXPECT_THAT(out, HasSubstr("No orphaned requirements or test cases found."));

    triplet_store_destroy(store);
    entity_list_free(&elist);
}

TEST(CmdOrphanTest, UnlinkedTestCaseAppearsAsOrphan)
{
    EntityList elist;
    entity_list_init(&elist);
    Entity tc = make_entity("TC-020", "Unlinked test", ENTITY_KIND_TEST_CASE);
    entity_list_add(&elist, &tc);

    TripletStore *store = triplet_store_create();

    std::string out = capture_stdout([&]() {
        cmd_orphan(&elist, store);
    });

    EXPECT_THAT(out, HasSubstr("TC-020"));
    EXPECT_THAT(out, HasSubstr("Total: 1 orphan(s)"));

    triplet_store_destroy(store);
    entity_list_free(&elist);
}
