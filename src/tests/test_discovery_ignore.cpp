/**
 * @file test_discovery_ignore.cpp
 * @brief Automated test for TC-IGNORE-DIRS-001 — verifies that directories
 *        listed in ignore_dirs of .vibe-req.yaml are not scanned during
 *        entity auto-discovery, using gtest/gmock.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>
#include <sys/stat.h>

extern "C" {
#include "discovery.h"
#include "config.h"
#include "entity.h"
}

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
 *       req1.yaml             — valid entity REQ-001
 *     examples/
 *       ex1.yaml              — valid entity EX-001 (must be ignored)
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

/* Return true if any item in list has the given id. */
static bool list_contains_id(const EntityList *list, const char *id)
{
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->items[i].identity.id, id) == 0)
            return true;
    }
    return false;
}

/* -------------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */

/*
 * TC-IGNORE-DIRS-001 core test:
 * Entities inside an ignored directory must not appear; entities
 * inside a non-ignored directory must appear.
 */
TEST(DiscoveryIgnoreTest, IgnoredDirNotScanned)
{
    const char *root = "/tmp/vibe_disco_test";
    ASSERT_EQ(setup_test_tree(root), 0);

    VibeConfig cfg;
    int rc = config_load(root, &cfg);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(cfg.ignore_dirs_count, 1);

    EntityList list;
    entity_list_init(&list);

    int found = discover_entities(root, &list, &cfg);

    /* At least one entity must be found (from requirements/). */
    EXPECT_GE(found, 1);
    EXPECT_GE(list.count, 1);

    /* REQ-001 (from requirements/) must be present. */
    EXPECT_TRUE(list_contains_id(&list, "REQ-001"));

    /* EX-001 (from examples/) must NOT be present. */
    EXPECT_FALSE(list_contains_id(&list, "EX-001"));

    entity_list_free(&list);
}

/*
 * Without a config (cfg == NULL) both directories are scanned, so both
 * entities must appear.
 */
TEST(DiscoveryIgnoreTest, NoCfgScansAllDirs)
{
    const char *root = "/tmp/vibe_disco_test";
    ASSERT_EQ(setup_test_tree(root), 0);

    EntityList list;
    entity_list_init(&list);

    int found = discover_entities(root, &list, nullptr);

    EXPECT_GE(found, 2);
    EXPECT_GE(list.count, 2);
    EXPECT_TRUE(list_contains_id(&list, "REQ-001"));
    EXPECT_TRUE(list_contains_id(&list, "EX-001"));

    entity_list_free(&list);
}

/*
 * A config whose ignore_dirs list is empty must also scan all directories.
 */
TEST(DiscoveryIgnoreTest, EmptyIgnoreListScansAllDirs)
{
    const char *root = "/tmp/vibe_disco_test";
    ASSERT_EQ(setup_test_tree(root), 0);

    VibeConfig cfg;
    memset(&cfg, 0, sizeof(cfg));   /* ignore_dirs_count == 0 */

    EntityList list;
    entity_list_init(&list);

    int found = discover_entities(root, &list, &cfg);

    EXPECT_GE(found, 2);
    EXPECT_GE(list.count, 2);
    EXPECT_TRUE(list_contains_id(&list, "REQ-001"));
    EXPECT_TRUE(list_contains_id(&list, "EX-001"));

    entity_list_free(&list);
}

/* discover_entities must return -1 for a nonexistent root. */
TEST(DiscoveryIgnoreTest, NonexistentRoot)
{
    VibeConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    EntityList list;
    entity_list_init(&list);

    int rc = discover_entities("/tmp/vibe_disco_no_such_dir_xyz", &list, &cfg);
    EXPECT_EQ(rc, -1);
    EXPECT_EQ(list.count, 0);

    entity_list_free(&list);
}
