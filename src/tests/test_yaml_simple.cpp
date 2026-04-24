/**
 * @file test_yaml_simple.cpp
 * @brief Unit tests for yaml_simple.c — single and multi-document YAML
 *        parsing, using gtest/gmock.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>

extern "C" {
#include "yaml_simple.h"
#include "entity.h"
#include "triplet_store_c.h"
}

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
    entity_list_init(&list);

    int rc = yaml_parse_entities(path, &list);
    EXPECT_EQ(rc, 1);
    EXPECT_EQ(list.count, 1);
    EXPECT_STREQ(list.items[0].identity.id,        "REQ-001");
    EXPECT_STREQ(list.items[0].identity.title,     "Single document");
    EXPECT_STREQ(list.items[0].identity.type_raw,  "functional");
    EXPECT_STREQ(list.items[0].lifecycle.status,   "draft");
    EXPECT_STREQ(list.items[0].lifecycle.priority, "must");

    entity_list_free(&list);
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
    entity_list_init(&list);

    int rc = yaml_parse_entities(path, &list);
    EXPECT_EQ(rc, 3);
    EXPECT_EQ(list.count, 3);

    EXPECT_STREQ(list.items[0].identity.id,    "REQ-001");
    EXPECT_STREQ(list.items[0].identity.title, "First requirement");

    EXPECT_STREQ(list.items[1].identity.id,    "REQ-002");
    EXPECT_STREQ(list.items[1].identity.title, "Second requirement");

    EXPECT_STREQ(list.items[2].identity.id,    "REQ-003");
    EXPECT_STREQ(list.items[2].identity.title, "Third requirement");

    entity_list_free(&list);
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
    entity_list_init(&list);

    int rc = yaml_parse_entities(path, &list);
    EXPECT_EQ(rc, 2);
    EXPECT_EQ(list.count, 2);
    EXPECT_STREQ(list.items[0].identity.id, "REQ-001");
    EXPECT_STREQ(list.items[1].identity.id, "REQ-003");

    entity_list_free(&list);
}

TEST(YamlSimpleTest, NonexistentFile)
{
    EntityList list;
    entity_list_init(&list);

    int rc = yaml_parse_entities("/tmp/no_such_file_xyz.yaml", &list);
    EXPECT_EQ(rc, -1);
    EXPECT_EQ(list.count, 0);

    entity_list_free(&list);
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
    entity_list_init(&list);

    yaml_parse_entities(path, &list);
    EXPECT_EQ(list.count, 2);
    /* Both entities should record the same file path. */
    EXPECT_STREQ(list.items[0].identity.file_path, path);
    EXPECT_STREQ(list.items[1].identity.file_path, path);

    entity_list_free(&list);
}

/* -------------------------------------------------------------------------
 * Tests — traceability via "traceability:" and "links:" keys
 * ---------------------------------------------------------------------- */

TEST(YamlSimpleTest, TraceabilitySingleDocument)
{
    const char *path = write_yaml("test_trace_single.yaml",
        "id: REQ-001\n"
        "title: Has traceability\n"
        "traceability:\n"
        "  - id: REQ-002\n"
        "    relation: implements\n"
        "  - id: REQ-003\n"
        "    relation: refines\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    ASSERT_EQ(rc, 0);
    EXPECT_EQ(e.traceability.count, 2);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    int added = entity_traceability_to_triplets(&e, store);
    EXPECT_EQ(added, 2);

    triplet_store_destroy(store);
}

TEST(YamlSimpleTest, TraceabilityMultiDocumentCorrectSubject)
{
    /* A two-document file; traceability belongs to REQ-002 only. */
    const char *path = write_yaml("test_trace_multi.yaml",
        "id: REQ-001\n"
        "title: No traceability\n"
        "---\n"
        "id: REQ-002\n"
        "title: Has traceability\n"
        "traceability:\n"
        "  - id: REQ-003\n"
        "    relation: implements\n");
    ASSERT_NE(path, nullptr);

    EntityList list;
    entity_list_init(&list);
    int rc = yaml_parse_entities(path, &list);
    ASSERT_EQ(rc, 2);

    /* REQ-001 should have no traceability links. */
    EXPECT_EQ(list.items[0].traceability.count, 0);
    /* REQ-002 should have one traceability link. */
    EXPECT_EQ(list.items[1].traceability.count, 1);

    entity_list_free(&list);
}

TEST(YamlSimpleTest, TraceabilityNonexistentFile)
{
    Entity e;
    int rc = yaml_parse_entity("/tmp/no_such_file_xyz.yaml", &e);
    EXPECT_EQ(rc, -1);
}

TEST(YamlSimpleTest, TraceabilityArtefactKey)
{
    /* Artefact-based links use "artefact" instead of "id" as the target key. */
    const char *path = write_yaml("test_trace_artefact.yaml",
        "id: TC-001\n"
        "title: Test case with artefact links\n"
        "traceability:\n"
        "  - id: REQ-005\n"
        "    relation: verifies\n"
        "  - artefact: src/tests/test_foo.cpp\n"
        "    relation: implemented-by\n"
        "  - artefact: src/tests/test_foo.cpp#Suite.Test\n"
        "    relation: implemented-by-test\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    ASSERT_EQ(rc, 0);
    EXPECT_EQ(e.traceability.count, 3);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    int added = entity_traceability_to_triplets(&e, store);
    EXPECT_EQ(added, 3);

    CTripleList list = triplet_store_find_by_subject(store, "TC-001");
    EXPECT_EQ(list.count, 3u);

    int found_verifies            = 0;
    int found_implemented_by      = 0;
    int found_implemented_by_test = 0;
    for (size_t i = 0; i < list.count; i++) {
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

    EXPECT_STREQ(e.identity.id, "TC-ALIAS-001");
    EXPECT_EQ(e.traceability.count, 2);
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
    ASSERT_EQ(e.traceability.count, 1);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    int added = entity_traceability_to_triplets(&e, store);
    EXPECT_EQ(added, 1);

    CTripleList list = triplet_store_find_by_subject(store, "TC-ALIAS-002");
    ASSERT_EQ(list.count, 1u);
    EXPECT_STREQ(list.triples[0].subject,   "TC-ALIAS-002");
    EXPECT_STREQ(list.triples[0].predicate, "verifies");
    EXPECT_STREQ(list.triples[0].object,    "REQ-010");

    triplet_store_list_free(list);
    triplet_store_destroy(store);
}
