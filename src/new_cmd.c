#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "new_cmd.h"
#include "entity.h"

/* -----------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------- */

/** Return 1 if the file at `path` already exists, 0 otherwise. */
static int file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

/* -----------------------------------------------------------------------
 * Per-kind template writers
 * Each function writes a minimal YAML template to `out`.
 * --------------------------------------------------------------------- */

static void write_requirement(FILE *out, const char *id)
{
    fprintf(out,
            "id: %s\n"
            "title: \"\"\n"
            "type: requirement\n"
            "status: draft\n"
            "priority: must\n"
            "owner: \"\"\n"
            "description: |\n"
            "  TODO: Add description.\n",
            id);
}

static void write_group(FILE *out, const char *id)
{
    fprintf(out,
            "id: %s\n"
            "title: \"\"\n"
            "type: group\n"
            "status: draft\n"
            "owner: \"\"\n"
            "description: |\n"
            "  TODO: Add description.\n",
            id);
}

static void write_story(FILE *out, const char *id)
{
    fprintf(out,
            "id: %s\n"
            "title: \"\"\n"
            "type: story\n"
            "status: draft\n"
            "priority: must\n"
            "owner: \"\"\n"
            "role: \"\"\n"
            "goal: \"\"\n"
            "reason: \"\"\n"
            "description: |\n"
            "  TODO: Add description.\n"
            "acceptance_criteria:\n"
            "  - TODO: Add acceptance criterion.\n",
            id);
}

static void write_design_note(FILE *out, const char *id)
{
    fprintf(out,
            "id: %s\n"
            "title: \"\"\n"
            "type: design-note\n"
            "status: draft\n"
            "owner: \"\"\n"
            "body: |\n"
            "  TODO: Add design note body.\n",
            id);
}

static void write_section(FILE *out, const char *id)
{
    fprintf(out,
            "id: %s\n"
            "title: \"\"\n"
            "type: section\n"
            "status: draft\n"
            "owner: \"\"\n"
            "body: |\n"
            "  TODO: Add section body.\n",
            id);
}

static void write_assumption(FILE *out, const char *id)
{
    fprintf(out,
            "id: %s\n"
            "title: \"\"\n"
            "type: assumption\n"
            "status: draft\n"
            "owner: \"\"\n"
            "assumption:\n"
            "  text: |\n"
            "    TODO: Add assumption text.\n"
            "  status: open\n"
            "  source: \"\"\n",
            id);
}

static void write_constraint(FILE *out, const char *id)
{
    fprintf(out,
            "id: %s\n"
            "title: \"\"\n"
            "type: constraint\n"
            "status: draft\n"
            "owner: \"\"\n"
            "constraint:\n"
            "  text: |\n"
            "    TODO: Add constraint text.\n"
            "  kind: technical\n"
            "  source: \"\"\n",
            id);
}

static void write_test_case(FILE *out, const char *id)
{
    fprintf(out,
            "id: %s\n"
            "title: \"\"\n"
            "type: test-case\n"
            "status: draft\n"
            "owner: \"\"\n"
            "description: |\n"
            "  TODO: Add description.\n"
            "preconditions:\n"
            "  - TODO: Add precondition.\n"
            "steps:\n"
            "  - step: 1\n"
            "    action: \"TODO: Add action.\"\n"
            "    expected_output: \"TODO: Add expected output.\"\n"
            "expected_result: \"TODO: Add expected result.\"\n",
            id);
}

static void write_external(FILE *out, const char *id)
{
    fprintf(out,
            "id: %s\n"
            "title: \"\"\n"
            "type: external\n"
            "status: draft\n"
            "owner: \"\"\n"
            "description: |\n"
            "  TODO: Add description.\n"
            "clauses:\n"
            "  - id: \"\"\n"
            "    title: \"TODO: Add clause title.\"\n",
            id);
}

static void write_document(FILE *out, const char *id, const char *type_str)
{
    fprintf(out,
            "id: %s\n"
            "title: \"\"\n"
            "type: %s\n"
            "status: draft\n"
            "owner: \"\"\n"
            "doc_meta:\n"
            "  doc_type: \"\"\n"
            "  version: \"1.0\"\n"
            "  client: \"\"\n"
            "  status: draft\n",
            id, type_str);
}

/* -----------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */

int new_cmd_scaffold(const char *type_str, const char *id, const char *dir)
{
    if (!type_str || type_str[0] == '\0')
        return -3;
    if (!id || id[0] == '\0')
        return -3;

    /* Resolve entity kind from type string. */
    EntityKind kind = entity_kind_from_string(type_str);
    if (kind == ENTITY_KIND_UNKNOWN)
        return -3;

    /* Build output path: <dir>/<id>.yaml */
    const char *effective_dir = (dir && dir[0] != '\0') ? dir : ".";
    char path[1024];
    int path_len = snprintf(path, sizeof(path), "%s/%s.yaml",
                            effective_dir, id);
    if (path_len < 0 || (size_t)path_len >= sizeof(path)) {
        return -2; /* path too long */
    }

    /* Refuse to overwrite an existing file. */
    if (file_exists(path))
        return -1;

    FILE *out = fopen(path, "w");
    if (!out)
        return -2;

    switch (kind) {
        case ENTITY_KIND_REQUIREMENT:
            write_requirement(out, id);
            break;
        case ENTITY_KIND_GROUP:
            write_group(out, id);
            break;
        case ENTITY_KIND_STORY:
            write_story(out, id);
            break;
        case ENTITY_KIND_DESIGN_NOTE:
            write_design_note(out, id);
            break;
        case ENTITY_KIND_SECTION:
            write_section(out, id);
            break;
        case ENTITY_KIND_ASSUMPTION:
            write_assumption(out, id);
            break;
        case ENTITY_KIND_CONSTRAINT:
            write_constraint(out, id);
            break;
        case ENTITY_KIND_TEST_CASE:
            write_test_case(out, id);
            break;
        case ENTITY_KIND_EXTERNAL:
            write_external(out, id);
            break;
        case ENTITY_KIND_DOCUMENT:
            write_document(out, id, type_str);
            break;
        default:
            fclose(out);
            return -3;
    }

    fclose(out);
    return 0;
}
