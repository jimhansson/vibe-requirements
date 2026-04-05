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

/*
 * Maximum buffer sizes used when extracting link fields from a YAML
 * link entry.  Entity IDs and artefact paths are capped at
 * LINK_TARGET_LEN; relation names are capped at LINK_RELATION_LEN.
 */
#define LINK_TARGET_LEN   256
#define LINK_RELATION_LEN 128

/*
 * Walk one link mapping node and add a triple to store.
 * Each link is expected to have either an "id" or "artefact" key (the
 * object) plus a "relation" key (the predicate).
 * Returns 1 if a triple was added, 0 otherwise.
 */
static int add_link_triple(yaml_document_t *doc, yaml_node_t *link_map,
                            const char *subject_id, TripletStore *store)
{
    if (!link_map || link_map->type != YAML_MAPPING_NODE)
        return 0;

    char target[LINK_TARGET_LEN]     = {0};
    char relation[LINK_RELATION_LEN] = {0};

    yaml_node_pair_t *pair = link_map->data.mapping.pairs.start;
    yaml_node_pair_t *end  = link_map->data.mapping.pairs.top;

    for (; pair < end; pair++) {
        yaml_node_t *key_node = yaml_document_get_node(doc, pair->key);
        yaml_node_t *val_node = yaml_document_get_node(doc, pair->value);

        if (!key_node || key_node->type != YAML_SCALAR_NODE) continue;
        if (!val_node || val_node->type != YAML_SCALAR_NODE) continue;

        const char *key = (const char *)key_node->data.scalar.value;
        const char *val = (const char *)val_node->data.scalar.value;

        if (strcmp(key, "id") == 0 || strcmp(key, "artefact") == 0) {
            strncpy(target, val, sizeof(target) - 1);
        } else if (strcmp(key, "relation") == 0) {
            strncpy(relation, val, sizeof(relation) - 1);
        }
    }

    if (target[0] == '\0' || relation[0] == '\0')
        return 0;

    size_t id = triplet_store_add(store, subject_id, relation, target);
    return (id != TRIPLE_ID_INVALID) ? 1 : 0;
}

int yaml_parse_links(const char *path, const char *subject_id,
                     TripletStore *store)
{
    if (!path || !subject_id || !store)
        return -1;

    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    yaml_parser_t   parser;
    yaml_document_t doc;
    int             added = -1;

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);

    if (!yaml_parser_load(&parser, &doc)) {
        yaml_parser_delete(&parser);
        fclose(f);
        return -1;
    }

    added = 0;
    yaml_node_t *root = yaml_document_get_root_node(&doc);

    if (root && root->type == YAML_MAPPING_NODE) {
        yaml_node_pair_t *pair = root->data.mapping.pairs.start;
        yaml_node_pair_t *end  = root->data.mapping.pairs.top;

        for (; pair < end; pair++) {
            yaml_node_t *key_node = yaml_document_get_node(&doc, pair->key);
            if (!key_node || key_node->type != YAML_SCALAR_NODE)
                continue;

            const char *key = (const char *)key_node->data.scalar.value;
            if (strcmp(key, "links") != 0)
                continue;

            /* Found the "links" key — iterate the sequence. */
            yaml_node_t *links_node = yaml_document_get_node(&doc, pair->value);
            if (!links_node || links_node->type != YAML_SEQUENCE_NODE)
                break;

            yaml_node_item_t *item = links_node->data.sequence.items.start;
            yaml_node_item_t *top  = links_node->data.sequence.items.top;

            for (; item < top; item++) {
                yaml_node_t *link_map = yaml_document_get_node(&doc, *item);
                added += add_link_triple(&doc, link_map, subject_id, store);
            }
            break;
        }
    }

    yaml_document_delete(&doc);
    yaml_parser_delete(&parser);
    fclose(f);
    return added;
}
