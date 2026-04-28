/*
 * @file yaml_link_parser.cpp
 * @brief YAML link / traceability parsing implementation (C++ edition).
 *
 * Implements yaml_parse_links() declared in yaml_simple.h.
 */

#include "../yaml_simple.h"
#include <yaml.h>
#include <cstdio>
#include <cstring>
#include <string>

/*
 * Walk a relation-keyed traceability mapping node and add
 * (subject, relation, target) triples to store.
 *
 * The mapping format is:
 *   <relation>: <target>          # scalar
 *   <relation>:                   # sequence
 *     - <target1>
 *     - <target2>
 *
 * Returns the number of triples added.
 */
static int add_traceability_triples(yaml_document_t *doc,
                                     yaml_node_t *map,
                                     const char *subject_id,
                                     TripletStore *store)
{
    if (!map || map->type != YAML_MAPPING_NODE)
        return 0;

    int added = 0;

    yaml_node_pair_t *pair = map->data.mapping.pairs.start;
    yaml_node_pair_t *end  = map->data.mapping.pairs.top;

    for (; pair < end; pair++) {
        yaml_node_t *rk = yaml_document_get_node(doc, pair->key);
        yaml_node_t *rv = yaml_document_get_node(doc, pair->value);

        if (!rk || rk->type != YAML_SCALAR_NODE) continue;
        if (!rv) continue;

        const char *relation = reinterpret_cast<const char *>(
            rk->data.scalar.value);
        if (!relation || relation[0] == '\0') continue;

        if (rv->type == YAML_SCALAR_NODE) {
            const char *target = reinterpret_cast<const char *>(
                rv->data.scalar.value);
            if (target && target[0] != '\0') {
                size_t id = triplet_store_add(store, subject_id, relation,
                                              target);
                if (id != TRIPLE_ID_INVALID)
                    added++;
            }
        } else if (rv->type == YAML_SEQUENCE_NODE) {
            yaml_node_item_t *it  = rv->data.sequence.items.start;
            yaml_node_item_t *top = rv->data.sequence.items.top;
            for (; it < top; it++) {
                yaml_node_t *item = yaml_document_get_node(doc, *it);
                if (!item || item->type != YAML_SCALAR_NODE) continue;
                const char *target = reinterpret_cast<const char *>(
                    item->data.scalar.value);
                if (target && target[0] != '\0') {
                    size_t id = triplet_store_add(store, subject_id, relation,
                                                  target);
                    if (id != TRIPLE_ID_INVALID)
                        added++;
                }
            }
        }
    }

    return added;
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

    while (true) {
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
             * different requirement.  Only process the "traceability" or
             * "links" mapping of the document whose top-level "id" matches
             * subject_id.
             */
            const char *doc_id = nullptr;
            yaml_node_pair_t *pair = root->data.mapping.pairs.start;
            yaml_node_pair_t *end  = root->data.mapping.pairs.top;

            for (; pair < end; pair++) {
                yaml_node_t *key_node = yaml_document_get_node(&doc, pair->key);
                if (!key_node || key_node->type != YAML_SCALAR_NODE)
                    continue;
                if (strcmp(reinterpret_cast<const char *>(
                        key_node->data.scalar.value), "id") == 0) {
                    yaml_node_t *val_node = yaml_document_get_node(&doc,
                                                                   pair->value);
                    if (val_node && val_node->type == YAML_SCALAR_NODE)
                        doc_id = reinterpret_cast<const char *>(
                            val_node->data.scalar.value);
                    break;
                }
            }

            /* Skip documents that don't belong to subject_id. */
            if (!doc_id || strcmp(doc_id, subject_id) != 0) {
                yaml_document_delete(&doc);
                continue;
            }

            /* Found the right document — extract its traceability links. */
            pair = root->data.mapping.pairs.start;
            end  = root->data.mapping.pairs.top;

            for (; pair < end; pair++) {
                yaml_node_t *key_node = yaml_document_get_node(&doc, pair->key);
                if (!key_node || key_node->type != YAML_SCALAR_NODE)
                    continue;

                const char *key = reinterpret_cast<const char *>(
                    key_node->data.scalar.value);
                if (strcmp(key, "traceability") != 0 &&
                    strcmp(key, "links") != 0)
                    continue;

                yaml_node_t *links_node = yaml_document_get_node(&doc,
                                                                  pair->value);
                added += add_traceability_triples(&doc, links_node,
                                                  subject_id, store);
                break;
            }
        }

        yaml_document_delete(&doc);
    }

    yaml_parser_delete(&parser);
    fclose(f);
    return added;
}
