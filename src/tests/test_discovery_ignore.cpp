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

#include "discovery.h"
#include "config.h"
#include "entity.h"

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
            "title: Ignored entity\n"
            "type: functional\n"
            "status: draft\n"
            "priority: must\n") != 0)
        return -1;

    return 0;
}

/* Return true if any item in list has the given id. */
static bool list_contains_id(const EntityList *list, const char *id)
{
    for (size_t i = 0; i < list->size(); i++) {
        if ((*list)[i].identity.id == id)
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

    int found = discover_entities(root, &list, &cfg);

    /* At least one entity must be found (from requirements/). */
    EXPECT_GE(found, 1);
    EXPECT_GE((int)list.size(), 1);

    /* REQ-001 (from requirements/) must be present. */
    EXPECT_TRUE(list_contains_id(&list, "REQ-001"));

    /* EX-001 (from examples/) must NOT be present. */
    EXPECT_FALSE(list_contains_id(&list, "EX-001"));

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

    int found = discover_entities(root, &list, nullptr);

    EXPECT_GE(found, 2);
    EXPECT_GE((int)list.size(), 2);
    EXPECT_TRUE(list_contains_id(&list, "REQ-001"));
    EXPECT_TRUE(list_contains_id(&list, "EX-001"));

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

    int found = discover_entities(root, &list, &cfg);

    EXPECT_GE(found, 2);
    EXPECT_GE((int)list.size(), 2);
    EXPECT_TRUE(list_contains_id(&list, "REQ-001"));
    EXPECT_TRUE(list_contains_id(&list, "EX-001"));

}

/* discover_entities must return -1 for a nonexistent root. */
TEST(DiscoveryIgnoreTest, NonexistentRoot)
{
    VibeConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    EntityList list;

    int rc = discover_entities("/tmp/vibe_disco_no_such_dir_xyz", &list, &cfg);
    EXPECT_EQ(rc, -1);
    EXPECT_EQ((int)list.size(), 0);

}

/*
 * Test that paths exceeding 512 characters are skipped with a warning.
 * The walk() function uses a 512-byte buffer for constructing paths.
 */
TEST(DiscoveryIgnoreTest, LongPathNamesSkipped)
{
    const char *root = "/tmp/vibe_disco_long_path_test";

    /* Clean up any previous test run */
    char cleanup_cmd[1024];
    snprintf(cleanup_cmd, sizeof(cleanup_cmd), "rm -rf %s", root);
    system(cleanup_cmd);

    make_dir(root);

    /* Create a deeply nested directory structure that will exceed 512 chars.
     * Each segment is 50 chars, so 11 levels = ~550 chars. */
    char deep_path[1024] = {0};
    strncpy(deep_path, root, sizeof(deep_path) - 1);

    for (int i = 0; i < 11; i++) {
        strncat(deep_path, "/this_is_a_very_long_directory_name_for_testing_01",
                sizeof(deep_path) - strlen(deep_path) - 1);
        make_dir(deep_path);
    }

    /* Write a valid entity YAML at the end of the deep path */
    char yaml_path[1024];
    snprintf(yaml_path, sizeof(yaml_path), "%s/entity.yaml", deep_path);
    write_file(yaml_path,
        "id: DEEP-001\n"
        "title: Entity in deep path\n"
        "type: functional\n");

    /* Also write a valid entity at the root level */
    char root_yaml[1024];
    snprintf(root_yaml, sizeof(root_yaml), "%s/root.yaml", root);
    write_file(root_yaml,
        "id: ROOT-001\n"
        "title: Root entity\n"
        "type: functional\n");

    EntityList list;

    int found = discover_entities(root, &list, nullptr);

    /* Should find ROOT-001 but not DEEP-001 (path too long) */
    EXPECT_GE(found, 1);
    EXPECT_TRUE(list_contains_id(&list, "ROOT-001"));
    EXPECT_FALSE(list_contains_id(&list, "DEEP-001"));

}

/*
 * Test that broken YAML files are skipped with a warning, but don't
 * stop discovery of other valid files.
 */
TEST(DiscoveryIgnoreTest, BrokenYamlSkipped)
{
    const char *root = "/tmp/vibe_disco_broken_yaml_test";

    /* Clean up any previous test run */
    char cleanup_cmd[1024];
    snprintf(cleanup_cmd, sizeof(cleanup_cmd), "rm -rf %s", root);
    system(cleanup_cmd);

    make_dir(root);

    /* Write a broken YAML file (invalid syntax) */
    char broken_path[512];
    snprintf(broken_path, sizeof(broken_path), "%s/broken.yaml", root);
    write_file(broken_path,
        "id: BROKEN-001\n"
        "title: This is broken\n"
        "  invalid indentation\n"
        "random: [\n"  /* unclosed bracket */
        "more junk\n");

    /* Write a YAML file without 'id' field (should be silently ignored) */
    char no_id_path[512];
    snprintf(no_id_path, sizeof(no_id_path), "%s/no_id.yaml", root);
    write_file(no_id_path,
        "title: No ID field\n"
        "type: functional\n");

    /* Write a valid YAML file */
    char valid_path[512];
    snprintf(valid_path, sizeof(valid_path), "%s/valid.yaml", root);
    write_file(valid_path,
        "id: VALID-001\n"
        "title: Valid entity\n"
        "type: functional\n");

    EntityList list;

    int found = discover_entities(root, &list, nullptr);

    /* Should find VALID-001 but not BROKEN-001 or the file without ID */
    EXPECT_EQ(found, 1);
    EXPECT_EQ((int)list.size(), 1);
    EXPECT_TRUE(list_contains_id(&list, "VALID-001"));
    EXPECT_FALSE(list_contains_id(&list, "BROKEN-001"));

}

/*
 * Test that hidden directories (names starting with '.') are skipped.
 */
TEST(DiscoveryIgnoreTest, HiddenDirectoriesSkipped)
{
    const char *root = "/tmp/vibe_disco_hidden_test";

    /* Clean up any previous test run */
    char cleanup_cmd[1024];
    snprintf(cleanup_cmd, sizeof(cleanup_cmd), "rm -rf %s", root);
    system(cleanup_cmd);

    make_dir(root);

    /* Create a hidden directory */
    char hidden_dir[512];
    snprintf(hidden_dir, sizeof(hidden_dir), "%s/.hidden", root);
    make_dir(hidden_dir);

    /* Write an entity in the hidden directory */
    char hidden_entity[512];
    snprintf(hidden_entity, sizeof(hidden_entity), "%s/entity.yaml", hidden_dir);
    write_file(hidden_entity,
        "id: HIDDEN-001\n"
        "title: Hidden entity\n"
        "type: functional\n");

    /* Write an entity in the root */
    char visible_entity[512];
    snprintf(visible_entity, sizeof(visible_entity), "%s/visible.yaml", root);
    write_file(visible_entity,
        "id: VISIBLE-001\n"
        "title: Visible entity\n"
        "type: functional\n");

    EntityList list;

    int found = discover_entities(root, &list, nullptr);

    /* Should find VISIBLE-001 but not HIDDEN-001 */
    EXPECT_EQ(found, 1);
    EXPECT_TRUE(list_contains_id(&list, "VISIBLE-001"));
    EXPECT_FALSE(list_contains_id(&list, "HIDDEN-001"));

}

/*
 * Test that .yml extension is also recognized (not just .yaml).
 */
TEST(DiscoveryIgnoreTest, YmlExtensionRecognized)
{
    const char *root = "/tmp/vibe_disco_yml_test";

    /* Clean up any previous test run */
    char cleanup_cmd[1024];
    snprintf(cleanup_cmd, sizeof(cleanup_cmd), "rm -rf %s", root);
    system(cleanup_cmd);

    make_dir(root);

    /* Write entity with .yml extension */
    char yml_path[512];
    snprintf(yml_path, sizeof(yml_path), "%s/entity.yml", root);
    write_file(yml_path,
        "id: YML-001\n"
        "title: YML extension entity\n"
        "type: functional\n");

    /* Write entity with .yaml extension */
    char yaml_path[512];
    snprintf(yaml_path, sizeof(yaml_path), "%s/entity.yaml", root);
    write_file(yaml_path,
        "id: YAML-001\n"
        "title: YAML extension entity\n"
        "type: functional\n");

    /* Write a non-YAML file */
    char txt_path[512];
    snprintf(txt_path, sizeof(txt_path), "%s/entity.txt", root);
    write_file(txt_path,
        "id: TXT-001\n"
        "title: TXT file\n"
        "type: functional\n");

    EntityList list;

    int found = discover_entities(root, &list, nullptr);

    /* Should find both YML-001 and YAML-001 but not TXT-001 */
    EXPECT_EQ(found, 2);
    EXPECT_TRUE(list_contains_id(&list, "YML-001"));
    EXPECT_TRUE(list_contains_id(&list, "YAML-001"));
    EXPECT_FALSE(list_contains_id(&list, "TXT-001"));

}

/*
 * Test that nested ignored directories are handled correctly.
 */
TEST(DiscoveryIgnoreTest, NestedIgnoredDirectories)
{
    const char *root = "/tmp/vibe_disco_nested_ignore_test";

    /* Clean up any previous test run */
    char cleanup_cmd[1024];
    snprintf(cleanup_cmd, sizeof(cleanup_cmd), "rm -rf %s", root);
    system(cleanup_cmd);

    make_dir(root);

    /* Create config that ignores "temp" and "backup" directories */
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/.vibe-req.yaml", root);
    write_file(config_path,
        "ignore_dirs:\n"
        "  - temp\n"
        "  - backup\n");

    /* Create a regular directory with a subdirectory named "temp" */
    char normal_dir[512];
    snprintf(normal_dir, sizeof(normal_dir), "%s/docs", root);
    make_dir(normal_dir);

    char nested_temp[512];
    snprintf(nested_temp, sizeof(nested_temp), "%s/temp", normal_dir);
    make_dir(nested_temp);

    /* Write entities in various locations */
    char root_entity[512];
    snprintf(root_entity, sizeof(root_entity), "%s/root.yaml", root);
    write_file(root_entity,
        "id: ROOT-001\n"
        "title: Root entity\n"
        "type: functional\n");

    char docs_entity[512];
    snprintf(docs_entity, sizeof(docs_entity), "%s/docs.yaml", normal_dir);
    write_file(docs_entity,
        "id: DOCS-001\n"
        "title: Docs entity\n"
        "type: functional\n");

    char temp_entity[512];
    snprintf(temp_entity, sizeof(temp_entity), "%s/temp.yaml", nested_temp);
    write_file(temp_entity,
        "id: TEMP-001\n"
        "title: Temp entity\n"
        "type: functional\n");

    VibeConfig cfg;
    int rc = config_load(root, &cfg);
    ASSERT_EQ(rc, 0);

    EntityList list;

    int found = discover_entities(root, &list, &cfg);

    /* Should find ROOT-001 and DOCS-001 but not TEMP-001 */
    EXPECT_EQ(found, 2);
    EXPECT_TRUE(list_contains_id(&list, "ROOT-001"));
    EXPECT_TRUE(list_contains_id(&list, "DOCS-001"));
    EXPECT_FALSE(list_contains_id(&list, "TEMP-001"));

}
