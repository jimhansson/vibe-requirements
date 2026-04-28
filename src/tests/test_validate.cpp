/**
 * @file test_validate.cpp
 * @brief Unit tests for cmd_validate() — the repository consistency checker.
 *
 * Tests cover:
 *   - Clean repository: no duplicate IDs, no broken links → returns 0
 *   - Duplicate IDs: two entities sharing the same ID → returns 1+ and
 *     prints an error message to stderr
 *   - Broken link: a traceability link whose object is not a known entity ID
 *     → returns 1+ and prints an error message to stderr
 *   - File-path and URL objects are not flagged as broken links
 *   - Inferred triples are not checked for broken links
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>
#include <functional>
#include <string>

#include "entity.h"
#include "triplet_store.hpp"
#include "validate.h"

using ::testing::HasSubstr;
using ::testing::Not;

/* -------------------------------------------------------------------------
 * Helpers — capture stdout / stderr
 * ---------------------------------------------------------------------- */

static std::string capture_stdout(const std::function<void()> &fn)
{
    char path[] = "/tmp/test_validate_XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) return "";
    close(fd);

    FILE *old = freopen(path, "w", stdout);
    if (!old) { remove(path); return ""; }

    fn();
    fflush(stdout);

    freopen("/dev/tty", "w", stdout);

    FILE *f = fopen(path, "r");
    if (!f) { remove(path); return ""; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string result(static_cast<size_t>(sz > 0 ? sz : 0), '\0');
    if (sz > 0)
        fread(&result[0], 1, static_cast<size_t>(sz), f);
    fclose(f);
    remove(path);
    return result;
}

static std::string capture_stderr(const std::function<void()> &fn)
{
    char path[] = "/tmp/test_validate_err_XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) return "";
    close(fd);

    FILE *old = freopen(path, "w", stderr);
    if (!old) { remove(path); return ""; }

    fn();
    fflush(stderr);

    freopen("/dev/tty", "w", stderr);

    FILE *f = fopen(path, "r");
    if (!f) { remove(path); return ""; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string result(static_cast<size_t>(sz > 0 ? sz : 0), '\0');
    if (sz > 0)
        fread(&result[0], 1, static_cast<size_t>(sz), f);
    fclose(f);
    remove(path);
    return result;
}

/* -------------------------------------------------------------------------
 * Helper — build a minimal Entity
 * ---------------------------------------------------------------------- */

static Entity make_entity(const char *id, const char *file_path = "req.yaml",
                           EntityKind kind = ENTITY_KIND_REQUIREMENT)
{
    Entity e{};
    e.identity.id        = id;
    e.identity.title     = id;
    e.identity.kind      = kind;
    e.identity.file_path = file_path;
    return e;
}

/* =========================================================================
 * Tests — clean repository
 * ======================================================================= */

TEST(CmdValidateTest, EmptyListReturnsZero)
{
    EntityList elist;
    vibe::TripletStore store;

    int out = 0;
    std::string stdout_out = capture_stdout([&]() {
        out = cmd_validate(&elist, &store);
    });

    EXPECT_EQ(out, 0);
    EXPECT_THAT(stdout_out, HasSubstr("OK"));
}

TEST(CmdValidateTest, UniqueIdsAndNoLinksReturnsZero)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", "a.yaml"));
    elist.push_back(make_entity("REQ-002", "b.yaml"));

    vibe::TripletStore store;

    int out = 0;
    std::string stdout_out = capture_stdout([&]() {
        out = cmd_validate(&elist, &store);
    });

    EXPECT_EQ(out, 0);
    EXPECT_THAT(stdout_out, HasSubstr("OK"));
}

TEST(CmdValidateTest, ValidLinkToKnownEntityReturnsZero)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", "a.yaml"));
    elist.push_back(make_entity("TC-001",  "b.yaml", ENTITY_KIND_TEST_CASE));

    vibe::TripletStore store;
    store.add("REQ-001", "verified-by", "TC-001");

    int out = 0;
    std::string stdout_out = capture_stdout([&]() {
        out = cmd_validate(&elist, &store);
    });

    EXPECT_EQ(out, 0);
    EXPECT_THAT(stdout_out, HasSubstr("OK"));
}

/* =========================================================================
 * Tests — duplicate IDs
 * ======================================================================= */

TEST(CmdValidateTest, DuplicateIdReturnsNonZero)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", "dir_a/req.yaml"));
    elist.push_back(make_entity("REQ-001", "dir_b/req.yaml"));

    vibe::TripletStore store;

    int out = 0;
    std::string err_out = capture_stderr([&]() {
        capture_stdout([&]() {
            out = cmd_validate(&elist, &store);
        });
    });

    EXPECT_GT(out, 0);
    EXPECT_THAT(err_out, HasSubstr("duplicate"));
    EXPECT_THAT(err_out, HasSubstr("REQ-001"));
}

TEST(CmdValidateTest, DuplicateIdMentionsBothFilePaths)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-DUP", "first/req.yaml"));
    elist.push_back(make_entity("REQ-DUP", "second/req.yaml"));

    vibe::TripletStore store;

    std::string err_out = capture_stderr([&]() {
        capture_stdout([&]() { cmd_validate(&elist, &store); });
    });

    EXPECT_THAT(err_out, HasSubstr("first/req.yaml"));
    EXPECT_THAT(err_out, HasSubstr("second/req.yaml"));
}

TEST(CmdValidateTest, NoDuplicatesNoErrorMessage)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", "a.yaml"));
    elist.push_back(make_entity("REQ-002", "b.yaml"));

    vibe::TripletStore store;

    std::string err_out = capture_stderr([&]() {
        capture_stdout([&]() { cmd_validate(&elist, &store); });
    });

    EXPECT_THAT(err_out, Not(HasSubstr("duplicate")));
}

/* =========================================================================
 * Tests — broken links
 * ======================================================================= */

TEST(CmdValidateTest, LinkToUnknownEntityReturnsNonZero)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", "req.yaml"));

    vibe::TripletStore store;
    store.add("REQ-001", "verified-by", "TC-NONEXISTENT");

    int out = 0;
    std::string err_out = capture_stderr([&]() {
        capture_stdout([&]() {
            out = cmd_validate(&elist, &store);
        });
    });

    EXPECT_GT(out, 0);
    EXPECT_THAT(err_out, HasSubstr("TC-NONEXISTENT"));
    EXPECT_THAT(err_out, HasSubstr("REQ-001"));
}

TEST(CmdValidateTest, LinkToUnknownEntityMentionsRelation)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", "req.yaml"));

    vibe::TripletStore store;
    store.add("REQ-001", "verified-by", "TC-GHOST");

    std::string err_out = capture_stderr([&]() {
        capture_stdout([&]() { cmd_validate(&elist, &store); });
    });

    EXPECT_THAT(err_out, HasSubstr("verified-by"));
}

TEST(CmdValidateTest, FilePathObjectIsNotFlaggedAsBrokenLink)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", "req.yaml"));

    vibe::TripletStore store;
    store.add("REQ-001", "implemented-in", "src/auth/login.cpp");

    int out = 0;
    std::string err_out = capture_stderr([&]() {
        capture_stdout([&]() {
            out = cmd_validate(&elist, &store);
        });
    });

    EXPECT_EQ(out, 0);
    EXPECT_THAT(err_out, Not(HasSubstr("error:")));
}

TEST(CmdValidateTest, WindowsPathObjectIsNotFlaggedAsBrokenLink)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", "req.yaml"));

    vibe::TripletStore store;
    store.add("REQ-001", "implemented-in", "src\\auth\\login.cpp");

    int out = 0;
    capture_stderr([&]() {
        capture_stdout([&]() {
            out = cmd_validate(&elist, &store);
        });
    });

    EXPECT_EQ(out, 0);
}

TEST(CmdValidateTest, UrlObjectIsNotFlaggedAsBrokenLink)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", "req.yaml"));

    vibe::TripletStore store;
    store.add("REQ-001", "source", "https://example.com/spec");

    int out = 0;
    capture_stderr([&]() {
        capture_stdout([&]() {
            out = cmd_validate(&elist, &store);
        });
    });

    EXPECT_EQ(out, 0);
}

TEST(CmdValidateTest, InferredTripleIsNotCheckedForBrokenLink)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", "req.yaml"));

    vibe::TripletStore store;
    /* Add an inferred triple whose object does not exist — should be ignored. */
    store.add_inferred("TC-GHOST", "verifies", "REQ-001");

    int out = 0;
    capture_stderr([&]() {
        capture_stdout([&]() {
            out = cmd_validate(&elist, &store);
        });
    });

    EXPECT_EQ(out, 0);
}

/* =========================================================================
 * Tests — combined problems
 * ======================================================================= */

TEST(CmdValidateTest, MultipleProblemsAreAllCounted)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", "a.yaml"));
    elist.push_back(make_entity("REQ-001", "b.yaml")); /* duplicate */

    vibe::TripletStore store;
    store.add("REQ-001", "verified-by", "TC-MISSING"); /* broken link */

    int out = 0;
    capture_stderr([&]() {
        capture_stdout([&]() {
            out = cmd_validate(&elist, &store);
        });
    });

    EXPECT_GE(out, 2);
}

TEST(CmdValidateTest, SummaryLinePrintedToStderrOnProblems)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", "a.yaml"));
    elist.push_back(make_entity("REQ-001", "b.yaml"));

    vibe::TripletStore store;

    std::string err_out = capture_stderr([&]() {
        capture_stdout([&]() { cmd_validate(&elist, &store); });
    });

    EXPECT_THAT(err_out, HasSubstr("problem"));
}
