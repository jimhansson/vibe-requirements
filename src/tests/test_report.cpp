/**
 * @file test_report.cpp
 * @brief Unit tests for report.c — report_write() Markdown output.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "entity.h"
#include "report.h"
#include "triplet_store_c.h"

using ::testing::HasSubstr;
using ::testing::Not;

/* -------------------------------------------------------------------------
 * Helper — capture report_write() output to a std::string
 * ---------------------------------------------------------------------- */

static std::string capture_report(const EntityList *list,
                                   const TripletStore *store,
                                   ReportFormat fmt = REPORT_FORMAT_MARKDOWN)
{
    /* Use a temporary file to capture output (portable; avoids fmemopen). */
    char path[] = "/tmp/test_report_XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0)
        return "";
    close(fd);

    FILE *f = fopen(path, "w");
    if (!f)
        return "";

    report_write(f, list, store, fmt);
    fclose(f);

    /* Read back */
    f = fopen(path, "r");
    if (!f)
        return "";
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string result(static_cast<size_t>(sz > 0 ? sz : 0), '\0');
    if (sz > 0) {
        size_t n = fread(&result[0], 1, static_cast<size_t>(sz), f);
        (void)n; /* byte count used for sizing; truncation is acceptable */
    }
    fclose(f);
    remove(path);
    return result;
}

/* -------------------------------------------------------------------------
 * Helper — build a minimal Entity
 * ---------------------------------------------------------------------- */

static Entity make_entity(const char *id, const char *title,
                           EntityKind kind,
                           const char *status  = "draft",
                           const char *priority = "must")
{
    Entity e{};
    e.identity.id = id;
    e.identity.title = title;
    e.identity.kind = kind;
    e.lifecycle.status = status;
    e.lifecycle.priority = priority;
    return e;
}

/* =========================================================================
 * Tests — Markdown output
 * ======================================================================= */

TEST(ReportMdTest, EmptyListProducesHeadingOnly)
{
    EntityList list;

    std::string out = capture_report(&list, nullptr);

    EXPECT_THAT(out, HasSubstr("# Requirements Report"));
    /* No entity sections expected */
    EXPECT_THAT(out, Not(HasSubstr("###")));
}

TEST(ReportMdTest, SingleRequirementHeadingAndMeta)
{
    EntityList list;

    Entity e = make_entity("REQ-001", "Login required",
                            ENTITY_KIND_REQUIREMENT, "approved", "must");
    list.push_back(e);

    std::string out = capture_report(&list, nullptr);

    EXPECT_THAT(out, HasSubstr("## Requirements (1)"));
    EXPECT_THAT(out, HasSubstr("### REQ-001 — Login required"));
    EXPECT_THAT(out, HasSubstr("**Kind:** requirement"));
    EXPECT_THAT(out, HasSubstr("**Status:** approved"));
    EXPECT_THAT(out, HasSubstr("**Priority:** must"));
}

TEST(ReportMdTest, MultipleKindsProduceMultipleSections)
{
    EntityList list;

    Entity req  = make_entity("REQ-001", "A requirement",
                               ENTITY_KIND_REQUIREMENT);
    Entity tc   = make_entity("TC-001",  "A test case",
                               ENTITY_KIND_TEST_CASE);
    list.push_back(req);
    list.push_back(tc);

    std::string out = capture_report(&list, nullptr);

    EXPECT_THAT(out, HasSubstr("## Requirements (1)"));
    EXPECT_THAT(out, HasSubstr("## Test-cases (1)"));
    EXPECT_THAT(out, HasSubstr("### REQ-001"));
    EXPECT_THAT(out, HasSubstr("### TC-001"));
}

TEST(ReportMdTest, DescriptionAndRationaleRendered)
{
    EntityList list;

    Entity e = make_entity("REQ-002", "Title", ENTITY_KIND_REQUIREMENT);
    strncpy(e.text.description, "The system must do X.",
            sizeof(e.text.description) - 1);
    strncpy(e.text.rationale, "Because of Y.",
            sizeof(e.text.rationale) - 1);
    list.push_back(e);

    std::string out = capture_report(&list, nullptr);

    EXPECT_THAT(out, HasSubstr("The system must do X."));
    EXPECT_THAT(out, HasSubstr("**Rationale:** Because of Y."));
}

TEST(ReportMdTest, TraceabilityLinksRendered)
{
    EntityList list;

    Entity e = make_entity("REQ-010", "Traceable requirement",
                            ENTITY_KIND_REQUIREMENT);
    list.push_back(e);

    /* Build a triplet store with one outgoing and one incoming link. */
    TripletStore *store = triplet_store_create();
    triplet_store_add(store, "REQ-010", "implements", "REQ-020");
    triplet_store_add(store, "STORY-001", "refines", "REQ-010");

    std::string out = capture_report(&list, store, REPORT_FORMAT_MARKDOWN);

    EXPECT_THAT(out, HasSubstr("**Traceability:**"));    EXPECT_THAT(out, HasSubstr("`[implements]` → REQ-020"));
    EXPECT_THAT(out, HasSubstr("`[refines]` ← STORY-001"));

    triplet_store_destroy(store);
}

TEST(ReportMdTest, InferredLinksNotRendered)
{
    EntityList list;

    Entity e = make_entity("REQ-011", "Req", ENTITY_KIND_REQUIREMENT);
    list.push_back(e);

    /* Add a declared triple from TC-001 → REQ-011.  After
     * triplet_store_infer_inverses() the inverse
     * REQ-011 -[implemented-by]→ TC-001 will be marked inferred. */
    TripletStore *store = triplet_store_create();
    triplet_store_add(store, "TC-001", "implements", "REQ-011");
    triplet_store_infer_inverses(store);

    std::string out = capture_report(&list, store, REPORT_FORMAT_MARKDOWN);

    /* The inferred inverse link must not appear in the report for REQ-011's
     * outgoing section (it is only inferred, not declared by REQ-011). */
    EXPECT_THAT(out, Not(HasSubstr("`[implemented-by]`")));

    triplet_store_destroy(store);
}

TEST(ReportMdTest, NoTraceabilitySection_WhenNoLinks)
{
    EntityList list;

    Entity e = make_entity("REQ-012", "Isolated", ENTITY_KIND_REQUIREMENT);
    list.push_back(e);

    TripletStore *store = triplet_store_create();

    std::string out = capture_report(&list, store, REPORT_FORMAT_MARKDOWN);

    EXPECT_THAT(out, Not(HasSubstr("**Traceability:**")));

    triplet_store_destroy(store);
}

TEST(ReportMdTest, UserStoryRendered)
{
    EntityList list;

    Entity e = make_entity("STORY-001", "User login story",
                            ENTITY_KIND_STORY);
    strncpy(e.user_story.role,   "registered user",
            sizeof(e.user_story.role)   - 1);
    strncpy(e.user_story.goal,   "log in with my email",
            sizeof(e.user_story.goal)   - 1);
    strncpy(e.user_story.reason, "access the dashboard",
            sizeof(e.user_story.reason) - 1);
    list.push_back(e);

    std::string out = capture_report(&list, nullptr, REPORT_FORMAT_MARKDOWN);

    EXPECT_THAT(out, HasSubstr("**User Story:**"));
    EXPECT_THAT(out, HasSubstr("As a: registered user"));
    EXPECT_THAT(out, HasSubstr("I want: log in with my email"));
    EXPECT_THAT(out, HasSubstr("So that: access the dashboard"));
}

TEST(ReportMdTest, TagsRendered)
{
    EntityList list;

    Entity e = make_entity("REQ-020", "Tagged req",
                            ENTITY_KIND_REQUIREMENT);
    strncpy(e.tags.tags, "auth\nsecurity", sizeof(e.tags.tags) - 1);
    e.(int)tags.size() = 2;
    list.push_back(e);

    std::string out = capture_report(&list, nullptr, REPORT_FORMAT_MARKDOWN);

    EXPECT_THAT(out, HasSubstr("**Tags:**"));
    EXPECT_THAT(out, HasSubstr("- auth"));
    EXPECT_THAT(out, HasSubstr("- security"));
}

/* =========================================================================
 * Tests — HTML output
 * ======================================================================= */

TEST(ReportHtmlTest, ProducesValidDoctype)
{
    EntityList list;

    std::string out = capture_report(&list, nullptr, REPORT_FORMAT_HTML);

    EXPECT_THAT(out, HasSubstr("<!DOCTYPE html>"));
    EXPECT_THAT(out, HasSubstr("<html"));
    EXPECT_THAT(out, HasSubstr("</html>"));
}

TEST(ReportHtmlTest, EmptyListHasHeading)
{
    EntityList list;

    std::string out = capture_report(&list, nullptr, REPORT_FORMAT_HTML);

    EXPECT_THAT(out, HasSubstr("<h1>Requirements Report</h1>"));
}

TEST(ReportHtmlTest, SingleEntityRenderedInSection)
{
    EntityList list;

    Entity e = make_entity("REQ-100", "My requirement",
                            ENTITY_KIND_REQUIREMENT, "approved", "must");
    list.push_back(e);

    std::string out = capture_report(&list, nullptr, REPORT_FORMAT_HTML);

    EXPECT_THAT(out, HasSubstr("<section class=\"entity\" id=\"REQ-100\""));
    EXPECT_THAT(out, HasSubstr("<h3>REQ-100 — My requirement</h3>"));
    EXPECT_THAT(out, HasSubstr("approved"));
    EXPECT_THAT(out, HasSubstr("must"));
}

TEST(ReportHtmlTest, HtmlSpecialCharsEscaped)
{
    EntityList list;

    Entity e = make_entity("REQ-200", "A < B & C > D",
                            ENTITY_KIND_REQUIREMENT);
    list.push_back(e);

    std::string out = capture_report(&list, nullptr, REPORT_FORMAT_HTML);

    /* Raw unescaped angle brackets must NOT appear in the heading. */
    EXPECT_THAT(out, Not(HasSubstr("A < B")));
    /* Escaped versions must appear. */
    EXPECT_THAT(out, HasSubstr("A &lt; B &amp; C &gt; D"));
}

TEST(ReportHtmlTest, TraceabilityLinksAreAnchors)
{
    EntityList list;

    Entity e = make_entity("REQ-300", "Linked", ENTITY_KIND_REQUIREMENT);
    list.push_back(e);

    TripletStore *store = triplet_store_create();
    triplet_store_add(store, "REQ-300", "verifies", "TC-001");

    std::string out = capture_report(&list, store, REPORT_FORMAT_HTML);

    EXPECT_THAT(out, HasSubstr("<a href=\"#TC-001\">TC-001</a>"));

    triplet_store_destroy(store);
}

TEST(ReportHtmlTest, EntityKindSectionHeading)
{
    EntityList list;

    Entity e = make_entity("TC-400", "Test something",
                            ENTITY_KIND_TEST_CASE);
    list.push_back(e);

    std::string out = capture_report(&list, nullptr, REPORT_FORMAT_HTML);

    EXPECT_THAT(out, HasSubstr("<h2>"));
    EXPECT_THAT(out, HasSubstr("Test-case"));
}

/* =========================================================================
 * Tests — filtered report output (status / kind / priority filtering)
 * ======================================================================= */

TEST(ReportFilterTest, StatusFilterExcludesDraftEntities)
{
    EntityList all;

    Entity approved = make_entity("REQ-A01", "Approved req",
                                   ENTITY_KIND_REQUIREMENT, "approved", "must");
    Entity draft    = make_entity("REQ-D01", "Draft req",
                                   ENTITY_KIND_REQUIREMENT, "draft",    "must");
    all.push_back(approved);
    all.push_back(draft);

    EntityList filtered;
    entity_apply_filter(&all, &filtered, nullptr, nullptr, "approved", nullptr);

    std::string out = capture_report(&filtered, nullptr);

    EXPECT_THAT(out, HasSubstr("REQ-A01"));
    EXPECT_THAT(out, Not(HasSubstr("REQ-D01")));

}

TEST(ReportFilterTest, StatusFilterExcludesApprovedEntities)
{
    EntityList all;

    Entity approved = make_entity("REQ-A02", "Approved req",
                                   ENTITY_KIND_REQUIREMENT, "approved", "must");
    Entity draft    = make_entity("REQ-D02", "Draft req",
                                   ENTITY_KIND_REQUIREMENT, "draft",    "must");
    all.push_back(approved);
    all.push_back(draft);

    EntityList filtered;
    entity_apply_filter(&all, &filtered, nullptr, nullptr, "draft", nullptr);

    std::string out = capture_report(&filtered, nullptr);

    EXPECT_THAT(out, HasSubstr("REQ-D02"));
    EXPECT_THAT(out, Not(HasSubstr("REQ-A02")));

}

TEST(ReportFilterTest, KindFilterOnlyIncludesMatchingKind)
{
    EntityList all;

    Entity req = make_entity("REQ-K01", "A requirement",
                              ENTITY_KIND_REQUIREMENT, "approved", "must");
    Entity tc  = make_entity("TC-K01",  "A test case",
                              ENTITY_KIND_TEST_CASE,   "approved", "must");
    all.push_back(req);
    all.push_back(tc);

    EntityList filtered;
    entity_apply_filter(&all, &filtered, "requirement", nullptr, nullptr, nullptr);

    std::string out = capture_report(&filtered, nullptr);

    EXPECT_THAT(out, HasSubstr("REQ-K01"));
    EXPECT_THAT(out, Not(HasSubstr("TC-K01")));

}

TEST(ReportFilterTest, KindAndStatusFiltersAppliedTogether)
{
    EntityList all;

    Entity req_approved = make_entity("REQ-C01", "Approved req",
                                       ENTITY_KIND_REQUIREMENT, "approved", "must");
    Entity req_draft    = make_entity("REQ-C02", "Draft req",
                                       ENTITY_KIND_REQUIREMENT, "draft",    "must");
    Entity tc_approved  = make_entity("TC-C01",  "Approved TC",
                                       ENTITY_KIND_TEST_CASE,   "approved", "must");
    all.push_back(req_approved);
    all.push_back(req_draft);
    all.push_back(tc_approved);

    EntityList filtered;
    entity_apply_filter(&all, &filtered, "requirement", nullptr, "approved", nullptr);

    std::string out = capture_report(&filtered, nullptr);

    EXPECT_THAT(out, HasSubstr("REQ-C01"));
    EXPECT_THAT(out, Not(HasSubstr("REQ-C02")));
    EXPECT_THAT(out, Not(HasSubstr("TC-C01")));

}

TEST(ReportFilterTest, PriorityFilterOnlyIncludesMatchingPriority)
{
    EntityList all;

    Entity must_req   = make_entity("REQ-P01", "Must req",
                                     ENTITY_KIND_REQUIREMENT, "approved", "must");
    Entity should_req = make_entity("REQ-P02", "Should req",
                                     ENTITY_KIND_REQUIREMENT, "approved", "should");
    all.push_back(must_req);
    all.push_back(should_req);

    EntityList filtered;
    entity_apply_filter(&all, &filtered, nullptr, nullptr, nullptr, "must");

    std::string out = capture_report(&filtered, nullptr);

    EXPECT_THAT(out, HasSubstr("REQ-P01"));
    EXPECT_THAT(out, Not(HasSubstr("REQ-P02")));

}

TEST(ReportFilterTest, NoMatchingStatusProducesEmptyEntitySection)
{
    EntityList all;

    Entity draft = make_entity("REQ-E01", "Draft req",
                                ENTITY_KIND_REQUIREMENT, "draft", "must");
    all.push_back(draft);

    EntityList filtered;
    entity_apply_filter(&all, &filtered, nullptr, nullptr, "approved", nullptr);

    std::string out = capture_report(&filtered, nullptr);

    /* No entity headings when none match the filter. */
    EXPECT_THAT(out, Not(HasSubstr("REQ-E01")));
    EXPECT_THAT(out, Not(HasSubstr("###")));

}

TEST(ReportFilterTest, HtmlOutputAlsoRespectsStatusFilter)
{
    EntityList all;

    Entity approved = make_entity("REQ-H01", "Approved req",
                                   ENTITY_KIND_REQUIREMENT, "approved", "must");
    Entity draft    = make_entity("REQ-H02", "Draft req",
                                   ENTITY_KIND_REQUIREMENT, "draft",    "must");
    all.push_back(approved);
    all.push_back(draft);

    EntityList filtered;
    entity_apply_filter(&all, &filtered, nullptr, nullptr, "approved", nullptr);

    std::string out = capture_report(&filtered, nullptr, REPORT_FORMAT_HTML);

    EXPECT_THAT(out, HasSubstr("REQ-H01"));
    EXPECT_THAT(out, Not(HasSubstr("REQ-H02")));

}
