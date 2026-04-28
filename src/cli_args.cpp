/**
 * @file cli_args.c
 * @brief Command-line argument parsing for the vibe-req CLI.
 *
 * Implements cli_parse_args() and cli_print_help() declared in cli_args.h.
 * The parsing logic is extracted from main() to make it independently
 * testable and to reduce the size of the main function.
 */

#include "cli_args.h"

#include <cstdio>
#include <cstring>

/* -------------------------------------------------------------------------
 * Help text
 * ---------------------------------------------------------------------- */

void cli_print_help(const char *prog)
{
    printf("Usage: %s [command] [options] [directory]\n\n", prog);
    printf("Commands:\n");
    printf("  (default)       List all requirements found in the directory tree.\n");
    printf("  list            List all entities (all kinds) with optional filters.\n");
    printf("  entities        Alias for 'list'.\n");
    printf("  links           List all relations parsed from requirement files.\n");
    printf("  trace <id>      Show full traceability chain for an entity.\n");
    printf("                  Displays entity info, outgoing links, and incoming links.\n");
    printf("                  Example: %s trace REQ-001\n", prog);
    printf("  coverage        Report how many requirements have traceability links\n");
    printf("                  to tests or code (verified-by, implemented-by, etc.).\n");
    printf("                  Example: %s coverage\n", prog);
    printf("  status          Summarise entity counts by status, priority, and kind.\n");
    printf("                  Example: %s status\n", prog);
    printf("  orphan          List requirements and test cases with no traceability\n");
    printf("                  links in either direction.\n");
    printf("                  Example: %s orphan\n", prog);
    printf("  report          Generate a Markdown report of all entities.\n");
    printf("                  Use --output and filter flags to customise output.\n");
    printf("                  Example: %s report --output report.md\n\n", prog);
    printf("  doc <id>        Print a document entity (SRS/SDD) together with all\n");
    printf("                  entities that are part of it.\n");
    printf("                  Example: %s doc SRS-CLIENT-001 --output srs.md\n\n", prog);
    printf("  validate        Check the repository for consistency problems:\n");
    printf("                  duplicate entity IDs and links to unknown entities.\n");
    printf("                  Exits with a non-zero code if any problems are found.\n");
    printf("                  Example: %s validate\n\n", prog);
    printf("  new <type> <id> Scaffold a new entity YAML file named <id>.yaml.\n");
    printf("                  Types: requirement, group, story, design-note,\n");
    printf("                         section, assumption, constraint, test-case,\n");
    printf("                         external, document, srs, sdd,\n");
    printf("                         document-schema\n");
    printf("                  Example: %s new requirement REQ-AUTH-003\n\n", prog);
    printf("Filter options (for 'list' / 'entities' / 'report'):\n");
    printf("  --kind <kind>        Show only entities of the given kind.\n");
    printf("                       Kinds: requirement, group, story, design-note,\n");
    printf("                              section, assumption, constraint, test-case,\n");
    printf("                              external, document, document-schema\n");
    printf("  --component <comp>   Show only entities that carry the named component.\n");
    printf("                       Components: user-story, acceptance-criteria, epic,\n");
    printf("                                  assumption, constraint, doc-meta,\n");
    printf("                                  doc-membership, doc-body, traceability,\n");
    printf("                                  tags, test-procedure, clause-collection,\n");
    printf("                                  attachment, applies-to, variant-profile,\n");
    printf("                                  composition-profile, render-profile\n");
    printf("  --status <status>    Show only entities with the given lifecycle status.\n");
    printf("  --priority <prio>    Show only entities with the given priority.\n\n");
    printf("Report options (for 'report' / 'doc'):\n");
    printf("  --format md     Output Markdown (default).\n");
    printf("  --format html   Output a self-contained HTML document.\n");
    printf("  --output <file> Write report to <file> instead of stdout.\n\n");
    printf("Other options:\n");
    printf("  --strict-links  Warn when a known relation is declared in only one\n");
    printf("                  direction (inverse not explicitly present in YAML).\n");
    printf("                  Exits with a non-zero code if any warnings are found.\n");
    printf("  directory       Root directory to scan (default: current directory).\n\n");
    printf("  YAML files without a top-level 'id' field are silently ignored.\n");
    printf("  Traceability links may be declared under either 'traceability:' or\n");
    printf("  'links:' YAML keys; both formats are recognised.\n");
}

/* -------------------------------------------------------------------------
 * Argument parsing
 * ---------------------------------------------------------------------- */

void cli_parse_args(int argc, char *argv[], CliOptions *opts)
{
    /* Zero-initialise then set defaults. */
    memset(opts, 0, sizeof(*opts));
    opts->root          = ".";
    opts->report_format = REPORT_FORMAT_MARKDOWN;

    if (argc < 2)
        return; /* no arguments — use all defaults */

    /* ------------------------------------------------------------------ */
    /* Step 1: detect -h / --help first (highest priority)               */
    /* ------------------------------------------------------------------ */
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        opts->show_help = 1;
        return;
    }

    /* ------------------------------------------------------------------ */
    /* Step 2: detect subcommand in argv[1]                              */
    /* ------------------------------------------------------------------ */
    int arg_idx = 1; /* next index to process after subcommand detection */

    if (strcmp(argv[1], "links") == 0) {
        opts->show_links = 1;
        arg_idx = 2;
    } else if (strcmp(argv[1], "entities") == 0 ||
               strcmp(argv[1], "list") == 0) {
        opts->show_entities = 1;
        arg_idx = 2;
    } else if (strcmp(argv[1], "trace") == 0) {
        if (argc < 3) {
            opts->parse_error = 1;
            opts->error_msg   = "error: 'trace' requires an entity ID argument";
            return;
        }
        opts->trace_id = argv[2];
        arg_idx = 3;
    } else if (strcmp(argv[1], "coverage") == 0) {
        opts->show_coverage = 1;
        arg_idx = 2;
    } else if (strcmp(argv[1], "status") == 0) {
        opts->show_status = 1;
        arg_idx = 2;
    } else if (strcmp(argv[1], "orphan") == 0) {
        opts->show_orphan = 1;
        arg_idx = 2;
    } else if (strcmp(argv[1], "report") == 0) {
        opts->show_report = 1;
        arg_idx = 2;
    } else if (strcmp(argv[1], "doc") == 0) {
        if (argc < 3) {
            opts->parse_error = 1;
            opts->error_msg   = "error: 'doc' requires a document ID argument";
            return;
        }
        opts->is_doc_cmd = 1;
        opts->doc_id     = argv[2];
        arg_idx = 3;
    } else if (strcmp(argv[1], "validate") == 0) {
        opts->is_validate_cmd = 1;
        arg_idx = 2;
    } else if (strcmp(argv[1], "new") == 0) {
        if (argc < 4) {
            opts->parse_error = 1;
            opts->error_msg   = "error: 'new' requires a type and an ID argument";
            return;
        }
        opts->is_new_cmd = 1;
        opts->new_type   = argv[2];
        opts->new_id     = argv[3];
        opts->new_dir    = (argc >= 5) ? argv[4] : ".";
        return; /* no further flags to parse for 'new' */
    }
    /* else: no recognised subcommand — treat argv[1] onward as flags/dir */

    /* ------------------------------------------------------------------ */
    /* Step 3: scan remaining arguments for flags and directory           */
    /* ------------------------------------------------------------------ */
    for (int i = arg_idx; i < argc; i++) {
        if (strcmp(argv[i], "--strict-links") == 0) {
            opts->strict_links = 1;
        } else if (strcmp(argv[i], "--kind") == 0) {
            if (i + 1 >= argc) {
                opts->parse_error = 1;
                opts->error_msg   = "error: '--kind' requires a value";
                return;
            }
            opts->filter_kind = argv[++i];
        } else if (strcmp(argv[i], "--component") == 0) {
            if (i + 1 >= argc) {
                opts->parse_error = 1;
                opts->error_msg   = "error: '--component' requires a value";
                return;
            }
            opts->filter_comp = argv[++i];
        } else if (strcmp(argv[i], "--status") == 0) {
            if (i + 1 >= argc) {
                opts->parse_error = 1;
                opts->error_msg   = "error: '--status' requires a value";
                return;
            }
            opts->filter_status = argv[++i];
        } else if (strcmp(argv[i], "--priority") == 0) {
            if (i + 1 >= argc) {
                opts->parse_error = 1;
                opts->error_msg   = "error: '--priority' requires a value";
                return;
            }
            opts->filter_priority = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                opts->parse_error = 1;
                opts->error_msg   = "error: '--output' requires a file path";
                return;
            }
            opts->report_output = argv[++i];
        } else if (strcmp(argv[i], "--format") == 0) {
            if (i + 1 >= argc) {
                opts->parse_error = 1;
                opts->error_msg   = "error: '--format' requires 'md' or 'html'";
                return;
            }
            i++;
            if (strcmp(argv[i], "html") == 0) {
                opts->report_format = REPORT_FORMAT_HTML;
            } else if (strcmp(argv[i], "md") == 0) {
                opts->report_format = REPORT_FORMAT_MARKDOWN;
            } else {
                opts->parse_error = 1;
                opts->error_msg   =
                    "error: unknown format value; use 'md' or 'html'";
                return;
            }
        } else {
            /* Positional argument — treat as the root directory. */
            opts->root = argv[i];
        }
    }
}
