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
