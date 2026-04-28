/**
 * @file test_status.cpp
 * @brief Unit tests for the status summary command.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdlib>
#include <cstdio>
#include <string>
#include <unistd.h>

#include "entity.h"
#include "status.h"

using ::testing::HasSubstr;

static Entity make_entity(const char *id, EntityKind kind,
                          const char *status = "draft",
                          const char *priority = "must")
{
    Entity e{};
    e.identity.id = id;
    e.identity.kind = kind;
    e.lifecycle.status = status;
    e.lifecycle.priority = priority;
    return e;
}

template <typename Fn>
static std::string capture_stdout(Fn fn)
{
    char path[] = "/tmp/test_status_XXXXXX";
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

TEST(CmdStatusTest, EmptyListPrintsNoCounts)
{
    EntityList elist;

    std::string out = capture_stdout([&]() {
        cmd_status(&elist);
    });

    EXPECT_THAT(out, HasSubstr("Status Summary"));
    EXPECT_THAT(out, HasSubstr("Total entities: 0"));
    EXPECT_THAT(out, HasSubstr("By status:"));
    EXPECT_THAT(out, HasSubstr("By priority:"));
    EXPECT_THAT(out, HasSubstr("By kind:"));
    EXPECT_THAT(out, HasSubstr("(none)"));
}

TEST(CmdStatusTest, CountsStatusPriorityAndKind)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", ENTITY_KIND_REQUIREMENT,
                                "draft", "must"));
    elist.push_back(make_entity("REQ-002", ENTITY_KIND_REQUIREMENT,
                                "approved", "should"));
    elist.push_back(make_entity("TC-001", ENTITY_KIND_TEST_CASE,
                                "draft", "must"));

    std::string out = capture_stdout([&]() {
        cmd_status(&elist);
    });

    EXPECT_THAT(out, HasSubstr("Total entities: 3"));
    EXPECT_THAT(out, HasSubstr("| draft"));
    EXPECT_THAT(out, HasSubstr("| approved"));
    EXPECT_THAT(out, HasSubstr("| must"));
    EXPECT_THAT(out, HasSubstr("| should"));
    EXPECT_THAT(out, HasSubstr("| requirement"));
    EXPECT_THAT(out, HasSubstr("| test-case"));
}

TEST(CmdStatusTest, EmptyStatusAndPriorityAreReportedAsNone)
{
    EntityList elist;
    elist.push_back(make_entity("REQ-001", ENTITY_KIND_REQUIREMENT, "", ""));

    std::string out = capture_stdout([&]() {
        cmd_status(&elist);
    });

    EXPECT_THAT(out, HasSubstr("| (none)"));
}
