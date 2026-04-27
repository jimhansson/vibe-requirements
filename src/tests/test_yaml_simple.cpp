/**
 * @file test_yaml_simple.cpp
 * @brief Unit tests for yaml_simple.c — single and multi-document YAML
 *        parsing, using gtest/gmock.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>

#include "yaml_simple.h"
#include "triplet_store_c.h"

/* -------------------------------------------------------------------------
 * Helpers — write temporary YAML files into /tmp
 * ---------------------------------------------------------------------- */

static const char *write_yaml(const char *filename, const char *content)
{
    static char path[512];
    snprintf(path, sizeof(path), "/tmp/%s", filename);
    FILE *f = fopen(path, "w");
    if (!f)
        return nullptr;
    fputs(content, f);
    fclose(f);
    return path;
}

/* -------------------------------------------------------------------------
 * Tests — yaml_parse_entities (multi-document)
 * ---------------------------------------------------------------------- */

TEST(YamlSimpleTest, SingleDocument)
{
    const char *path = write_yaml("test_single.yaml",
        "id: REQ-001\n"
        "title: Single document\n"
        "type: functional\n"
        "status: draft\n"
        "priority: must\n");
    ASSERT_NE(path, nullptr);

    EntityList list;

    int rc = yaml_parse_entities(path, &list);
    EXPECT_EQ(rc, 1);
    EXPECT_EQ((int)list.size(), 1);
    EXPECT_EQ(list[0].identity.id, std::string("REQ-001"));
    EXPECT_EQ(list[0].identity.title, std::string("Single document"));
    EXPECT_EQ(list[0].lifecycle.status, std::string("draft"));
    EXPECT_EQ(list[0].lifecycle.priority, std::string("must"));
}

TEST(YamlSimpleTest, MultiDocument)
{
    const char *path = write_yaml("test_multi.yaml",
        "id: REQ-001\n"
        "title: First requirement\n"
        "type: functional\n"
        "status: draft\n"
        "priority: must\n"
        "---\n"
        "id: REQ-002\n"
        "title: Second requirement\n"
        "type: non-functional\n"
        "status: approved\n"
        "priority: should\n"
        "---\n"
        "id: REQ-003\n"
        "title: Third requirement\n"
        "type: functional\n"
        "status: draft\n"
        "priority: could\n");
    ASSERT_NE(path, nullptr);

    EntityList list;

    int rc = yaml_parse_entities(path, &list);
    EXPECT_EQ(rc, 3);
    EXPECT_EQ((int)list.size(), 3);

    EXPECT_EQ(list[0].identity.id, std::string("REQ-001"));
    EXPECT_EQ(list[0].identity.title, std::string("First requirement"));

    EXPECT_EQ(list[1].identity.id, std::string("REQ-002"));
    EXPECT_EQ(list[1].identity.title, std::string("Second requirement"));

    EXPECT_EQ(list[2].identity.id, std::string("REQ-003"));
    EXPECT_EQ(list[2].identity.title, std::string("Third requirement"));
}

TEST(YamlSimpleTest, MultiDocumentSkipsNoId)
{
    /* Second document lacks an "id" field and should be silently skipped. */
    const char *path = write_yaml("test_multi_noid.yaml",
        "id: REQ-001\n"
        "title: Has id\n"
        "---\n"
        "title: No id here\n"
        "---\n"
        "id: REQ-003\n"
        "title: Also has id\n");
    ASSERT_NE(path, nullptr);

    EntityList list;

    int rc = yaml_parse_entities(path, &list);
    EXPECT_EQ(rc, 2);
    EXPECT_EQ((int)list.size(), 2);
    EXPECT_EQ(list[0].identity.id, std::string("REQ-001"));
    EXPECT_EQ(list[1].identity.id, std::string("REQ-003"));
}

TEST(YamlSimpleTest, NonexistentFile)
{
    EntityList list;

    int rc = yaml_parse_entities("/tmp/no_such_file_xyz.yaml", &list);
    EXPECT_EQ(rc, -1);
    EXPECT_EQ((int)list.size(), 0);
}

TEST(YamlSimpleTest, FilePathStoredPerDocument)
{
    const char *path = write_yaml("test_path.yaml",
        "id: REQ-001\n"
        "title: First\n"
        "---\n"
        "id: REQ-002\n"
        "title: Second\n");
    ASSERT_NE(path, nullptr);

    EntityList list;

    yaml_parse_entities(path, &list);
    EXPECT_EQ((int)list.size(), 2);
    /* Both entities should record the same file path. */
    EXPECT_STREQ(list[0].identity.file_path, path);
    EXPECT_STREQ(list[1].identity.file_path, path);
}

/* -------------------------------------------------------------------------
 * Tests — yaml_parse_links with multi-document files
 * ---------------------------------------------------------------------- */

TEST(YamlSimpleTest, LinksSingleDocument)
{
    const char *path = write_yaml("test_links_single.yaml",
        "id: REQ-001\n"
        "title: Has links\n"
        "links:\n"
        "  - id: REQ-002\n"
        "    relation: implements\n"
        "  - id: REQ-003\n"
        "    relation: refines\n");
    ASSERT_NE(path, nullptr);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    int rc = yaml_parse_links(path, "REQ-001", store);
    EXPECT_EQ(rc, 2);

    triplet_store_destroy(store);
}

TEST(YamlSimpleTest, LinksMultiDocumentCorrectSubject)
{
    /* A two-document file; links belong to REQ-002 only. */
    const char *path = write_yaml("test_links_multi.yaml",
        "id: REQ-001\n"
        "title: No links\n"
        "---\n"
        "id: REQ-002\n"
        "title: Has links\n"
        "links:\n"
        "  - id: REQ-003\n"
        "    relation: implements\n");
    ASSERT_NE(path, nullptr);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    /* Asking for links of REQ-001 should yield 0 — it has none. */
    int rc1 = yaml_parse_links(path, "REQ-001", store);
    EXPECT_EQ(rc1, 0);

    /* Asking for links of REQ-002 should yield 1. */
    int rc2 = yaml_parse_links(path, "REQ-002", store);
    EXPECT_EQ(rc2, 1);

    triplet_store_destroy(store);
}

TEST(YamlSimpleTest, LinksNonexistentFile)
{
    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    int rc = yaml_parse_links("/tmp/no_such_file_xyz.yaml", "REQ-001", store);
    EXPECT_EQ(rc, -1);

    triplet_store_destroy(store);
}

TEST(YamlSimpleTest, LinksArtefactKey)
{
    /* Artefact-based links use "artefact" instead of "id" as the target key. */
    const char *path = write_yaml("test_links_artefact.yaml",
        "id: TC-001\n"
        "title: Test case with artefact links\n"
        "links:\n"
        "  - id: REQ-005\n"
        "    relation: verifies\n"
        "  - artefact: src/tests/test_foo.cpp\n"
        "    relation: implemented-by\n"
        "  - artefact: src/tests/test_foo.cpp#Suite.Test\n"
        "    relation: implemented-by-test\n");
    ASSERT_NE(path, nullptr);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    int rc = yaml_parse_links(path, "TC-001", store);
    EXPECT_EQ(rc, 3);

    /* Verify that each triple was stored with the correct object. */
    CTripleList list = triplet_store_find_by_subject(store, "TC-001");
    EXPECT_EQ((int)list.size(), 3u);

    int found_verifies          = 0;
    int found_implemented_by    = 0;
    int found_implemented_by_test = 0;
    for (size_t i = 0; i < (int)list.size(); i++) {
        const CTriple *t = &list.triples[i];
        if (strcmp(t->predicate, "verifies") == 0 &&
            strcmp(t->object, "REQ-005") == 0)
            found_verifies = 1;
        if (strcmp(t->predicate, "implemented-by") == 0 &&
            strcmp(t->object, "src/tests/test_foo.cpp") == 0)
            found_implemented_by = 1;
        if (strcmp(t->predicate, "implemented-by-test") == 0 &&
            strcmp(t->object, "src/tests/test_foo.cpp#Suite.Test") == 0)
            found_implemented_by_test = 1;
    }
    EXPECT_EQ(found_verifies, 1);
    EXPECT_EQ(found_implemented_by, 1);
    EXPECT_EQ(found_implemented_by_test, 1);

    triplet_store_list_free(list);
    triplet_store_destroy(store);
}

/* -------------------------------------------------------------------------
 * Tests — yaml_parse_entity with "links:" as an alias for "traceability:"
 * ---------------------------------------------------------------------- */

TEST(YamlSimpleTest, EntityLinksKeyAliasForTraceability)
{
    /* When a YAML entity uses "links:" instead of "traceability:", the
     * entity parser must still populate the TraceabilityComponent. */
    const char *path = write_yaml("test_entity_links_alias.yaml",
        "id: TC-ALIAS-001\n"
        "title: Test case using legacy links key\n"
        "type: test-case\n"
        "status: draft\n"
        "links:\n"
        "  - id: REQ-005\n"
        "    relation: verifies\n"
        "  - artefact: src/tests/test_foo.cpp\n"
        "    relation: implemented-by\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    ASSERT_EQ(rc, 0);

    EXPECT_EQ(e.identity.id, std::string("TC-ALIAS-001"));
    EXPECT_EQ(e.(int)traceability.size(), 2);
    EXPECT_NE(strstr(e.traceability.entries, "REQ-005"),           nullptr);
    EXPECT_NE(strstr(e.traceability.entries, "verifies"),          nullptr);
    EXPECT_NE(strstr(e.traceability.entries, "src/tests/test_foo.cpp"), nullptr);
    EXPECT_NE(strstr(e.traceability.entries, "implemented-by"),    nullptr);
}

TEST(YamlSimpleTest, EntityLinksKeyLoadedIntoTripletStore)
{
    /* Verify that a "links:"-based entity populates the TripletStore
     * via entity_traceability_to_triplets(). */
    const char *path = write_yaml("test_entity_links_triplets.yaml",
        "id: TC-ALIAS-002\n"
        "title: Test entity traceability via links key\n"
        "type: test-case\n"
        "links:\n"
        "  - id: REQ-010\n"
        "    relation: verifies\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(e.(int)traceability.size(), 1);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    int added = entity_traceability_to_triplets(&e, store);
    EXPECT_EQ(added, 1);

    CTripleList list = triplet_store_find_by_subject(store, "TC-ALIAS-002");
    ASSERT_EQ((int)list.size(), 1u);
    EXPECT_STREQ(list.triples[0].subject,   "TC-ALIAS-002");
    EXPECT_STREQ(list.triples[0].predicate, "verifies");
    EXPECT_STREQ(list.triples[0].object,    "REQ-010");

    triplet_store_list_free(list);
    triplet_store_destroy(store);
}
