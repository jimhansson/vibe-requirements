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
#include "requirement.h"
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
 * Tests — yaml_parse_requirements (multi-document)
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

    RequirementList list;
    req_list_init(&list);

    int rc = yaml_parse_requirements(path, &list);
    EXPECT_EQ(rc, 1);
    EXPECT_EQ(list.count, 1);
    EXPECT_STREQ(list.items[0].id,       "REQ-001");
    EXPECT_STREQ(list.items[0].title,    "Single document");
    EXPECT_STREQ(list.items[0].type,     "functional");
    EXPECT_STREQ(list.items[0].status,   "draft");
    EXPECT_STREQ(list.items[0].priority, "must");

    req_list_free(&list);
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

    RequirementList list;
    req_list_init(&list);

    int rc = yaml_parse_requirements(path, &list);
    EXPECT_EQ(rc, 3);
    EXPECT_EQ(list.count, 3);

    EXPECT_STREQ(list.items[0].id,    "REQ-001");
    EXPECT_STREQ(list.items[0].title, "First requirement");

    EXPECT_STREQ(list.items[1].id,    "REQ-002");
    EXPECT_STREQ(list.items[1].title, "Second requirement");

    EXPECT_STREQ(list.items[2].id,    "REQ-003");
    EXPECT_STREQ(list.items[2].title, "Third requirement");

    req_list_free(&list);
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

    RequirementList list;
    req_list_init(&list);

    int rc = yaml_parse_requirements(path, &list);
    EXPECT_EQ(rc, 2);
    EXPECT_EQ(list.count, 2);
    EXPECT_STREQ(list.items[0].id, "REQ-001");
    EXPECT_STREQ(list.items[1].id, "REQ-003");

    req_list_free(&list);
}

TEST(YamlSimpleTest, NonexistentFile)
{
    RequirementList list;
    req_list_init(&list);

    int rc = yaml_parse_requirements("/tmp/no_such_file_xyz.yaml", &list);
    EXPECT_EQ(rc, -1);
    EXPECT_EQ(list.count, 0);

    req_list_free(&list);
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

    RequirementList list;
    req_list_init(&list);

    yaml_parse_requirements(path, &list);
    EXPECT_EQ(list.count, 2);
    /* Both requirements should record the same file path. */
    EXPECT_STREQ(list.items[0].file_path, path);
    EXPECT_STREQ(list.items[1].file_path, path);

    req_list_free(&list);
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
