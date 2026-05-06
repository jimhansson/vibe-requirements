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
#include <initializer_list>
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

/* Build a config with a single vocabulary constraint for the given field. */
static VibeConfig make_vocab_config(const char *field,
                                    std::initializer_list<const char *> values)
{
    VibeConfig cfg{};
    if (!field)
        return cfg;
    strncpy(cfg.vocabulary[0].field, field, CONFIG_VOCAB_FIELD_LEN - 1);
    cfg.vocabulary[0].field[CONFIG_VOCAB_FIELD_LEN - 1] = '\0';
    int idx = 0;
    for (const char *value : values) {
        if (idx >= CONFIG_VOCAB_VALUES_MAX)
            break;
        strncpy(cfg.vocabulary[0].values[idx], value,
                CONFIG_VOCAB_VALUE_LEN - 1);
        cfg.vocabulary[0].values[idx][CONFIG_VOCAB_VALUE_LEN - 1] = '\0';
        idx++;
    }
    cfg.vocabulary[0].value_count = idx;
    cfg.vocabulary_count = 1;
    return cfg;
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
        out = cmd_validate(&elist, &store, nullptr);
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
        out = cmd_validate(&elist, &store, nullptr);
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
        out = cmd_validate(&elist, &store, nullptr);
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
            out = cmd_validate(&elist, &store, nullptr);
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
        capture_stdout([&]() { cmd_validate(&elist, &store, nullptr); });
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
        capture_stdout([&]() { cmd_validate(&elist, &store, nullptr); });
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
            out = cmd_validate(&elist, &store, nullptr);
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
        capture_stdout([&]() { cmd_validate(&elist, &store, nullptr); });
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
            out = cmd_validate(&elist, &store, nullptr);
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
            out = cmd_validate(&elist, &store, nullptr);
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
            out = cmd_validate(&elist, &store, nullptr);
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
            out = cmd_validate(&elist, &store, nullptr);
        });
    });

    EXPECT_EQ(out, 0);
}

/* =========================================================================
 * Tests — vocabulary validation
 * ======================================================================= */

TEST(CmdValidateTest, VocabularyViolationReportsFieldAndAllowedValues)
{
    EntityList elist;
    Entity e = make_entity("REQ-001", "req.yaml");
    e.lifecycle.status = "blocked";
    elist.push_back(e);

    vibe::TripletStore store;
    VibeConfig cfg = make_vocab_config("status", {"draft", "approved"});

    int out = 0;
    std::string err_out = capture_stderr([&]() {
        capture_stdout([&]() {
            out = cmd_validate(&elist, &store, &cfg);
        });
    });

    EXPECT_GT(out, 0);
    EXPECT_THAT(err_out, HasSubstr("status"));
    EXPECT_THAT(err_out, HasSubstr("blocked"));
    EXPECT_THAT(err_out, HasSubstr("draft"));
}

TEST(CmdValidateTest, VocabularyMatchIsCaseInsensitive)
{
    EntityList elist;
    Entity e = make_entity("REQ-001", "req.yaml");
    e.lifecycle.status = "Draft";
    elist.push_back(e);

    vibe::TripletStore store;
    VibeConfig cfg = make_vocab_config("status", {"draft", "approved"});

    int out = 0;
    std::string err_out = capture_stderr([&]() {
        capture_stdout([&]() {
            out = cmd_validate(&elist, &store, &cfg);
        });
    });

    EXPECT_EQ(out, 0);
    EXPECT_THAT(err_out, Not(HasSubstr("error:")));
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
            out = cmd_validate(&elist, &store, nullptr);
        });
    });

    EXPECT_GE(out, 2);
}

TEST(CmdValidateTest, FailFastStopsAfterFirstProblem)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", "a.yaml"));
    elist.push_back(make_entity("REQ-001", "b.yaml")); /* duplicate */

    vibe::TripletStore store;
    store.add("REQ-001", "verified-by", "TC-MISSING"); /* broken link */

    int out = 0;
    capture_stderr([&]() {
        capture_stdout([&]() {
            out = cmd_validate_with_options(&elist, &store, 1, nullptr);
        });
    });

    EXPECT_EQ(out, 1);
}

TEST(CmdValidateTest, SummaryLinePrintedToStderrOnProblems)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", "a.yaml"));
    elist.push_back(make_entity("REQ-001", "b.yaml"));

    vibe::TripletStore store;

    std::string err_out = capture_stderr([&]() {
        capture_stdout([&]() { cmd_validate(&elist, &store, nullptr); });
    });

    EXPECT_THAT(err_out, HasSubstr("problem"));
}

/* =========================================================================
 * Tests — required fields validation
 * ======================================================================= */

/* Helper — build a config with a requiredFields list. */
static VibeConfig make_required_fields_config(
    std::initializer_list<const char *> fields)
{
    VibeConfig cfg{};
    int idx = 0;
    for (const char *field : fields) {
        if (idx >= CONFIG_REQUIRED_FIELDS_MAX)
            break;
        strncpy(cfg.required_fields[idx], field, CONFIG_REQUIRED_FIELD_LEN - 1);
        cfg.required_fields[idx][CONFIG_REQUIRED_FIELD_LEN - 1] = '\0';
        idx++;
    }
    cfg.required_fields_count = idx;
    return cfg;
}

TEST(CmdValidateTest, RequiredFieldMissingReturnsNonZero)
{
    EntityList elist;
    /* Entity has no status set (make_entity only populates id, title, kind, file_path) */
    elist.push_back(make_entity("REQ-001", "req.yaml"));

    vibe::TripletStore store;
    VibeConfig cfg = make_required_fields_config({"status"});

    int out = 0;
    capture_stderr([&]() {
        capture_stdout([&]() {
            out = cmd_validate(&elist, &store, &cfg);
        });
    });

    EXPECT_GT(out, 0);
}

TEST(CmdValidateTest, RequiredFieldMissingMentionsEntityAndField)
{
    EntityList elist;
    Entity e{};
    e.identity.id        = "REQ-001";
    e.identity.file_path = "req.yaml";
    /* title deliberately left empty */
    elist.push_back(e);

    vibe::TripletStore store;
    VibeConfig cfg = make_required_fields_config({"title"});

    std::string err_out = capture_stderr([&]() {
        capture_stdout([&]() { cmd_validate(&elist, &store, &cfg); });
    });

    EXPECT_THAT(err_out, HasSubstr("REQ-001"));
    EXPECT_THAT(err_out, HasSubstr("title"));
}

TEST(CmdValidateTest, RequiredFieldEmptyStringCountsAsMissing)
{
    EntityList elist;
    Entity e{};
    e.identity.id        = "REQ-002";
    e.identity.file_path = "req.yaml";
    e.lifecycle.status   = "";          /* empty */
    elist.push_back(e);

    vibe::TripletStore store;
    VibeConfig cfg = make_required_fields_config({"status"});

    int out = 0;
    capture_stderr([&]() {
        capture_stdout([&]() {
            out = cmd_validate(&elist, &store, &cfg);
        });
    });

    EXPECT_GT(out, 0);
}

TEST(CmdValidateTest, RequiredFieldPresentPassesValidation)
{
    EntityList elist;
    Entity e{};
    e.identity.id        = "REQ-003";
    e.identity.title     = "My title";
    e.identity.file_path = "req.yaml";
    e.lifecycle.status   = "approved";
    e.lifecycle.priority = "must";
    elist.push_back(e);

    vibe::TripletStore store;
    VibeConfig cfg = make_required_fields_config({"title", "status", "priority"});

    int out = 0;
    std::string err_out = capture_stderr([&]() {
        capture_stdout([&]() {
            out = cmd_validate(&elist, &store, &cfg);
        });
    });

    EXPECT_EQ(out, 0);
    EXPECT_THAT(err_out, Not(HasSubstr("error:")));
}

TEST(CmdValidateTest, NoRequiredFieldsConfiguredPassesValidation)
{
    EntityList elist;
    /* Entity with only id — no title, status, etc. */
    elist.push_back(make_entity("REQ-004", "req.yaml"));

    vibe::TripletStore store;
    VibeConfig cfg{};  /* requiredFields not configured */

    int out = 0;
    capture_stderr([&]() {
        capture_stdout([&]() {
            out = cmd_validate(&elist, &store, &cfg);
        });
    });

    EXPECT_EQ(out, 0);
}

TEST(CmdValidateTest, MultipleRequiredFieldsReportsEachMissing)
{
    EntityList elist;
    Entity e{};
    e.identity.id        = "REQ-005";
    e.identity.file_path = "req.yaml";
    /* title, status, priority all empty */
    elist.push_back(e);

    vibe::TripletStore store;
    VibeConfig cfg = make_required_fields_config({"title", "status", "priority"});

    int out = 0;
    std::string err_out = capture_stderr([&]() {
        capture_stdout([&]() {
            out = cmd_validate(&elist, &store, &cfg);
        });
    });

    EXPECT_GE(out, 3);
    EXPECT_THAT(err_out, HasSubstr("title"));
    EXPECT_THAT(err_out, HasSubstr("status"));
    EXPECT_THAT(err_out, HasSubstr("priority"));
}
