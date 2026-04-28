/**
 * @file test_cli_args.cpp
 * @brief Unit tests for cli_args.c — cli_parse_args() and cli_print_help().
 *
 * Tests cover:
 *   - Default values when no arguments are given
 *   - Help flag detection (-h and --help)
 *   - Each subcommand (links, list, entities, trace, coverage, orphan,
 *     report, doc, new)
 *   - All filter flags (--kind, --component, --status, --priority)
 *   - Report flags (--output, --format md, --format html)
 *   - --strict-links flag
 *   - Positional directory argument
 *   - Error cases (missing required argument values, bad format, etc.)
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include "cli_args.h"
#include "report.h"
}

using ::testing::HasSubstr;

/* -------------------------------------------------------------------------
 * Helper — build an argv array from a list of string literals
 * ---------------------------------------------------------------------- */

/**
 * Thin wrapper that holds a vector<char*> (non-const, as main() expects).
 * The strings in args are string literals (static storage) so no copy needed.
 */
struct Argv {
    std::vector<char *> v;

    explicit Argv(std::initializer_list<const char *> args)
    {
        for (const char *a : args)
            v.push_back(const_cast<char *>(a));
    }

    int    argc() const { return static_cast<int>(v.size()); }
    char **argv() const { return const_cast<char **>(v.data()); }
};

/* =========================================================================
 * Tests — defaults
 * ======================================================================= */

TEST(CliParseArgsTest, NoArgumentsGivesDefaults)
{
    Argv a{"vibe-req"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);

    EXPECT_EQ(opts.show_help,     0);
    EXPECT_EQ(opts.parse_error,   0);
    EXPECT_EQ(opts.show_links,    0);
    EXPECT_EQ(opts.strict_links,  0);
    EXPECT_EQ(opts.show_entities, 0);
    EXPECT_EQ(opts.show_coverage, 0);
    EXPECT_EQ(opts.show_orphan,   0);
    EXPECT_EQ(opts.show_report,   0);
    EXPECT_EQ(opts.is_doc_cmd,    0);
    EXPECT_EQ(opts.is_new_cmd,    0);
    EXPECT_EQ(opts.doc_id,        nullptr);
    EXPECT_EQ(opts.trace_id,      nullptr);
    EXPECT_STREQ(opts.root,       ".");
    EXPECT_EQ(opts.filter_kind,     nullptr);
    EXPECT_EQ(opts.filter_comp,     nullptr);
    EXPECT_EQ(opts.filter_status,   nullptr);
    EXPECT_EQ(opts.filter_priority, nullptr);
    EXPECT_EQ(opts.report_output,   nullptr);
    EXPECT_EQ(opts.report_format,   REPORT_FORMAT_MARKDOWN);
}

/* =========================================================================
 * Tests — help flag
 * ======================================================================= */

TEST(CliParseArgsTest, ShortHelpFlagSetsShowHelp)
{
    Argv a{"vibe-req", "-h"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.show_help,   1);
    EXPECT_EQ(opts.parse_error, 0);
}

TEST(CliParseArgsTest, LongHelpFlagSetsShowHelp)
{
    Argv a{"vibe-req", "--help"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.show_help,   1);
    EXPECT_EQ(opts.parse_error, 0);
}

/* =========================================================================
 * Tests — subcommands
 * ======================================================================= */

TEST(CliParseArgsTest, LinksSubcommand)
{
    Argv a{"vibe-req", "links"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.show_links, 1);
    EXPECT_EQ(opts.parse_error, 0);
}

TEST(CliParseArgsTest, ListSubcommand)
{
    Argv a{"vibe-req", "list"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.show_entities, 1);
    EXPECT_EQ(opts.parse_error, 0);
}

TEST(CliParseArgsTest, EntitiesSubcommandAliasForList)
{
    Argv a{"vibe-req", "entities"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.show_entities, 1);
    EXPECT_EQ(opts.parse_error, 0);
}

TEST(CliParseArgsTest, TraceSubcommandSetsTraceId)
{
    Argv a{"vibe-req", "trace", "REQ-001"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_STREQ(opts.trace_id, "REQ-001");
}

TEST(CliParseArgsTest, TraceMissingIdGivesParseError)
{
    Argv a{"vibe-req", "trace"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_NE(opts.parse_error, 0);
    EXPECT_NE(opts.error_msg, nullptr);
}

TEST(CliParseArgsTest, CoverageSubcommand)
{
    Argv a{"vibe-req", "coverage"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.show_coverage, 1);
    EXPECT_EQ(opts.parse_error, 0);
}

TEST(CliParseArgsTest, OrphanSubcommand)
{
    Argv a{"vibe-req", "orphan"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.show_orphan, 1);
    EXPECT_EQ(opts.parse_error, 0);
}

TEST(CliParseArgsTest, ReportSubcommand)
{
    Argv a{"vibe-req", "report"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.show_report, 1);
    EXPECT_EQ(opts.parse_error, 0);
}

TEST(CliParseArgsTest, DocSubcommandSetsDocumentId)
{
    Argv a{"vibe-req", "doc", "SRS-001"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.is_doc_cmd, 1);
    EXPECT_STREQ(opts.doc_id, "SRS-001");
}

TEST(CliParseArgsTest, DocMissingIdGivesParseError)
{
    Argv a{"vibe-req", "doc"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_NE(opts.parse_error, 0);
    EXPECT_NE(opts.error_msg, nullptr);
}

/* =========================================================================
 * Tests — 'new' subcommand
 * ======================================================================= */

TEST(CliParseArgsTest, NewSubcommandSetsFields)
{
    Argv a{"vibe-req", "new", "requirement", "REQ-042"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.is_new_cmd, 1);
    EXPECT_STREQ(opts.new_type, "requirement");
    EXPECT_STREQ(opts.new_id,   "REQ-042");
    EXPECT_STREQ(opts.new_dir,  ".");
}

TEST(CliParseArgsTest, NewSubcommandWithDirectory)
{
    Argv a{"vibe-req", "new", "test-case", "TC-001", "requirements/"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.is_new_cmd, 1);
    EXPECT_STREQ(opts.new_dir, "requirements/");
}

TEST(CliParseArgsTest, NewSubcommandMissingArgsGivesParseError)
{
    Argv a{"vibe-req", "new", "requirement"}; /* missing id */
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_NE(opts.parse_error, 0);
}

/* =========================================================================
 * Tests — filter flags
 * ======================================================================= */

TEST(CliParseArgsTest, KindFilterFlag)
{
    Argv a{"vibe-req", "list", "--kind", "requirement"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_STREQ(opts.filter_kind, "requirement");
}

TEST(CliParseArgsTest, ComponentFilterFlag)
{
    Argv a{"vibe-req", "list", "--component", "traceability"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_STREQ(opts.filter_comp, "traceability");
}

TEST(CliParseArgsTest, StatusFilterFlag)
{
    Argv a{"vibe-req", "list", "--status", "approved"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_STREQ(opts.filter_status, "approved");
}

TEST(CliParseArgsTest, PriorityFilterFlag)
{
    Argv a{"vibe-req", "list", "--priority", "must"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_STREQ(opts.filter_priority, "must");
}

TEST(CliParseArgsTest, MultipleFilterFlagsTogether)
{
    Argv a{"vibe-req", "list", "--kind", "test-case",
           "--status", "draft", "--priority", "should"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_STREQ(opts.filter_kind,     "test-case");
    EXPECT_STREQ(opts.filter_status,   "draft");
    EXPECT_STREQ(opts.filter_priority, "should");
}

TEST(CliParseArgsTest, KindFlagMissingValueGivesParseError)
{
    Argv a{"vibe-req", "--kind"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_NE(opts.parse_error, 0);
    EXPECT_NE(opts.error_msg, nullptr);
}

TEST(CliParseArgsTest, ComponentFlagMissingValueGivesParseError)
{
    Argv a{"vibe-req", "--component"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_NE(opts.parse_error, 0);
}

TEST(CliParseArgsTest, StatusFlagMissingValueGivesParseError)
{
    Argv a{"vibe-req", "--status"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_NE(opts.parse_error, 0);
}

TEST(CliParseArgsTest, PriorityFlagMissingValueGivesParseError)
{
    Argv a{"vibe-req", "--priority"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_NE(opts.parse_error, 0);
}

/* =========================================================================
 * Tests — report flags
 * ======================================================================= */

TEST(CliParseArgsTest, OutputFlag)
{
    Argv a{"vibe-req", "report", "--output", "out.md"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_STREQ(opts.report_output, "out.md");
}

TEST(CliParseArgsTest, OutputFlagMissingValueGivesParseError)
{
    Argv a{"vibe-req", "report", "--output"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_NE(opts.parse_error, 0);
}

TEST(CliParseArgsTest, FormatFlagMarkdown)
{
    Argv a{"vibe-req", "report", "--format", "md"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.report_format, REPORT_FORMAT_MARKDOWN);
}

TEST(CliParseArgsTest, FormatFlagHtml)
{
    Argv a{"vibe-req", "report", "--format", "html"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.report_format, REPORT_FORMAT_HTML);
}

TEST(CliParseArgsTest, FormatFlagUnknownValueGivesParseError)
{
    Argv a{"vibe-req", "report", "--format", "pdf"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_NE(opts.parse_error, 0);
}

TEST(CliParseArgsTest, FormatFlagMissingValueGivesParseError)
{
    Argv a{"vibe-req", "report", "--format"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_NE(opts.parse_error, 0);
}

/* =========================================================================
 * Tests — report command with filter flags
 * ======================================================================= */

TEST(CliParseArgsTest, ReportWithStatusFilter)
{
    Argv a{"vibe-req", "report", "--status", "approved"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.show_report, 1);
    EXPECT_STREQ(opts.filter_status, "approved");
}

TEST(CliParseArgsTest, ReportWithKindFilter)
{
    Argv a{"vibe-req", "report", "--kind", "requirement"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.show_report, 1);
    EXPECT_STREQ(opts.filter_kind, "requirement");
}

TEST(CliParseArgsTest, ReportWithPriorityFilter)
{
    Argv a{"vibe-req", "report", "--priority", "must"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.show_report, 1);
    EXPECT_STREQ(opts.filter_priority, "must");
}

TEST(CliParseArgsTest, ReportWithComponentFilter)
{
    Argv a{"vibe-req", "report", "--component", "traceability"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.show_report, 1);
    EXPECT_STREQ(opts.filter_comp, "traceability");
}

TEST(CliParseArgsTest, ReportWithMultipleFilters)
{
    Argv a{"vibe-req", "report", "--kind", "requirement",
           "--status", "approved", "--priority", "must"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.show_report, 1);
    EXPECT_STREQ(opts.filter_kind,     "requirement");
    EXPECT_STREQ(opts.filter_status,   "approved");
    EXPECT_STREQ(opts.filter_priority, "must");
}

TEST(CliParseArgsTest, ReportWithFiltersAndFormat)
{
    Argv a{"vibe-req", "report", "--status", "approved",
           "--format", "html", "--output", "approved.html"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.show_report, 1);
    EXPECT_STREQ(opts.filter_status, "approved");
    EXPECT_EQ(opts.report_format, REPORT_FORMAT_HTML);
    EXPECT_STREQ(opts.report_output, "approved.html");
}

TEST(CliParseArgsTest, ReportWithFiltersAndDirectory)
{
    Argv a{"vibe-req", "report", "--status", "draft", "requirements/"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.show_report, 1);
    EXPECT_STREQ(opts.filter_status, "draft");
    EXPECT_STREQ(opts.root, "requirements/");
}

TEST(CliParseArgsTest, DocWithFormatOutputAndDirectory)
{
    Argv a{"vibe-req", "doc", "SRS-001", "--format", "html",
           "--output", "srs.html", "requirements/"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.is_doc_cmd, 1);
    EXPECT_STREQ(opts.doc_id, "SRS-001");
    EXPECT_EQ(opts.report_format, REPORT_FORMAT_HTML);
    EXPECT_STREQ(opts.report_output, "srs.html");
    EXPECT_STREQ(opts.root, "requirements/");
}

/* =========================================================================
 * Tests — --strict-links and directory argument
 * ======================================================================= */

TEST(CliParseArgsTest, StrictLinksFlag)
{
    Argv a{"vibe-req", "--strict-links"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.strict_links, 1);
}

TEST(CliParseArgsTest, DirectoryPositionalArgument)
{
    Argv a{"vibe-req", "requirements/"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_STREQ(opts.root, "requirements/");
}

TEST(CliParseArgsTest, DirectoryAfterSubcommand)
{
    Argv a{"vibe-req", "list", "requirements/"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.show_entities, 1);
    EXPECT_STREQ(opts.root, "requirements/");
}

TEST(CliParseArgsTest, FlagsAndDirectoryTogether)
{
    Argv a{"vibe-req", "list", "--kind", "requirement",
           "--status", "approved", "docs/requirements"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.show_entities, 1);
    EXPECT_STREQ(opts.filter_kind,   "requirement");
    EXPECT_STREQ(opts.filter_status, "approved");
    EXPECT_STREQ(opts.root,          "docs/requirements");
}

/* =========================================================================
 * Tests — validate subcommand
 * ======================================================================= */

TEST(CliParseArgsTest, ValidateSubcommandSetsFlag)
{
    Argv a{"vibe-req", "validate"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.is_validate_cmd, 1);
}

TEST(CliParseArgsTest, ValidateSubcommandWithDirectory)
{
    Argv a{"vibe-req", "validate", "requirements/"};
    CliOptions opts;
    cli_parse_args(a.argc(), a.argv(), &opts);
    EXPECT_EQ(opts.parse_error, 0);
    EXPECT_EQ(opts.is_validate_cmd, 1);
    EXPECT_STREQ(opts.root, "requirements/");
}

/* =========================================================================
 * Tests — cli_print_help (smoke test)
 * ======================================================================= */

TEST(CliPrintHelpTest, PrintsUsageLine)
{
    char path[] = "/tmp/test_cli_help_XXXXXX";
    int fd = mkstemp(path);
    ASSERT_GE(fd, 0);
    close(fd);

    FILE *old = freopen(path, "w", stdout);
    ASSERT_NE(old, nullptr);

    cli_print_help("vibe-req");
    fflush(stdout);
    freopen("/dev/tty", "w", stdout);

    FILE *f = fopen(path, "r");
    ASSERT_NE(f, nullptr);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string out(static_cast<size_t>(sz > 0 ? sz : 0), '\0');
    if (sz > 0) fread(&out[0], 1, static_cast<size_t>(sz), f);
    fclose(f);
    remove(path);

    EXPECT_THAT(out, HasSubstr("Usage:"));
    EXPECT_THAT(out, HasSubstr("vibe-req"));
    EXPECT_THAT(out, HasSubstr("list"));
    EXPECT_THAT(out, HasSubstr("doc <id>"));
    EXPECT_THAT(out, HasSubstr("coverage"));
    EXPECT_THAT(out, HasSubstr("orphan"));
    EXPECT_THAT(out, HasSubstr("validate"));
    EXPECT_THAT(out, HasSubstr("--kind"));
    EXPECT_THAT(out, HasSubstr("--strict-links"));
}
