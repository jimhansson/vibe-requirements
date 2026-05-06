/**
 * @file test_config.cpp
 * @brief Unit tests for the project configuration parser (config.h / config.cpp)
 *        using gtest/gmock.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>

#include "config.h"
#include "string_utils.h"

/* -------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------- */

/* Write a temporary .vibe-req.yaml into dir and return 0 on success. */
static int write_config(const char *dir, const char *content)
{
    char path[CONFIG_PATH_BUF_LEN];
    snprintf(path, sizeof(path), "%s/.vibe-req.yaml", dir);
    FILE *f = fopen(path, "w");
    if (!f)
        return -1;
    fputs(content, f);
    fclose(f);
    return 0;
}

static int find_vocab_field(const VibeConfig *cfg, const char *field)
{
    if (!cfg || !field)
        return -1;
    for (int i = 0; i < cfg->vocabulary_count; i++) {
        if (str_equal_ci(cfg->vocabulary[i].field, field))
            return i;
    }
    return -1;
}

/* -------------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */

TEST(ConfigTest, LoadNonexistent)
{
    VibeConfig cfg;
    int rc = config_load("/tmp/vibe_cfg_nonexistent_dir_xyz", &cfg);
    EXPECT_EQ(rc, -1);
    EXPECT_EQ(cfg.ignore_dirs_count, 0);
}

TEST(ConfigTest, LoadNullRoot)
{
    VibeConfig cfg;
    int rc = config_load(nullptr, &cfg);
    EXPECT_EQ(rc, -1);
    EXPECT_EQ(cfg.ignore_dirs_count, 0);
}

TEST(ConfigTest, LoadSingleIgnoreDir)
{
    const char *dir = "/tmp";
    write_config(dir, "ignore_dirs:\n  - examples\n");

    VibeConfig cfg;
    int rc = config_load(dir, &cfg);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(cfg.ignore_dirs_count, 1);
    EXPECT_STREQ(cfg.ignore_dirs[0], "examples");
}

TEST(ConfigTest, LoadMultipleIgnoreDirs)
{
    const char *dir = "/tmp";
    write_config(dir, "ignore_dirs:\n  - examples\n  - vendor\n  - build\n");

    VibeConfig cfg;
    int rc = config_load(dir, &cfg);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(cfg.ignore_dirs_count, 3);
    EXPECT_STREQ(cfg.ignore_dirs[0], "examples");
    EXPECT_STREQ(cfg.ignore_dirs[1], "vendor");
    EXPECT_STREQ(cfg.ignore_dirs[2], "build");
}

TEST(ConfigTest, LoadVocabularyConfig)
{
    const char *dir = "/tmp";
    write_config(dir,
        "ignore_dirs:\n"
        "  - examples\n"
        "vocabulary:\n"
        "  status:\n"
        "    - todo\n"
        "    - done\n"
        "  priority: [low, medium, high]\n");

    VibeConfig cfg;
    int rc = config_load(dir, &cfg);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(cfg.vocabulary_count, 2);

    int status_idx = find_vocab_field(&cfg, "status");
    ASSERT_GE(status_idx, 0);
    EXPECT_EQ(cfg.vocabulary[status_idx].value_count, 2);
    EXPECT_STREQ(cfg.vocabulary[status_idx].values[0], "todo");
    EXPECT_STREQ(cfg.vocabulary[status_idx].values[1], "done");

    int prio_idx = find_vocab_field(&cfg, "priority");
    ASSERT_GE(prio_idx, 0);
    EXPECT_EQ(cfg.vocabulary[prio_idx].value_count, 3);
    EXPECT_STREQ(cfg.vocabulary[prio_idx].values[0], "low");
    EXPECT_STREQ(cfg.vocabulary[prio_idx].values[1], "medium");
    EXPECT_STREQ(cfg.vocabulary[prio_idx].values[2], "high");
}

TEST(ConfigTest, LoadNoIgnoreDirsKey)
{
    const char *dir = "/tmp";
    write_config(dir, "other_key: value\n");

    VibeConfig cfg;
    int rc = config_load(dir, &cfg);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(cfg.ignore_dirs_count, 0);
}

TEST(ConfigTest, IsIgnoredDirMatch)
{
    VibeConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.ignore_dirs[0], "examples", CONFIG_IGNORE_DIR_LEN - 1);
    cfg.ignore_dirs_count = 1;

    EXPECT_EQ(config_is_ignored_dir(&cfg, "examples"), 1);
}

TEST(ConfigTest, IsIgnoredDirNoMatch)
{
    VibeConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.ignore_dirs[0], "examples", CONFIG_IGNORE_DIR_LEN - 1);
    cfg.ignore_dirs_count = 1;

    EXPECT_EQ(config_is_ignored_dir(&cfg, "requirements"), 0);
}

TEST(ConfigTest, IsIgnoredDirNullCfg)
{
    EXPECT_EQ(config_is_ignored_dir(nullptr, "examples"), 0);
}

TEST(ConfigTest, IsIgnoredDirNullName)
{
    VibeConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.ignore_dirs[0], "examples", CONFIG_IGNORE_DIR_LEN - 1);
    cfg.ignore_dirs_count = 1;

    EXPECT_EQ(config_is_ignored_dir(&cfg, nullptr), 0);
}

TEST(ConfigTest, IsIgnoredDirEmptyList)
{
    VibeConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    EXPECT_EQ(config_is_ignored_dir(&cfg, "examples"), 0);
}
