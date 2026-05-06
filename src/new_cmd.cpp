#include <cstdio>
#include <cstring>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <sys/stat.h>

#include "new_cmd.h"
#include "config.h"
#include "discovery.h"
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

static const char *default_prefix_for_type(const char *type_str)
{
    if (!type_str || type_str[0] == '\0')
        return nullptr;

    if (strcmp(type_str, "requirement") == 0 ||
        strcmp(type_str, "functional") == 0 ||
        strcmp(type_str, "non-functional") == 0)
        return "REQ-";
    if (strcmp(type_str, "group") == 0)
        return "GRP-";
    if (strcmp(type_str, "story") == 0 ||
        strcmp(type_str, "user-story") == 0)
        return "STORY-";
    if (strcmp(type_str, "design-note") == 0 ||
        strcmp(type_str, "design") == 0)
        return "DESIGN-";
    if (strcmp(type_str, "section") == 0)
        return "SEC-";
    if (strcmp(type_str, "assumption") == 0)
        return "ASSUM-";
    if (strcmp(type_str, "constraint") == 0)
        return "CONSTR-";
    if (strcmp(type_str, "test-case") == 0 ||
        strcmp(type_str, "test") == 0)
        return "TC-";
    if (strcmp(type_str, "external") == 0 ||
        strcmp(type_str, "directive") == 0 ||
        strcmp(type_str, "standard") == 0 ||
        strcmp(type_str, "regulation") == 0)
        return "EXT-";
    if (strcmp(type_str, "document") == 0)
        return "DOC-";
    if (strcmp(type_str, "srs") == 0)
        return "SRS-";
    if (strcmp(type_str, "sdd") == 0)
        return "SDD-";
    if (strcmp(type_str, "document-schema") == 0)
        return "DOC-SCHEMA-";

    return nullptr;
}

static bool is_numeric_suffix(const char *text)
{
    if (!text || text[0] == '\0')
        return false;
    for (const char *p = text; *p; ++p) {
        if (!std::isdigit(static_cast<unsigned char>(*p)))
            return false;
    }
    return true;
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
            "acceptance-criteria:\n"
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
            "    expected-output: \"TODO: Add expected output.\"\n"
            "expected-result: \"TODO: Add expected result.\"\n",
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
            "doc-meta:\n"
            "  doc-type: \"\"\n"
            "  version: \"1.0\"\n"
            "  client: \"\"\n"
            "  status: draft\n",
            id, type_str);
}

static void write_document_schema(FILE *out, const char *id)
{
    fprintf(out,
            "id: %s\n"
            "title: \"\"\n"
            "type: document-schema\n"
            "status: draft\n"
            "owner: \"\"\n"
            "description: |\n"
            "  TODO: Add description.\n"
            "variant-profile:\n"
            "  customer: \"\"\n"
            "  product: \"\"\n"
            "composition-profile:\n"
            "  order:\n"
            "    - SEC-INTRO\n"
            "render-profile:\n"
            "  format: markdown\n",
            id);
}

/* -----------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */

int new_cmd_next_id(const char *type_str, const char *prefix, const char *dir,
                    char *out, size_t out_len)
{
    if (!out || out_len == 0)
        return -2;
    if (!type_str || type_str[0] == '\0')
        return -3;

    EntityKind kind = entity_kind_from_string(type_str);
    if (kind == ENTITY_KIND_UNKNOWN)
        return -3;

    const char *effective_prefix =
        (prefix && prefix[0] != '\0') ? prefix : default_prefix_for_type(type_str);
    if (!effective_prefix || effective_prefix[0] == '\0')
        return -4;

    const char *effective_dir = (dir && dir[0] != '\0') ? dir : ".";

    VibeConfig cfg;
    config_load(effective_dir, &cfg);

    EntityList list;
    int found = discover_entities_with_options(effective_dir, &list, &cfg, 0);
    if (found < 0)
        return -1;

    long long max_value = 0;
    size_t max_width = 0;
    size_t prefix_len = strlen(effective_prefix);

    for (const auto &entity : list) {
        if (entity.identity.kind != kind)
            continue;
        const std::string &id = entity.identity.id;
        if (id.size() <= prefix_len)
            continue;
        if (id.compare(0, prefix_len, effective_prefix) != 0)
            continue;

        const char *suffix = id.c_str() + prefix_len;
        if (!is_numeric_suffix(suffix))
            continue;

        errno = 0;
        char *end = nullptr;
        long long value = std::strtoll(suffix, &end, 10);
        if (errno == ERANGE || !end || *end != '\0' || value < 0)
            continue;
        size_t width = strlen(suffix);

        /* Preserve zero-padding width when duplicate values differ in digits. */
        if (value > max_value || (value == max_value && width > max_width)) {
            max_value = value;
            max_width = width;
        }
    }

    if (max_width == 0)
        max_width = 3;

    long long next_value = max_value + 1;
    int needed = snprintf(out, out_len, "%s%0*lld", effective_prefix,
                          static_cast<int>(max_width), next_value);
    if (needed < 0 || static_cast<size_t>(needed) >= out_len)
        return -2;

    return 0;
}

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
        case ENTITY_KIND_DOCUMENT_SCHEMA:
            write_document_schema(out, id);
            break;
        default:
            fclose(out);
            return -3;
    }

    fclose(out);
    return 0;
}
