/**
 * @file test_main.cpp
 * @brief Unit tests for list_cmd.c — the table-rendering and trace helpers
 *        extracted from main.c.
 *
 * Tests cover:
 *   - build_entity_relation_store(): empty list, entities with links,
 *     inferred inverse inclusion
 *   - list_entities(): empty list message, single/multiple entities,
 *     header and footer presence, title truncation at 48 characters,
 *     entity count in footer
 *   - list_relations(): empty store message, single relation header/row,
 *     inferred vs. declared label, subject/object truncation at 32/48 chars
 *   - cmd_trace_entity(): entity not found, entity found with metadata,
 *     outgoing links, incoming links, no links case
 *   - check_strict_links(): empty store, no bidirectional pairs, fully
 *     bidirectional link (0 warnings), one-sided link (1 warning), multiple
 *     one-sided links
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <functional>

#include "entity.h"

#include "list_cmd.h"
#include "triplet_store.hpp"

using ::testing::HasSubstr;
using ::testing::Not;

/* -------------------------------------------------------------------------
 * Helper — capture stdout to a std::string
 * ---------------------------------------------------------------------- */

/**
 * Redirect stdout to a temporary file for the duration of fn(), then read
 * the captured text back into a std::string.
 */
static std::string capture_stdout(const std::function<void()> &fn)
{
    char path[] = "/tmp/test_main_XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) return "";
    close(fd);

    FILE *old = freopen(path, "w", stdout);
    if (!old) { remove(path); return ""; }

    fn();
    fflush(stdout);

    freopen("/dev/tty", "w", stdout);

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

/**
 * Redirect stderr to a temporary file for the duration of fn(), then read
 * the captured text back into a std::string.
 */
static std::string capture_stderr(const std::function<void()> &fn)
{
    char path[] = "/tmp/test_main_err_XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) return "";
    close(fd);

    FILE *old = freopen(path, "w", stderr);
    if (!old) { remove(path); return ""; }

    fn();
    fflush(stderr);

    freopen("/dev/tty", "w", stderr);

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

/* -------------------------------------------------------------------------
 * Helper — build a minimal Entity
 * ---------------------------------------------------------------------- */

static Entity make_entity(const char *id, const char *title,
                           EntityKind kind,
                           const char *status   = "draft",
                           const char *priority = "must")
{
    Entity e{};
    e.identity.id = id;
    e.identity.title = title;
    e.identity.kind = kind;
    e.lifecycle.status = status;
    e.lifecycle.priority = priority;
    return e;
}

/* =========================================================================
 * Tests — build_entity_relation_store
 * ======================================================================= */

TEST(BuildEntityRelationStoreTest, EmptyListReturnsEmptyStore)
{
    EntityList list;

    vibe::TripletStore *store = build_entity_relation_store(&list);
    ASSERT_NE(store, nullptr);

    auto all = store->find_all();
    EXPECT_EQ((int)all.size(), 0u);

    delete store;
}

TEST(BuildEntityRelationStoreTest, EntityWithLinksPopulatesStore)
{
    EntityList list;

    Entity req = make_entity("REQ-001", "Login", ENTITY_KIND_REQUIREMENT);
    /* Add a declared traceability entry: REQ-001 verified-by TC-001 */
    req.traceability.entries.push_back({"TC-001", "verified-by"});
    list.push_back(req);

    vibe::TripletStore *store = build_entity_relation_store(&list);
    ASSERT_NE(store, nullptr);

    auto by_subj = store->find_by_subject("REQ-001");
    /* At least one declared triple for REQ-001. */
    int declared = 0;
    for (const auto *t : by_subj) {
        if (!t->inferred)
            declared++;
    }
    EXPECT_GE(declared, 1);

    delete store;
}

TEST(BuildEntityRelationStoreTest, InverseRelationsAreInferred)
{
    EntityList list;

    Entity req = make_entity("REQ-001", "Login", ENTITY_KIND_REQUIREMENT);
    req.traceability.entries.push_back({"TC-001", "verified-by"});
    list.push_back(req);

    vibe::TripletStore *store = build_entity_relation_store(&list);
    ASSERT_NE(store, nullptr);

    /* The inverse (TC-001, verifies, REQ-001) should have been inferred. */
    auto by_subj = store->find_by_subject("TC-001");
    int inferred_verifies = 0;
    for (const auto *t : by_subj) {
        if (t->inferred &&
            t->predicate == "verifies" &&
            t->object    == "REQ-001")
            inferred_verifies++;
    }
    EXPECT_EQ(inferred_verifies, 1);

    delete store;
}

TEST(BuildEntityRelationStoreTest, MultipleEntitiesAllLinksPresent)
{
    EntityList list;

    Entity req = make_entity("REQ-010", "Feature", ENTITY_KIND_REQUIREMENT);
    req.traceability.entries.push_back({"TC-010", "verified-by"});

    Entity tc = make_entity("TC-010", "Test feature", ENTITY_KIND_TEST_CASE);
    tc.traceability.entries.push_back({"REQ-010", "verifies"});

    list.push_back(req);
    list.push_back(tc);

    vibe::TripletStore *store = build_entity_relation_store(&list);
    ASSERT_NE(store, nullptr);

    auto all = store->find_all();
    EXPECT_GE((int)all.size(), 2); /* at least the two declared triples */

    delete store;
}

TEST(BuildEntityRelationStoreTest, DocMembershipBecomesPartOfTriple)
{
    EntityList list;

    /* Entity that belongs to a document. */
    Entity req = make_entity("REQ-DOC-001", "Member requirement",
                             ENTITY_KIND_REQUIREMENT);
    req.doc_membership.doc_ids.push_back("SRS-MAIN-001");
    list.push_back(req);

    vibe::TripletStore *store = build_entity_relation_store(&list);
    ASSERT_NE(store, nullptr);

    /* A declared (REQ-DOC-001, part-of, SRS-MAIN-001) triple must exist. */
    auto by_subj = store->find_by_subject("REQ-DOC-001");
    int part_of_declared = 0;
    for (const auto *t : by_subj) {
        if (!t->inferred &&
            t->predicate == "part-of" &&
            t->object    == "SRS-MAIN-001")
            part_of_declared++;
    }
    EXPECT_EQ(part_of_declared, 1);

    /* The inferred inverse (SRS-MAIN-001, contains, REQ-DOC-001) must exist. */
    auto by_doc = store->find_by_subject("SRS-MAIN-001");
    int contains_inferred = 0;
    for (const auto *t : by_doc) {
        if (t->inferred &&
            t->predicate == "contains" &&
            t->object    == "REQ-DOC-001")
            contains_inferred++;
    }
    EXPECT_EQ(contains_inferred, 1);

    delete store;
}

/* =========================================================================
 * Tests — collect_document_entities
 * ======================================================================= */

TEST(CollectDocumentEntitiesTest, MissingDocumentReturnsMinusOne)
{
    EntityList list;

    Entity req = make_entity("REQ-001", "Login", ENTITY_KIND_REQUIREMENT);
    list.push_back(req);

    vibe::TripletStore *store = build_entity_relation_store(&list);
    ASSERT_NE(store, nullptr);

    EntityList collected;

    EXPECT_EQ(collect_document_entities(&list, store, "SRS-001", &collected), -1);
    EXPECT_EQ((int)collected.size(), 0);
    delete store;
}

TEST(CollectDocumentEntitiesTest, NonDocumentEntityReturnsMinusTwo)
{
    EntityList list;

    Entity req = make_entity("REQ-001", "Login", ENTITY_KIND_REQUIREMENT);
    list.push_back(req);

    vibe::TripletStore *store = build_entity_relation_store(&list);
    ASSERT_NE(store, nullptr);

    EntityList collected;

    EXPECT_EQ(collect_document_entities(&list, store, "REQ-001", &collected), -2);
    EXPECT_EQ((int)collected.size(), 0);
    delete store;
}

TEST(CollectDocumentEntitiesTest, CollectsDocumentAndMembers)
{
    EntityList list;

    Entity doc = make_entity("SRS-001", "System Requirements", ENTITY_KIND_DOCUMENT);
    Entity req = make_entity("REQ-001", "Login", ENTITY_KIND_REQUIREMENT);
    Entity tc  = make_entity("TC-001", "Verify login", ENTITY_KIND_TEST_CASE);
    Entity ext = make_entity("EXT-001", "IEC 61508", ENTITY_KIND_EXTERNAL);

    req.doc_membership.doc_ids.push_back("SRS-001");

    tc.traceability.entries.push_back({"SRS-001", "part-of"});

    list.push_back(doc);
    list.push_back(req);
    list.push_back(tc);
    list.push_back(ext);

    vibe::TripletStore *store = build_entity_relation_store(&list);
    ASSERT_NE(store, nullptr);

    EntityList collected;

    ASSERT_EQ(collect_document_entities(&list, store, "SRS-001", &collected), 0);
    ASSERT_EQ((int)collected.size(), 3);
    EXPECT_EQ(collected[0].identity.id, std::string("SRS-001"));
    EXPECT_EQ(collected[1].identity.id, std::string("REQ-001"));
    EXPECT_EQ(collected[2].identity.id, std::string("TC-001"));
    delete store;
}

/* =========================================================================
 * Tests — list_entities
 * ======================================================================= */

TEST(ListEntitiesTest, EmptyListPrintsNoEntitiesFound)
{
    EntityList list;

    std::string out = capture_stdout([&]() {
        list_entities(&list);
    });

    EXPECT_THAT(out, HasSubstr("No entities found."));
}

TEST(ListEntitiesTest, SingleEntityShowsHeaderAndRow)
{
    EntityList list;
    Entity e = make_entity("REQ-001", "Login required",
                            ENTITY_KIND_REQUIREMENT, "approved", "must");
    list.push_back(e);

    std::string out = capture_stdout([&]() {
        list_entities(&list);
    });

    /* Column headers */
    EXPECT_THAT(out, HasSubstr("ID"));
    EXPECT_THAT(out, HasSubstr("Title"));
    EXPECT_THAT(out, HasSubstr("Kind"));
    EXPECT_THAT(out, HasSubstr("Status"));
    EXPECT_THAT(out, HasSubstr("Priority"));

    /* Entity data */
    EXPECT_THAT(out, HasSubstr("REQ-001"));
    EXPECT_THAT(out, HasSubstr("Login required"));
    EXPECT_THAT(out, HasSubstr("requirement"));
    EXPECT_THAT(out, HasSubstr("approved"));
    EXPECT_THAT(out, HasSubstr("must"));

    /* Footer */
    EXPECT_THAT(out, HasSubstr("Total: 1 entity"));
}

TEST(ListEntitiesTest, MultipleEntitiesFooterShowsCount)
{
    EntityList list;
    Entity e1 = make_entity("REQ-001", "Alpha", ENTITY_KIND_REQUIREMENT);
    Entity e2 = make_entity("TC-001",  "Beta",  ENTITY_KIND_TEST_CASE);
    list.push_back(e1);
    list.push_back(e2);

    std::string out = capture_stdout([&]() {
        list_entities(&list);
    });

    EXPECT_THAT(out, HasSubstr("Total: 2 entities"));
    EXPECT_THAT(out, HasSubstr("REQ-001"));
    EXPECT_THAT(out, HasSubstr("TC-001"));
}

TEST(ListEntitiesTest, LongTitleIsTruncated)
{
    EntityList list;
    /* Title is 52 characters — should be truncated to 48 with "..." suffix. */
    Entity e = make_entity("REQ-002",
                            "This title is exactly fifty-two characters long!!",
                            ENTITY_KIND_REQUIREMENT);
    list.push_back(e);

    std::string out = capture_stdout([&]() {
        list_entities(&list);
    });

    EXPECT_THAT(out, HasSubstr("..."));
    /* The full 52-char title must NOT appear verbatim. */
    EXPECT_THAT(out, Not(HasSubstr("This title is exactly fifty-two characters long!!")));
}

TEST(ListEntitiesTest, TitleAtExactly48CharactersIsNotTruncated)
{
    EntityList list;
    /* Title is exactly 48 characters — should not be truncated. */
    const char *title48 = "Exactly48CharsLongTitle1234567890123456789012345";
    ASSERT_EQ(strlen(title48), 48u);
    Entity e = make_entity("REQ-003", title48, ENTITY_KIND_REQUIREMENT);
    list.push_back(e);

    std::string out = capture_stdout([&]() {
        list_entities(&list);
    });

    EXPECT_THAT(out, HasSubstr(title48));
}

TEST(ListEntitiesTest, EntityKindLabelsAppearCorrectly)
{
    EntityList list;
    Entity tc = make_entity("TC-001", "A test", ENTITY_KIND_TEST_CASE);
    list.push_back(tc);

    std::string out = capture_stdout([&]() {
        list_entities(&list);
    });

    EXPECT_THAT(out, HasSubstr("test-case"));
}

TEST(ListEntitiesTest, TableContainsBorderCharacters)
{
    EntityList list;
    Entity e = make_entity("REQ-001", "Title", ENTITY_KIND_REQUIREMENT);
    list.push_back(e);

    std::string out = capture_stdout([&]() {
        list_entities(&list);
    });

    /* The ASCII table uses '+' for corners and '|' for column separators. */
    EXPECT_THAT(out, HasSubstr("+"));
    EXPECT_THAT(out, HasSubstr("|"));
}

/* =========================================================================
 * Tests — list_relations
 * ======================================================================= */

TEST(ListRelationsTest, EmptyStorePrintsNoRelationsFound)
{
    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);

    std::string out = capture_stdout([&]() {
        list_relations(store);
    });

    EXPECT_THAT(out, HasSubstr("No relations found."));

    delete store;
}

TEST(ListRelationsTest, SingleRelationShowsHeaderAndRow)
{
    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);
    store->add( "REQ-001", "verified-by", "TC-001");

    std::string out = capture_stdout([&]() {
        list_relations(store);
    });

    EXPECT_THAT(out, HasSubstr("Subject"));
    EXPECT_THAT(out, HasSubstr("Relation"));
    EXPECT_THAT(out, HasSubstr("Object"));
    EXPECT_THAT(out, HasSubstr("Source"));
    EXPECT_THAT(out, HasSubstr("REQ-001"));
    EXPECT_THAT(out, HasSubstr("verified-by"));
    EXPECT_THAT(out, HasSubstr("TC-001"));

    delete store;
}

TEST(ListRelationsTest, DeclaredRelationMarkedAsDeclared)
{
    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);
    store->add( "REQ-001", "verified-by", "TC-001");

    std::string out = capture_stdout([&]() {
        list_relations(store);
    });

    EXPECT_THAT(out, HasSubstr("declared"));

    delete store;
}

TEST(ListRelationsTest, InferredRelationMarkedAsInferred)
{
    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);
    store->add( "REQ-001", "verified-by", "TC-001");
    store->infer_inverses();

    std::string out = capture_stdout([&]() {
        list_relations(store);
    });

    EXPECT_THAT(out, HasSubstr("inferred"));

    delete store;
}

TEST(ListRelationsTest, FooterShowsTotalRelationCount)
{
    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);
    store->add( "REQ-001", "verified-by", "TC-001");
    store->add( "REQ-002", "verified-by", "TC-002");

    std::string out = capture_stdout([&]() {
        list_relations(store);
    });

    EXPECT_THAT(out, HasSubstr("relation(s)"));

    delete store;
}

TEST(ListRelationsTest, LongSubjectIsTruncated)
{
    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);
    /* Subject is 40 chars — exceeds SUBJ_MAX_DISPLAY (32), should be truncated. */
    store->add(
                      "REQ-THIS-SUBJECT-IS-LONGER-THAN-32-CHARS",
                      "verified-by", "TC-001");

    std::string out = capture_stdout([&]() {
        list_relations(store);
    });

    EXPECT_THAT(out, HasSubstr("..."));

    delete store;
}

TEST(ListRelationsTest, LongObjectIsTruncated)
{
    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);
    /* Object is 55 chars — exceeds OBJ_MAX_DISPLAY (48), should be truncated. */
    store->add( "REQ-001", "verified-by",
                      "TC-THIS-OBJECT-IS-MUCH-LONGER-THAN-48-CHARS-XXXXXXX");

    std::string out = capture_stdout([&]() {
        list_relations(store);
    });

    EXPECT_THAT(out, HasSubstr("..."));

    delete store;
}

/* =========================================================================
 * Tests — cmd_trace_entity
 * ======================================================================= */

TEST(CmdTraceEntityTest, EntityNotFoundShowsNotFoundMessage)
{
    EntityList elist;
    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);

    std::string out = capture_stdout([&]() {
        cmd_trace_entity(&elist, store, "REQ-UNKNOWN");
    });

    EXPECT_THAT(out, HasSubstr("Traceability chain for: REQ-UNKNOWN"));
    EXPECT_THAT(out, HasSubstr("(entity not found in scanned files)"));

    delete store;
}

TEST(CmdTraceEntityTest, EntityFoundShowsMetadata)
{
    EntityList elist;
    Entity req = make_entity("REQ-001", "Login required",
                              ENTITY_KIND_REQUIREMENT, "approved", "must");
    elist.push_back(req);

    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);

    std::string out = capture_stdout([&]() {
        cmd_trace_entity(&elist, store, "REQ-001");
    });

    EXPECT_THAT(out, HasSubstr("Traceability chain for: REQ-001"));
    EXPECT_THAT(out, HasSubstr("requirement"));
    EXPECT_THAT(out, HasSubstr("Login required"));
    EXPECT_THAT(out, HasSubstr("approved"));

    delete store;
}

TEST(CmdTraceEntityTest, NoLinksShowsNoneForBothDirections)
{
    EntityList elist;
    Entity req = make_entity("REQ-001", "Login", ENTITY_KIND_REQUIREMENT);
    elist.push_back(req);

    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);

    std::string out = capture_stdout([&]() {
        cmd_trace_entity(&elist, store, "REQ-001");
    });

    EXPECT_THAT(out, HasSubstr("Outgoing links:"));
    EXPECT_THAT(out, HasSubstr("Incoming links:"));
    /* Both directions should report "(none)" */
    size_t first_none  = out.find("(none)");
    size_t second_none = out.find("(none)", first_none + 1);
    EXPECT_NE(first_none,  std::string::npos);
    EXPECT_NE(second_none, std::string::npos);

    delete store;
}

TEST(CmdTraceEntityTest, OutgoingLinkAppearsInOutput)
{
    EntityList elist;
    Entity req = make_entity("REQ-001", "Login", ENTITY_KIND_REQUIREMENT);
    elist.push_back(req);

    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);
    store->add( "REQ-001", "verified-by", "TC-001");

    std::string out = capture_stdout([&]() {
        cmd_trace_entity(&elist, store, "REQ-001");
    });

    EXPECT_THAT(out, HasSubstr("verified-by"));
    EXPECT_THAT(out, HasSubstr("TC-001"));

    delete store;
}

TEST(CmdTraceEntityTest, IncomingLinkAppearsInOutput)
{
    EntityList elist;
    Entity req = make_entity("REQ-001", "Login", ENTITY_KIND_REQUIREMENT);
    elist.push_back(req);

    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);
    /* TC-002 links TO REQ-001 — incoming for REQ-001 */
    store->add( "TC-002", "verifies", "REQ-001");

    std::string out = capture_stdout([&]() {
        cmd_trace_entity(&elist, store, "REQ-001");
    });

    EXPECT_THAT(out, HasSubstr("TC-002"));
    EXPECT_THAT(out, HasSubstr("verifies"));

    delete store;
}

TEST(CmdTraceEntityTest, EntityWithNoTitleOrStatusSuppressesThoseLines)
{
    EntityList elist;
    /* Entity with empty title and status */
    Entity e{};
    e.identity.id   = "REQ-BARE";
    e.identity.kind = ENTITY_KIND_REQUIREMENT;
    elist.push_back(e);

    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);

    std::string out = capture_stdout([&]() {
        cmd_trace_entity(&elist, store, "REQ-BARE");
    });

    EXPECT_THAT(out, HasSubstr("Traceability chain for: REQ-BARE"));
    /* Title and Status lines should be absent when fields are empty */
    EXPECT_THAT(out, Not(HasSubstr("Title:")));
    EXPECT_THAT(out, Not(HasSubstr("Status:")));

    delete store;
}

/* =========================================================================
 * Tests — check_strict_links
 * ======================================================================= */

TEST(CheckStrictLinksTest, EmptyStoreReturnsZeroWarnings)
{
    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);

    int warnings = check_strict_links(store);
    EXPECT_EQ(warnings, 0);

    delete store;
}

TEST(CheckStrictLinksTest, UnknownRelationIsIgnored)
{
    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);
    /* "annotates" is not in the known-pair table — no warning expected. */
    store->add( "REQ-001", "annotates", "NOTE-001");
    store->infer_inverses();

    int warnings = check_strict_links(store);
    EXPECT_EQ(warnings, 0);

    delete store;
}

TEST(CheckStrictLinksTest, BidirectionalLinkGivesZeroWarnings)
{
    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);
    /* Both directions explicitly declared. */
    store->add( "REQ-001", "verified-by", "TC-001");
    store->add( "TC-001",  "verifies",    "REQ-001");
    store->infer_inverses();

    std::string err = capture_stderr([&]() {
        int w = check_strict_links(store);
        EXPECT_EQ(w, 0);
    });

    EXPECT_THAT(err, Not(HasSubstr("warning")));

    delete store;
}

TEST(CheckStrictLinksTest, OneSidedKnownLinkEmitsWarning)
{
    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);
    /* Only REQ-001 → TC-001 declared; TC-001 → REQ-001 is missing. */
    store->add( "REQ-001", "verified-by", "TC-001");
    store->infer_inverses();

    std::string err = capture_stderr([&]() {
        int w = check_strict_links(store);
        EXPECT_EQ(w, 1);
    });

    EXPECT_THAT(err, HasSubstr("strict-links"));
    EXPECT_THAT(err, HasSubstr("REQ-001"));

    delete store;
}

TEST(CheckStrictLinksTest, MultipleMissingInversesCountsCorrectly)
{
    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);
    /* Two one-sided links. */
    store->add( "REQ-001", "verified-by", "TC-001");
    store->add( "REQ-002", "verified-by", "TC-002");
    store->infer_inverses();

    int warnings = check_strict_links(store);
    EXPECT_EQ(warnings, 2);

    delete store;
}

TEST(CheckStrictLinksTest, ImplementsRelationPairChecked)
{
    vibe::TripletStore *store = new vibe::TripletStore();
    ASSERT_NE(store, nullptr);
    /* One-sided "implements" link — expect a warning. */
    store->add( "STORY-001", "implements", "REQ-001");
    store->infer_inverses();

    std::string err = capture_stderr([&]() {
        int w = check_strict_links(store);
        EXPECT_EQ(w, 1);
    });

    EXPECT_THAT(err, HasSubstr("strict-links"));

    delete store;
}
