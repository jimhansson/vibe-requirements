#include "yaml_simple.h"
#include <yaml.h>
#include <stdio.h>
#include <string.h>

/*
 * Copy at most (dst_size - 1) bytes from src into dst and NUL-terminate.
 * Truncates silently when src is longer than the buffer.
 */
static void copy_field(char *dst, size_t dst_size, const char *src)
{
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

/*
 * Walk the top-level mapping of a libyaml document node and copy
 * recognised scalar values into *out.
 *
 * libyaml document model:
 *   yaml_document_t holds a list of yaml_node_t.
 *   A mapping node contains pairs of (key_index, value_index).
 *   Indexes are 1-based; pass them to yaml_document_get_node().
 */
static void extract_fields(yaml_document_t *doc, yaml_node_t *map, Requirement *out)
{
    if (!map || map->type != YAML_MAPPING_NODE)
        return;

    yaml_node_pair_t *pair = map->data.mapping.pairs.start;
    yaml_node_pair_t *end  = map->data.mapping.pairs.top;

    for (; pair < end; pair++) {
        yaml_node_t *key_node = yaml_document_get_node(doc, pair->key);
        yaml_node_t *val_node = yaml_document_get_node(doc, pair->value);

        if (!key_node || key_node->type != YAML_SCALAR_NODE)
            continue;
        if (!val_node || val_node->type != YAML_SCALAR_NODE)
            continue;

        const char *key = (const char *)key_node->data.scalar.value;
        const char *val = (const char *)val_node->data.scalar.value;

#define MATCH(field, buf, bufsz)              \
        if (strcmp(key, field) == 0) {        \
            copy_field(out->buf, bufsz, val); \
            continue;                         \
        }

        MATCH("id",          id,          REQ_ID_LEN)
        MATCH("title",       title,       REQ_TITLE_LEN)
        MATCH("type",        type,        REQ_TYPE_LEN)
        MATCH("status",      status,      REQ_STATUS_LEN)
        MATCH("priority",    priority,    REQ_PRIORITY_LEN)
        MATCH("description", description, REQ_DESC_LEN)

#undef MATCH
    }
}

int yaml_parse_requirement(const char *path, Requirement *out)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    memset(out, 0, sizeof(*out));
    copy_field(out->file_path, REQ_PATH_LEN, path);

    yaml_parser_t   parser;
    yaml_document_t doc;
    int             rc = -1;

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);

    if (yaml_parser_load(&parser, &doc)) {
        yaml_node_t *root = yaml_document_get_root_node(&doc);
        extract_fields(&doc, root, out);
        yaml_document_delete(&doc);
        rc = (out->id[0] != '\0') ? 0 : -1;
    }

    yaml_parser_delete(&parser);
    fclose(f);
    return rc;
}
