#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entity.h"
#include "discovery.h"
#include "config.h"
#include "yaml_simple.h"
#include "triplet_store_c.h"
#include "report.h"
#include "new_cmd.h"
#include "coverage.h"
#include "cli_args.h"
#include "list_cmd.h"

int main(int argc, char *argv[])
{
    CliOptions opts;
    cli_parse_args(argc, argv, &opts);

    /* ------------------------------------------------------------------ */
    /* Handle -h / --help                                                  */
    /* ------------------------------------------------------------------ */
    if (opts.show_help) {
        cli_print_help(argv[0]);
        return 0;
    }

    /* ------------------------------------------------------------------ */
    /* Handle parse errors                                                 */
    /* ------------------------------------------------------------------ */
    if (opts.parse_error) {
        fprintf(stderr, "%s\n", opts.error_msg);
        if (opts.is_new_cmd)
            fprintf(stderr, "usage: %s new <type> <id> [directory]\n", argv[0]);
        if (opts.is_doc_cmd)
            fprintf(stderr,
                    "usage: %s doc <doc-id> [--format md|html] "
                    "[--output <file>] [directory]\n",
                    argv[0]);
        return 1;
    }

    /* ------------------------------------------------------------------ */
    /* Handle 'new' subcommand                                             */
    /* ------------------------------------------------------------------ */
    if (opts.is_new_cmd) {
        int rc = new_cmd_scaffold(opts.new_type, opts.new_id, opts.new_dir);
        if (rc == -3) {
            fprintf(stderr, "error: unrecognised entity type '%s'\n"
                    "Valid types: requirement, group, story, design-note,\n"
                    "             section, assumption, constraint, test-case,\n"
                    "             external, document, srs, sdd,\n"
                    "             document-schema\n",
                    opts.new_type);
            return 1;
        }
        if (rc == -1) {
            fprintf(stderr, "error: file '%s/%s.yaml' already exists\n",
                    opts.new_dir, opts.new_id);
            return 1;
        }
        if (rc == -2) {
            fprintf(stderr, "error: cannot create '%s/%s.yaml'\n",
                    opts.new_dir, opts.new_id);
            return 1;
        }
        printf("Created %s/%s.yaml\n", opts.new_dir, opts.new_id);
        return 0;
    }

    /* ------------------------------------------------------------------ */
    /* All remaining subcommands require entity discovery.                 */
    /* ------------------------------------------------------------------ */
    EntityList elist;
    entity_list_init(&elist);

    VibeConfig cfg;
    /* config_load() returns -1 when .vibe-req.yaml is absent or unparseable,
     * which is perfectly valid — cfg is zeroed so discovery runs unfiltered. */
    config_load(opts.root, &cfg);

    int found = discover_entities(opts.root, &elist, &cfg);
    if (found < 0) {
        fprintf(stderr, "error: cannot open directory '%s'\n", opts.root);
        entity_list_free(&elist);
        return 1;
    }

    /* Sort entities alphabetically by ID before displaying. */
    if (elist.count > 1)
        qsort(elist.items, (size_t)elist.count, sizeof(Entity),
              entity_cmp_by_id);

    /* ------------------------------------------------------------------ */
    /* 'list' / 'entities' subcommand                                      */
    /* ------------------------------------------------------------------ */
    if (opts.show_entities) {
        if (opts.filter_kind || opts.filter_comp ||
            opts.filter_status || opts.filter_priority) {
            EntityList filtered;
            entity_list_init(&filtered);
            entity_apply_filter(&elist, &filtered,
                                opts.filter_kind, opts.filter_comp,
                                opts.filter_status, opts.filter_priority);
            list_entities(&filtered);
            entity_list_free(&filtered);
        } else {
            list_entities(&elist);
        }
        entity_list_free(&elist);
        return 0;
    }

    /* ------------------------------------------------------------------ */
    /* 'report' subcommand                                                 */
    /* ------------------------------------------------------------------ */
    if (opts.show_report) {
        TripletStore *store = build_entity_relation_store(&elist);
        if (!store) {
            fprintf(stderr, "error: failed to create relation store\n");
            entity_list_free(&elist);
            return 1;
        }

        /* Apply filters when provided. */
        EntityList *src = &elist;
        EntityList  filtered;
        entity_list_init(&filtered);
        if (opts.filter_kind || opts.filter_comp ||
            opts.filter_status || opts.filter_priority) {
            entity_apply_filter(&elist, &filtered,
                                opts.filter_kind, opts.filter_comp,
                                opts.filter_status, opts.filter_priority);
            src = &filtered;
        }

        FILE *out = stdout;
        if (opts.report_output) {
            out = fopen(opts.report_output, "w");
            if (!out) {
                fprintf(stderr, "error: cannot open output file '%s'\n",
                        opts.report_output);
                entity_list_free(&filtered);
                triplet_store_destroy(store);
                entity_list_free(&elist);
                return 1;
            }
        }

        report_write(out, src, store, opts.report_format);

        if (opts.report_output)
            fclose(out);

        entity_list_free(&filtered);
        triplet_store_destroy(store);
        entity_list_free(&elist);
        return 0;
    }

    /* ------------------------------------------------------------------ */
    /* 'doc' subcommand                                                    */
    /* ------------------------------------------------------------------ */
    if (opts.is_doc_cmd) {
        TripletStore *store = build_entity_relation_store(&elist);
        if (!store) {
            fprintf(stderr, "error: failed to create relation store\n");
            entity_list_free(&elist);
            return 1;
        }

        EntityList doc_entities;
        entity_list_init(&doc_entities);

        int rc = collect_document_entities(&elist, store, opts.doc_id,
                                           &doc_entities);
        if (rc == -1) {
            fprintf(stderr, "error: document '%s' not found\n", opts.doc_id);
            triplet_store_destroy(store);
            entity_list_free(&doc_entities);
            entity_list_free(&elist);
            return 1;
        }
        if (rc == -2) {
            fprintf(stderr, "error: entity '%s' is not a document\n",
                    opts.doc_id);
            triplet_store_destroy(store);
            entity_list_free(&doc_entities);
            entity_list_free(&elist);
            return 1;
        }
        if (rc != 0) {
            fprintf(stderr,
                    "error: failed to collect document members for '%s'\n",
                    opts.doc_id);
            triplet_store_destroy(store);
            entity_list_free(&doc_entities);
            entity_list_free(&elist);
            return 1;
        }

        FILE *out = stdout;
        if (opts.report_output) {
            out = fopen(opts.report_output, "w");
            if (!out) {
                fprintf(stderr, "error: cannot open output file '%s'\n",
                        opts.report_output);
                triplet_store_destroy(store);
                entity_list_free(&doc_entities);
                entity_list_free(&elist);
                return 1;
            }
        }

        report_write(out, &doc_entities, store, opts.report_format);

        if (opts.report_output)
            fclose(out);

        triplet_store_destroy(store);
        entity_list_free(&doc_entities);
        entity_list_free(&elist);
        return 0;
    }

    /* ------------------------------------------------------------------ */
    /* 'trace', 'coverage', 'orphan' subcommands                          */
    /* ------------------------------------------------------------------ */
    if (opts.trace_id || opts.show_coverage || opts.show_orphan) {
        TripletStore *store = build_entity_relation_store(&elist);
        if (!store) {
            fprintf(stderr, "error: failed to create relation store\n");
            entity_list_free(&elist);
            return 1;
        }

        if (opts.trace_id)
            cmd_trace_entity(&elist, store, opts.trace_id);
        if (opts.show_coverage)
            cmd_coverage(&elist, store);
        if (opts.show_orphan)
            cmd_orphan(&elist, store);

        triplet_store_destroy(store);
        entity_list_free(&elist);
        return 0;
    }

    /* ------------------------------------------------------------------ */
    /* Default / 'links' / --strict-links path                             */
    /* ------------------------------------------------------------------ */
    int exit_code = 0;

    if (opts.show_links || opts.strict_links) {
        TripletStore *store = build_entity_relation_store(&elist);
        if (!store) {
            fprintf(stderr, "error: failed to create relation store\n");
            entity_list_free(&elist);
            return 1;
        }
        if (opts.show_links)
            list_relations(store);
        if (opts.strict_links) {
            int warnings = check_strict_links(store);
            if (warnings > 0) {
                fprintf(stderr, "%d strict-links warning(s) found.\n", warnings);
                exit_code = 1;
            }
        }
        triplet_store_destroy(store);
    } else {
        list_entities(&elist);
    }

    entity_list_free(&elist);
    return exit_code;
}
