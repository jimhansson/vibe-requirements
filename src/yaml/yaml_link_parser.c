/*
 * @file yaml_link_parser.c
 * @brief YAML link / traceability parsing implementation.
 *
 * Implements yaml_parse_links() declared in yaml_simple.h.
 * The static helper add_link_triple() is internal to this translation unit.
 */

#include "../yaml_simple.h"
#include "yaml_helpers.h"
#include <yaml.h>
#include <stdio.h>
#include <string.h>

/*
 * Walk one link mapping node and add a (subject, relation, target) triple
 * to store.
 *
 * Each link entry is expected to have either an "id" or "artefact" key
 * (the object) plus a "relation" key (the predicate).
 *
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

    yaml_parser_t parser;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);

    int added = 0;

    while (1) {
        yaml_document_t doc;
        if (!yaml_parser_load(&parser, &doc))
            break; /* parse error — stop, keep any links already added */

        yaml_node_t *root = yaml_document_get_root_node(&doc);
        if (!root) {
            yaml_document_delete(&doc);
            break; /* end of stream */
        }

        if (root->type == YAML_MAPPING_NODE) {
            /*
             * In a multi-document file each document may belong to a
             * different requirement.  Only process the "links" sequence
             * of the document whose top-level "id" matches subject_id.
             */
            const char *doc_id = NULL;
            yaml_node_pair_t *pair = root->data.mapping.pairs.start;
            yaml_node_pair_t *end  = root->data.mapping.pairs.top;

            for (; pair < end; pair++) {
                yaml_node_t *key_node = yaml_document_get_node(&doc, pair->key);
                if (!key_node || key_node->type != YAML_SCALAR_NODE)
                    continue;
                if (strcmp((const char *)key_node->data.scalar.value, "id") == 0) {
                    yaml_node_t *val_node = yaml_document_get_node(&doc, pair->value);
                    if (val_node && val_node->type == YAML_SCALAR_NODE)
                        doc_id = (const char *)val_node->data.scalar.value;
                    break;
                }
            }

            /* Skip documents that don't belong to subject_id. */
            if (!doc_id || strcmp(doc_id, subject_id) != 0) {
                yaml_document_delete(&doc);
                continue;
            }

            /* Found the right document — extract its links. */
            pair = root->data.mapping.pairs.start;
            end  = root->data.mapping.pairs.top;

            for (; pair < end; pair++) {
                yaml_node_t *key_node = yaml_document_get_node(&doc, pair->key);
                if (!key_node || key_node->type != YAML_SCALAR_NODE)
                    continue;

                const char *key = (const char *)key_node->data.scalar.value;
                if (strcmp(key, "links") != 0)
                    continue;

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
    }

    yaml_parser_delete(&parser);
    fclose(f);
    return added;
}
