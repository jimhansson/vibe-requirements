/**
 * @file test_yaml_simple.c
 * @brief Unit tests for yaml_simple.c — single and multi-document YAML parsing.
 */

#include "yaml_simple.h"
#include "requirement.h"
#include "triplet_store_c.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Tiny test framework (mirrors test_config.c)
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
        if ((int)(a) != (int)(b)) {                                         \
            fprintf(stderr, "FAIL  %s:%d  expected %d, got %d\n",          \
                    __FILE__, __LINE__, (int)(b), (int)(a));                \
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
 * Helpers — write temporary YAML files into /tmp
 * ---------------------------------------------------------------------- */

static const char *write_yaml(const char *filename, const char *content)
{
    static char path[512];
    snprintf(path, sizeof(path), "/tmp/%s", filename);
    FILE *f = fopen(path, "w");
    if (!f)
        return NULL;
    fputs(content, f);
    fclose(f);
    return path;
}

/* -------------------------------------------------------------------------
 * Tests — yaml_parse_requirements (multi-document)
 * ---------------------------------------------------------------------- */

static void test_single_document(void)
{
    const char *path = write_yaml("test_single.yaml",
        "id: REQ-001\n"
        "title: Single document\n"
        "type: functional\n"
        "status: draft\n"
        "priority: must\n");
    ASSERT(path != NULL);

    RequirementList list;
    req_list_init(&list);

    int rc = yaml_parse_requirements(path, &list);
    ASSERT_EQ(rc, 1);
    ASSERT_EQ(list.count, 1);
    ASSERT_STREQ(list.items[0].id,       "REQ-001");
    ASSERT_STREQ(list.items[0].title,    "Single document");
    ASSERT_STREQ(list.items[0].type,     "functional");
    ASSERT_STREQ(list.items[0].status,   "draft");
    ASSERT_STREQ(list.items[0].priority, "must");

    req_list_free(&list);
}

static void test_multi_document(void)
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
    ASSERT(path != NULL);

    RequirementList list;
    req_list_init(&list);

    int rc = yaml_parse_requirements(path, &list);
    ASSERT_EQ(rc, 3);
    ASSERT_EQ(list.count, 3);

    ASSERT_STREQ(list.items[0].id,    "REQ-001");
    ASSERT_STREQ(list.items[0].title, "First requirement");

    ASSERT_STREQ(list.items[1].id,    "REQ-002");
    ASSERT_STREQ(list.items[1].title, "Second requirement");

    ASSERT_STREQ(list.items[2].id,    "REQ-003");
    ASSERT_STREQ(list.items[2].title, "Third requirement");

    req_list_free(&list);
}

static void test_multi_document_skips_no_id(void)
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
    ASSERT(path != NULL);

    RequirementList list;
    req_list_init(&list);

    int rc = yaml_parse_requirements(path, &list);
    ASSERT_EQ(rc, 2);
    ASSERT_EQ(list.count, 2);
    ASSERT_STREQ(list.items[0].id, "REQ-001");
    ASSERT_STREQ(list.items[1].id, "REQ-003");

    req_list_free(&list);
}

static void test_nonexistent_file(void)
{
    RequirementList list;
    req_list_init(&list);

    int rc = yaml_parse_requirements("/tmp/no_such_file_xyz.yaml", &list);
    ASSERT_EQ(rc, -1);
    ASSERT_EQ(list.count, 0);

    req_list_free(&list);
}

static void test_file_path_stored_per_document(void)
{
    const char *path = write_yaml("test_path.yaml",
        "id: REQ-001\n"
        "title: First\n"
        "---\n"
        "id: REQ-002\n"
        "title: Second\n");
    ASSERT(path != NULL);

    RequirementList list;
    req_list_init(&list);

    yaml_parse_requirements(path, &list);
    ASSERT_EQ(list.count, 2);
    /* Both requirements should record the same file path. */
    ASSERT_STREQ(list.items[0].file_path, path);
    ASSERT_STREQ(list.items[1].file_path, path);

    req_list_free(&list);
}

/* -------------------------------------------------------------------------
 * Tests — yaml_parse_links with multi-document files
 * ---------------------------------------------------------------------- */

static void test_links_single_document(void)
{
    const char *path = write_yaml("test_links_single.yaml",
        "id: REQ-001\n"
        "title: Has links\n"
        "links:\n"
        "  - id: REQ-002\n"
        "    relation: implements\n"
        "  - id: REQ-003\n"
        "    relation: refines\n");
    ASSERT(path != NULL);

    TripletStore *store = triplet_store_create();
    ASSERT(store != NULL);

    int rc = yaml_parse_links(path, "REQ-001", store);
    ASSERT_EQ(rc, 2);

    triplet_store_destroy(store);
}

static void test_links_multi_document_correct_subject(void)
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
    ASSERT(path != NULL);

    TripletStore *store = triplet_store_create();
    ASSERT(store != NULL);

    /* Asking for links of REQ-001 should yield 0 — it has none. */
    int rc1 = yaml_parse_links(path, "REQ-001", store);
    ASSERT_EQ(rc1, 0);

    /* Asking for links of REQ-002 should yield 1. */
    int rc2 = yaml_parse_links(path, "REQ-002", store);
    ASSERT_EQ(rc2, 1);

    triplet_store_destroy(store);
}

static void test_links_nonexistent_file(void)
{
    TripletStore *store = triplet_store_create();
    ASSERT(store != NULL);

    int rc = yaml_parse_links("/tmp/no_such_file_xyz.yaml", "REQ-001", store);
    ASSERT_EQ(rc, -1);

    triplet_store_destroy(store);
}

/* -------------------------------------------------------------------------
 * Entry point
 * ---------------------------------------------------------------------- */

int main(void)
{
    test_single_document();
    test_multi_document();
    test_multi_document_skips_no_id();
    test_nonexistent_file();
    test_file_path_stored_per_document();
    test_links_single_document();
    test_links_multi_document_correct_subject();
    test_links_nonexistent_file();

    printf("\n%d test(s) run, %d failed.\n", g_tests_run, g_tests_failed);
    return (g_tests_failed > 0) ? 1 : 0;
}
