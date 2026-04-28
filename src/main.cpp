#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#include "entity.h"
#include "discovery.h"
#include "config.h"
#include "yaml_simple.h"

#include "report.h"
#include "new_cmd.h"
#include "coverage.h"
#include "cli_args.h"
#include "list_cmd.h"
#include "validate.h"

int main(int argc, char *argv[])
{
    CliOptions opts;
    cli_parse_args(argc, argv, &opts);

    if (opts.show_help) {
        cli_print_help(argv[0]);
        return 0;
    }

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

    EntityList elist;

    VibeConfig cfg;
    config_load(opts.root, &cfg);

    int found = discover_entities(opts.root, &elist, &cfg);
    if (found < 0) {
        fprintf(stderr, "error: cannot open directory '%s'\n", opts.root);
        return 1;
    }

    if (elist.size() > 1) {
        std::sort(elist.begin(), elist.end(),
                  [](const Entity &a, const Entity &b) {
                      return a.identity.id < b.identity.id;
                  });
    }

    if (opts.show_entities) {
        if (opts.filter_kind || opts.filter_comp ||
            opts.filter_status || opts.filter_priority) {
            EntityList filtered;
            entity_apply_filter(&elist, &filtered,
                                opts.filter_kind, opts.filter_comp,
                                opts.filter_status, opts.filter_priority);
            list_entities(&filtered);
        } else {
            list_entities(&elist);
        }
        return 0;
    }

    if (opts.show_report) {
        vibe::TripletStore *store = build_entity_relation_store(&elist);
        if (!store) {
            fprintf(stderr, "error: failed to create relation store\n");
            return 1;
        }

        EntityList *src = &elist;
        EntityList  filtered;
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
                delete store;
                return 1;
            }
        }

        report_write(out, src, store, opts.report_format);

        if (opts.report_output)
            fclose(out);

        delete store;
        return 0;
    }

    if (opts.is_doc_cmd) {
        vibe::TripletStore *store = build_entity_relation_store(&elist);
        if (!store) {
            fprintf(stderr, "error: failed to create relation store\n");
            return 1;
        }

        EntityList doc_entities;

        int rc = collect_document_entities(&elist, store, opts.doc_id,
                                           &doc_entities);
        if (rc == -1) {
            fprintf(stderr, "error: document '%s' not found\n", opts.doc_id);
            delete store;
            return 1;
        }
        if (rc == -2) {
            fprintf(stderr, "error: entity '%s' is not a document\n",
                    opts.doc_id);
            delete store;
            return 1;
        }
        if (rc != 0) {
            fprintf(stderr,
                    "error: failed to collect document members for '%s'\n",
                    opts.doc_id);
            delete store;
            return 1;
        }

        FILE *out = stdout;
        if (opts.report_output) {
            out = fopen(opts.report_output, "w");
            if (!out) {
                fprintf(stderr, "error: cannot open output file '%s'\n",
                        opts.report_output);
                delete store;
                return 1;
            }
        }

        report_write(out, &doc_entities, store, opts.report_format);

        if (opts.report_output)
            fclose(out);

        delete store;
        return 0;
    }

    if (opts.is_validate_cmd) {
        vibe::TripletStore *store = build_entity_relation_store(&elist);
        if (!store) {
            fprintf(stderr, "error: failed to create relation store\n");
            return 1;
        }
        int problems = cmd_validate(&elist, store);
        delete store;
        return problems > 0 ? 1 : 0;
    }

    if (opts.trace_id || opts.show_coverage || opts.show_orphan) {
        vibe::TripletStore *store = build_entity_relation_store(&elist);
        if (!store) {
            fprintf(stderr, "error: failed to create relation store\n");
            return 1;
        }

        if (opts.trace_id)
            cmd_trace_entity(&elist, store, opts.trace_id);
        if (opts.show_coverage)
            cmd_coverage(&elist, store);
        if (opts.show_orphan)
            cmd_orphan(&elist, store);

        delete store;
        return 0;
    }

    int exit_code = 0;

    if (opts.show_links || opts.strict_links) {
        vibe::TripletStore *store = build_entity_relation_store(&elist);
        if (!store) {
            fprintf(stderr, "error: failed to create relation store\n");
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
        delete store;
    } else {
        list_entities(&elist);
    }

    return exit_code;
}
