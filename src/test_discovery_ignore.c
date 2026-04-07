/**
 * @file test_discovery_ignore.c
 * @brief Automated test for TC-IGNORE-DIRS-001 — verifies that directories
 *        listed in ignore_dirs of .vibe-req.yaml are not scanned during
 *        requirement auto-discovery.
 */

#include "discovery.h"
#include "config.h"
#include "requirement.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

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

/* -------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------- */

/* Create a directory (no-op if it already exists). */
static int make_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
        return 0;
    return mkdir(path, 0755);
}

/* Write content to a file, creating or overwriting it. */
static int write_file(const char *path, const char *content)
{
    FILE *f = fopen(path, "w");
    if (!f)
        return -1;
    fputs(content, f);
    fclose(f);
    return 0;
}

/*
 * Build a temporary directory tree under /tmp/vibe_disco_test/:
 *
 *   /tmp/vibe_disco_test/
 *     .vibe-req.yaml          — config: ignore_dirs: [examples]
 *     requirements/
 *       req1.yaml             — valid requirement REQ-001
 *     examples/
 *       ex1.yaml              — valid requirement EX-001 (must be ignored)
 *
 * Returns 0 on success.
 */
static int setup_test_tree(const char *root)
{
    char path[512];

    if (make_dir(root) != 0)
        return -1;

    /* .vibe-req.yaml */
    snprintf(path, sizeof(path), "%s/.vibe-req.yaml", root);
    if (write_file(path, "ignore_dirs:\n  - examples\n") != 0)
        return -1;

    /* requirements/ subdirectory */
    snprintf(path, sizeof(path), "%s/requirements", root);
    if (make_dir(path) != 0)
        return -1;

    snprintf(path, sizeof(path), "%s/requirements/req1.yaml", root);
    if (write_file(path,
            "id: REQ-001\n"
            "title: Scanned requirement\n"
            "type: functional\n"
            "status: draft\n"
            "priority: must\n") != 0)
        return -1;

    /* examples/ subdirectory (to be ignored) */
    snprintf(path, sizeof(path), "%s/examples", root);
    if (make_dir(path) != 0)
        return -1;

    snprintf(path, sizeof(path), "%s/examples/ex1.yaml", root);
    if (write_file(path,
            "id: EX-001\n"
            "title: Ignored requirement\n"
            "type: functional\n"
            "status: draft\n"
            "priority: must\n") != 0)
        return -1;

    return 0;
}

/* Return 1 if any item in list has the given id, 0 otherwise. */
static int list_contains_id(const RequirementList *list, const char *id)
{
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->items[i].id, id) == 0)
            return 1;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */

/*
 * TC-IGNORE-DIRS-001 core test:
 * Requirements inside an ignored directory must not appear; requirements
 * inside a non-ignored directory must appear.
 */
static void test_ignored_dir_not_scanned(void)
{
    const char *root = "/tmp/vibe_disco_test";
    ASSERT(setup_test_tree(root) == 0);

    VibeConfig cfg;
    int rc = config_load(root, &cfg);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(cfg.ignore_dirs_count, 1);

    RequirementList list;
    req_list_init(&list);

    int found = discover_requirements(root, &list, &cfg);

    /* At least one requirement must be found (from requirements/). */
    ASSERT(found >= 1);
    ASSERT(list.count >= 1);

    /* REQ-001 (from requirements/) must be present. */
    ASSERT(list_contains_id(&list, "REQ-001"));

    /* EX-001 (from examples/) must NOT be present. */
    ASSERT(!list_contains_id(&list, "EX-001"));

    req_list_free(&list);
}

/*
 * Without a config (cfg == NULL) both directories are scanned, so both
 * requirements must appear.
 */
static void test_no_cfg_scans_all_dirs(void)
{
    const char *root = "/tmp/vibe_disco_test";
    ASSERT(setup_test_tree(root) == 0);

    RequirementList list;
    req_list_init(&list);

    int found = discover_requirements(root, &list, NULL);

    ASSERT(found >= 2);
    ASSERT(list.count >= 2);
    ASSERT(list_contains_id(&list, "REQ-001"));
    ASSERT(list_contains_id(&list, "EX-001"));

    req_list_free(&list);
}

/*
 * A config whose ignore_dirs list is empty must also scan all directories.
 */
static void test_empty_ignore_list_scans_all_dirs(void)
{
    const char *root = "/tmp/vibe_disco_test";
    ASSERT(setup_test_tree(root) == 0);

    VibeConfig cfg;
    memset(&cfg, 0, sizeof(cfg));   /* ignore_dirs_count == 0 */

    RequirementList list;
    req_list_init(&list);

    int found = discover_requirements(root, &list, &cfg);

    ASSERT(found >= 2);
    ASSERT(list.count >= 2);
    ASSERT(list_contains_id(&list, "REQ-001"));
    ASSERT(list_contains_id(&list, "EX-001"));

    req_list_free(&list);
}

/* discover_requirements must return -1 for a nonexistent root. */
static void test_nonexistent_root(void)
{
    VibeConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    RequirementList list;
    req_list_init(&list);

    int rc = discover_requirements("/tmp/vibe_disco_no_such_dir_xyz", &list, &cfg);
    ASSERT_EQ(rc, -1);
    ASSERT_EQ(list.count, 0);

    req_list_free(&list);
}

/* -------------------------------------------------------------------------
 * Entry point
 * ---------------------------------------------------------------------- */

int main(void)
{
    test_ignored_dir_not_scanned();
    test_no_cfg_scans_all_dirs();
    test_empty_ignore_list_scans_all_dirs();
    test_nonexistent_root();

    printf("\n%d test(s) run, %d failed.\n", g_tests_run, g_tests_failed);
    return (g_tests_failed > 0) ? 1 : 0;
}
