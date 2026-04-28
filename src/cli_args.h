/**
 * @file cli_args.h
 * @brief Command-line argument parsing for the vibe-req CLI.
 *
 * Provides a CliOptions structure and the cli_parse_args() / cli_print_help()
 * functions that handle all argument parsing for the vibe-req binary.
 *
 * Separating argument parsing from command dispatch allows the parsing logic
 * to be unit-tested independently of the I/O-heavy command implementations.
 */

#ifndef VIBE_CLI_ARGS_H
#define VIBE_CLI_ARGS_H

#include "report.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parsed command-line options for a single vibe-req invocation.
 *
 * Populated by cli_parse_args().  All string fields point directly into the
 * original argv[] array and must never be freed by the caller.
 *
 * After parsing, the caller should:
 *   1. Check parse_error — if non-zero, print error_msg and exit(1).
 *   2. Check show_help  — if set, call cli_print_help() and return 0.
 *   3. Check is_new_cmd — if set, handle the 'new' subcommand.
 *   4. Check is_doc_cmd — if set, handle the 'doc' subcommand.
 *   5. Otherwise dispatch the appropriate command using the flag fields.
 */
typedef struct {
    /* ------------------------------------------------------------------ */
    /* Subcommand flags                                                    */
    /* ------------------------------------------------------------------ */

    /** Non-zero when the 'links' subcommand was requested. */
    int         show_links;

    /** Non-zero when --strict-links flag was present. */
    int         strict_links;

    /** Non-zero when 'list' or 'entities' subcommand was requested. */
    int         show_entities;

    /** Non-zero when 'coverage' subcommand was requested. */
    int         show_coverage;

    /** Non-zero when 'status' subcommand was requested. */
    int         show_status;

    /** Non-zero when 'orphan' subcommand was requested. */
    int         show_orphan;

    /** Non-zero when 'report' subcommand was requested. */
    int         show_report;

    /**
     * Non-zero when the 'doc' subcommand was detected.
     * The caller is responsible for resolving the target document and
     * rendering it with report_write().
     */
    int         is_doc_cmd;

    /** Non-zero when the 'validate' subcommand was requested. */
    int         is_validate_cmd;

    /** Document ID argument for 'doc'; valid when is_doc_cmd is set. */
    const char *doc_id;

    /**
     * Entity ID argument for the 'trace' subcommand.
     * NULL when the trace subcommand was not requested.
     */
    const char *trace_id;

    /* ------------------------------------------------------------------ */
    /* Filter flags (for 'list', 'entities', and 'report')               */
    /* ------------------------------------------------------------------ */

    /** EntityKind label from --kind <kind>; NULL if not provided. */
    const char *filter_kind;

    /** Component name from --component <comp>; NULL if not provided. */
    const char *filter_comp;

    /** Lifecycle status from --status <status>; NULL if not provided. */
    const char *filter_status;

    /** Lifecycle priority from --priority <prio>; NULL if not provided. */
    const char *filter_priority;

    /* ------------------------------------------------------------------ */
    /* Report options (for 'report' / 'doc')                              */
    /* ------------------------------------------------------------------ */

    /** Output file path from --output <file>; NULL means stdout. */
    const char  *report_output;

    /** Report format from --format md|html; default REPORT_FORMAT_MARKDOWN. */
    ReportFormat report_format;

    /* ------------------------------------------------------------------ */
    /* Positional argument                                                 */
    /* ------------------------------------------------------------------ */

    /** Root directory to scan; defaults to "." when not provided. */
    const char *root;

    /* ------------------------------------------------------------------ */
    /* Special: 'new' subcommand                                          */
    /* ------------------------------------------------------------------ */

    /**
     * Non-zero when the 'new' subcommand was detected.
     * The caller is responsible for dispatching new_cmd_scaffold().
     */
    int         is_new_cmd;

    /** Entity type argument for 'new'; valid when is_new_cmd is set. */
    const char *new_type;

    /** Entity ID argument for 'new'; valid when is_new_cmd is set. */
    const char *new_id;

    /** Directory argument for 'new'; defaults to "." when not provided. */
    const char *new_dir;

    /* ------------------------------------------------------------------ */
    /* Help and error state                                               */
    /* ------------------------------------------------------------------ */

    /** Non-zero when -h or --help was requested. */
    int         show_help;

    /**
     * Non-zero when a parse error was detected.
     * error_msg contains a human-readable description.
     */
    int         parse_error;

    /** Human-readable error message; NULL when parse_error is zero. */
    const char *error_msg;

} CliOptions;

/**
 * Print the full vibe-req usage text to stdout.
 *
 * @param prog  argv[0] (used in the usage line and examples)
 */
void cli_print_help(const char *prog);

/**
 * Parse command-line arguments into @p opts.
 *
 * All fields of *opts are initialised before parsing begins; callers do not
 * need to zero the struct before calling.
 *
 * On success  opts->parse_error == 0.
 * On error    opts->parse_error != 0 and opts->error_msg is set.
 *
 * @param argc  argument count from main()
 * @param argv  argument vector from main()
 * @param opts  output; must not be NULL
 */
void cli_parse_args(int argc, char *argv[], CliOptions *opts);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VIBE_CLI_ARGS_H */
