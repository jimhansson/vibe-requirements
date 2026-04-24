/**
 * @file test_new_cmd.cpp
 * @brief Unit tests for new_cmd.c — the 'new' subcommand that scaffolds
 *        entity YAML files.
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

extern "C" {
#include "new_cmd.h"
#include "entity.h"
}

/* -------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------- */

static bool file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

/** Read the entire content of a file into a std::string. */
static std::string read_file(const char *path)
{
    std::ifstream f(path);
    if (!f.is_open())
        return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

/** Remove a file; ignore errors. */
static void remove_file(const char *path)
{
    std::remove(path);
}

/* =========================================================================
 * Tests — error cases
 * ======================================================================= */

TEST(NewCmdTest, UnknownTypeReturnsError)
{
    int rc = new_cmd_scaffold("unknowntype", "TEST-001", "/tmp");
    EXPECT_EQ(rc, -3);
    EXPECT_FALSE(file_exists("/tmp/TEST-001.yaml"));
}

TEST(NewCmdTest, EmptyTypeReturnsError)
{
    int rc = new_cmd_scaffold("", "TEST-002", "/tmp");
    EXPECT_EQ(rc, -3);
}

TEST(NewCmdTest, NullTypeReturnsError)
{
    int rc = new_cmd_scaffold(nullptr, "TEST-003", "/tmp");
    EXPECT_EQ(rc, -3);
}

TEST(NewCmdTest, EmptyIdReturnsError)
{
    int rc = new_cmd_scaffold("requirement", "", "/tmp");
    EXPECT_EQ(rc, -3);
}

TEST(NewCmdTest, NullIdReturnsError)
{
    int rc = new_cmd_scaffold("requirement", nullptr, "/tmp");
    EXPECT_EQ(rc, -3);
}

TEST(NewCmdTest, ExistingFileReturnsError)
{
    const char *path = "/tmp/NEW-EXIST-001.yaml";
    /* Create the file first. */
    FILE *f = fopen(path, "w");
    ASSERT_NE(f, nullptr);
    fputs("id: NEW-EXIST-001\n", f);
    fclose(f);

    int rc = new_cmd_scaffold("requirement", "NEW-EXIST-001", "/tmp");
    EXPECT_EQ(rc, -1);

    remove_file(path);
}

/* =========================================================================
 * Tests — successful scaffold for every entity kind
 * ======================================================================= */

struct ScaffoldCase {
    const char *type_str;
    const char *id;
    const char *expected_type_in_yaml;
};

class NewCmdScaffoldTest : public ::testing::TestWithParam<ScaffoldCase> {};

TEST_P(NewCmdScaffoldTest, CreatesFileWithCorrectContent)
{
    const ScaffoldCase &tc = GetParam();
    std::string path = std::string("/tmp/") + tc.id + ".yaml";

    /* Ensure file does not exist before the test. */
    remove_file(path.c_str());

    int rc = new_cmd_scaffold(tc.type_str, tc.id, "/tmp");
    EXPECT_EQ(rc, 0) << "scaffold failed for type=" << tc.type_str;

    EXPECT_TRUE(file_exists(path.c_str()));

    std::string content = read_file(path.c_str());

    /* id field must be present. */
    EXPECT_NE(content.find(std::string("id: ") + tc.id), std::string::npos)
        << "missing id field in " << path;

    /* type field must be present. */
    std::string type_line = std::string("type: ") + tc.expected_type_in_yaml;
    EXPECT_NE(content.find(type_line), std::string::npos)
        << "missing type line '" << type_line << "' in " << path;

    /* status: draft must be present. */
    EXPECT_NE(content.find("status: draft"), std::string::npos)
        << "missing 'status: draft' in " << path;

    remove_file(path.c_str());
}

INSTANTIATE_TEST_SUITE_P(
    AllEntityTypes,
    NewCmdScaffoldTest,
    ::testing::Values(
        ScaffoldCase{"requirement",  "NEW-REQ-001",    "requirement"},
        ScaffoldCase{"functional",   "NEW-REQ-002",    "requirement"},
        ScaffoldCase{"non-functional","NEW-REQ-003",   "requirement"},
        ScaffoldCase{"group",        "NEW-GRP-001",    "group"},
        ScaffoldCase{"story",        "NEW-STORY-001",  "story"},
        ScaffoldCase{"user-story",   "NEW-STORY-002",  "story"},
        ScaffoldCase{"design-note",  "NEW-DN-001",     "design-note"},
        ScaffoldCase{"design",       "NEW-DN-002",     "design"},
        ScaffoldCase{"section",      "NEW-SEC-001",    "section"},
        ScaffoldCase{"assumption",   "NEW-ASSUM-001",  "assumption"},
        ScaffoldCase{"constraint",   "NEW-CONSTR-001", "constraint"},
        ScaffoldCase{"test-case",    "NEW-TC-001",     "test-case"},
        ScaffoldCase{"test",         "NEW-TC-002",     "test-case"},
        ScaffoldCase{"external",     "NEW-EXT-001",    "external"},
        ScaffoldCase{"directive",    "NEW-EXT-002",    "external"},
        ScaffoldCase{"standard",     "NEW-EXT-003",    "external"},
        ScaffoldCase{"regulation",   "NEW-EXT-004",    "external"},
        ScaffoldCase{"document",     "NEW-DOC-001",    "document"},
        ScaffoldCase{"srs",          "NEW-DOC-002",    "srs"},
        ScaffoldCase{"sdd",          "NEW-DOC-003",    "sdd"}
    )
);

/* =========================================================================
 * Tests — content spot-checks for specific kinds
 * ======================================================================= */

TEST(NewCmdContentTest, RequirementHasPriority)
{
    const char *path = "/tmp/NEW-REQ-CONTENT-001.yaml";
    remove_file(path);
    ASSERT_EQ(new_cmd_scaffold("requirement", "NEW-REQ-CONTENT-001", "/tmp"), 0);
    std::string content = read_file(path);
    EXPECT_NE(content.find("priority: must"), std::string::npos);
    remove_file(path);
}

TEST(NewCmdContentTest, StoryHasRoleGoalReason)
{
    const char *path = "/tmp/NEW-STORY-CONTENT-001.yaml";
    remove_file(path);
    ASSERT_EQ(new_cmd_scaffold("story", "NEW-STORY-CONTENT-001", "/tmp"), 0);
    std::string content = read_file(path);
    EXPECT_NE(content.find("role:"), std::string::npos);
    EXPECT_NE(content.find("goal:"), std::string::npos);
    EXPECT_NE(content.find("reason:"), std::string::npos);
    EXPECT_NE(content.find("acceptance_criteria:"), std::string::npos);
    remove_file(path);
}

TEST(NewCmdContentTest, AssumptionHasAssumptionBlock)
{
    const char *path = "/tmp/NEW-ASSUM-CONTENT-001.yaml";
    remove_file(path);
    ASSERT_EQ(new_cmd_scaffold("assumption", "NEW-ASSUM-CONTENT-001", "/tmp"), 0);
    std::string content = read_file(path);
    EXPECT_NE(content.find("assumption:"), std::string::npos);
    remove_file(path);
}

TEST(NewCmdContentTest, ConstraintHasConstraintBlock)
{
    const char *path = "/tmp/NEW-CONSTR-CONTENT-001.yaml";
    remove_file(path);
    ASSERT_EQ(new_cmd_scaffold("constraint", "NEW-CONSTR-CONTENT-001", "/tmp"), 0);
    std::string content = read_file(path);
    EXPECT_NE(content.find("constraint:"), std::string::npos);
    remove_file(path);
}

TEST(NewCmdContentTest, TestCaseHasPreconditionsAndSteps)
{
    const char *path = "/tmp/NEW-TC-CONTENT-001.yaml";
    remove_file(path);
    ASSERT_EQ(new_cmd_scaffold("test-case", "NEW-TC-CONTENT-001", "/tmp"), 0);
    std::string content = read_file(path);
    EXPECT_NE(content.find("preconditions:"), std::string::npos);
    EXPECT_NE(content.find("steps:"), std::string::npos);
    EXPECT_NE(content.find("expected_result:"), std::string::npos);
    remove_file(path);
}

TEST(NewCmdContentTest, ExternalHasClauses)
{
    const char *path = "/tmp/NEW-EXT-CONTENT-001.yaml";
    remove_file(path);
    ASSERT_EQ(new_cmd_scaffold("external", "NEW-EXT-CONTENT-001", "/tmp"), 0);
    std::string content = read_file(path);
    EXPECT_NE(content.find("clauses:"), std::string::npos);
    remove_file(path);
}

TEST(NewCmdContentTest, DocumentHasDocMeta)
{
    const char *path = "/tmp/NEW-DOC-CONTENT-001.yaml";
    remove_file(path);
    ASSERT_EQ(new_cmd_scaffold("document", "NEW-DOC-CONTENT-001", "/tmp"), 0);
    std::string content = read_file(path);
    EXPECT_NE(content.find("doc_meta:"), std::string::npos);
    remove_file(path);
}

TEST(NewCmdContentTest, DesignNoteHasBody)
{
    const char *path = "/tmp/NEW-DN-CONTENT-001.yaml";
    remove_file(path);
    ASSERT_EQ(new_cmd_scaffold("design-note", "NEW-DN-CONTENT-001", "/tmp"), 0);
    std::string content = read_file(path);
    EXPECT_NE(content.find("body:"), std::string::npos);
    remove_file(path);
}

/* =========================================================================
 * Tests — null/empty directory falls back to "."
 * ======================================================================= */

TEST(NewCmdDirTest, NullDirUsesCurrentDirectory)
{
    /* Change to /tmp for this test to avoid polluting cwd. */
    char saved_cwd[1024];
    ASSERT_NE(getcwd(saved_cwd, sizeof(saved_cwd)), nullptr);
    ASSERT_EQ(chdir("/tmp"), 0);

    const char *path = "/tmp/NEW-NULLDIR-001.yaml";
    remove_file(path);

    int rc = new_cmd_scaffold("requirement", "NEW-NULLDIR-001", nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_TRUE(file_exists(path));

    remove_file(path);
    ASSERT_EQ(chdir(saved_cwd), 0);
}

TEST(NewCmdDirTest, EmptyDirUsesCurrentDirectory)
{
    char saved_cwd[1024];
    ASSERT_NE(getcwd(saved_cwd, sizeof(saved_cwd)), nullptr);
    ASSERT_EQ(chdir("/tmp"), 0);

    const char *path = "/tmp/NEW-EMPTYDIR-001.yaml";
    remove_file(path);

    int rc = new_cmd_scaffold("requirement", "NEW-EMPTYDIR-001", "");
    EXPECT_EQ(rc, 0);
    EXPECT_TRUE(file_exists(path));

    remove_file(path);
    ASSERT_EQ(chdir(saved_cwd), 0);
}
