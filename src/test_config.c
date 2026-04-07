/**
 * @file test_config.c
 * @brief Unit tests for the project configuration parser (config.h / config.c).
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Tiny test framework (mirrors test_triplet_store.c)
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

static void test_load_nonexistent(void)
{
    VibeConfig cfg;
    int rc = config_load("/tmp/vibe_cfg_nonexistent_dir_xyz", &cfg);
    ASSERT_EQ(rc, -1);
    ASSERT_EQ(cfg.ignore_dirs_count, 0);
}

static void test_load_null_root(void)
{
    VibeConfig cfg;
    int rc = config_load(NULL, &cfg);
    ASSERT_EQ(rc, -1);
    ASSERT_EQ(cfg.ignore_dirs_count, 0);
}

static void test_load_single_ignore_dir(void)
{
    const char *dir = "/tmp";
    write_config(dir, "ignore_dirs:\n  - examples\n");

    VibeConfig cfg;
    int rc = config_load(dir, &cfg);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(cfg.ignore_dirs_count, 1);
    ASSERT_STREQ(cfg.ignore_dirs[0], "examples");
}

static void test_load_multiple_ignore_dirs(void)
{
    const char *dir = "/tmp";
    write_config(dir, "ignore_dirs:\n  - examples\n  - vendor\n  - build\n");

    VibeConfig cfg;
    int rc = config_load(dir, &cfg);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(cfg.ignore_dirs_count, 3);
    ASSERT_STREQ(cfg.ignore_dirs[0], "examples");
    ASSERT_STREQ(cfg.ignore_dirs[1], "vendor");
    ASSERT_STREQ(cfg.ignore_dirs[2], "build");
}

static void test_load_no_ignore_dirs_key(void)
{
    const char *dir = "/tmp";
    write_config(dir, "other_key: value\n");

    VibeConfig cfg;
    int rc = config_load(dir, &cfg);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(cfg.ignore_dirs_count, 0);
}

static void test_is_ignored_dir_match(void)
{
    VibeConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.ignore_dirs[0], "examples", CONFIG_IGNORE_DIR_LEN - 1);
    cfg.ignore_dirs_count = 1;

    ASSERT_EQ(config_is_ignored_dir(&cfg, "examples"), 1);
}

static void test_is_ignored_dir_no_match(void)
{
    VibeConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.ignore_dirs[0], "examples", CONFIG_IGNORE_DIR_LEN - 1);
    cfg.ignore_dirs_count = 1;

    ASSERT_EQ(config_is_ignored_dir(&cfg, "requirements"), 0);
}

static void test_is_ignored_dir_null_cfg(void)
{
    ASSERT_EQ(config_is_ignored_dir(NULL, "examples"), 0);
}

static void test_is_ignored_dir_null_name(void)
{
    VibeConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.ignore_dirs[0], "examples", CONFIG_IGNORE_DIR_LEN - 1);
    cfg.ignore_dirs_count = 1;

    ASSERT_EQ(config_is_ignored_dir(&cfg, NULL), 0);
}

static void test_is_ignored_dir_empty_list(void)
{
    VibeConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    ASSERT_EQ(config_is_ignored_dir(&cfg, "examples"), 0);
}

/* -------------------------------------------------------------------------
 * Entry point
 * ---------------------------------------------------------------------- */

int main(void)
{
    test_load_nonexistent();
    test_load_null_root();
    test_load_single_ignore_dir();
    test_load_multiple_ignore_dirs();
    test_load_no_ignore_dirs_key();
    test_is_ignored_dir_match();
    test_is_ignored_dir_no_match();
    test_is_ignored_dir_null_cfg();
    test_is_ignored_dir_null_name();
    test_is_ignored_dir_empty_list();

    printf("\n%d test(s) run, %d failed.\n", g_tests_run, g_tests_failed);
    return (g_tests_failed > 0) ? 1 : 0;
}
