/**
 * @file entity_parser.cpp
 * @brief YAML entity field extraction and multi-document parsing (C++ edition).
 */

#include "../yaml_simple.h"
#include "yaml_helpers.h"
#include "../entity.h"
#include <yaml.h>
#include <cstdio>
#include <cstring>
#include <string>

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

static std::string scalar_value(yaml_node_t *node)
{
    if (!node || node->type != YAML_SCALAR_NODE)
        return {};
    return reinterpret_cast<const char *>(node->data.scalar.value);
}

static void collect_sequence_to_vector(yaml_document_t *doc,
                                        yaml_node_t *seq,
                                        std::vector<std::string> &out)
{
    if (!seq || seq->type != YAML_SEQUENCE_NODE)
        return;
    for (yaml_node_item_t *it = seq->data.sequence.items.start;
         it < seq->data.sequence.items.top; ++it) {
        yaml_node_t *item = yaml_document_get_node(doc, *it);
        if (item && item->type == YAML_SCALAR_NODE) {
            std::string v = scalar_value(item);
            if (!v.empty())
                out.push_back(std::move(v));
        }
    }
}

static void extract_entity_fields(yaml_document_t *doc, yaml_node_t *map,
                                   Entity *out)
{
    if (!map || map->type != YAML_MAPPING_NODE)
        return;

    for (yaml_node_pair_t *pair = map->data.mapping.pairs.start;
         pair < map->data.mapping.pairs.top; ++pair) {

        yaml_node_t *key_node = yaml_document_get_node(doc, pair->key);
        yaml_node_t *val_node = yaml_document_get_node(doc, pair->value);

        if (!key_node || key_node->type != YAML_SCALAR_NODE)
            continue;

        const char *key = reinterpret_cast<const char *>(
            key_node->data.scalar.value);

        /* --- Scalar fields ------------------------------------------- */
        if (val_node && val_node->type == YAML_SCALAR_NODE) {
            std::string val = scalar_value(val_node);

#define ASSIGN_IF(field_key, dst) \
            if (strcmp(key, field_key) == 0) { dst = val; continue; }

            ASSIGN_IF("id",          out->identity.id)
            ASSIGN_IF("title",       out->identity.title)
            ASSIGN_IF("type",        out->identity.type_raw)
            ASSIGN_IF("status",      out->lifecycle.status)
            ASSIGN_IF("priority",    out->lifecycle.priority)
            ASSIGN_IF("owner",       out->lifecycle.owner)
            ASSIGN_IF("version",     out->lifecycle.version)
            ASSIGN_IF("description", out->text.description)
            ASSIGN_IF("rationale",   out->text.rationale)
            ASSIGN_IF("role",        out->user_story.role)
            ASSIGN_IF("goal",        out->user_story.goal)
            ASSIGN_IF("reason",      out->user_story.reason)
            ASSIGN_IF("as-a",        out->user_story.role)
            ASSIGN_IF("i-want",      out->user_story.goal)
            ASSIGN_IF("so-that",     out->user_story.reason)
            ASSIGN_IF("epic",        out->epic_membership.epic_id)
            ASSIGN_IF("body",        out->doc_body.body)
            ASSIGN_IF("expected-result", out->test_procedure.expected_result)

#undef ASSIGN_IF

            /* applies-to — scalar form */
            if (strcmp(key, "applies-to") == 0) {
                if (!val.empty())
                    out->applies_to.applies_to.push_back(val);
                continue;
            }
        }

        /* --- Sequence fields ----------------------------------------- */
        if (val_node && val_node->type == YAML_SEQUENCE_NODE) {
            if (strcmp(key, "tags") == 0) {
                collect_sequence_to_vector(doc, val_node, out->tags.tags);
                continue;
            }
            if (strcmp(key, "acceptance-criteria") == 0) {
                collect_sequence_to_vector(doc, val_node,
                                           out->acceptance_criteria.criteria);
                continue;
            }
            if (strcmp(key, "documents") == 0) {
                collect_sequence_to_vector(doc, val_node,
                                           out->doc_membership.doc_ids);
                continue;
            }
            if (strcmp(key, "applies-to") == 0) {
                collect_sequence_to_vector(doc, val_node,
                                           out->applies_to.applies_to);
                continue;
            }
            if (strcmp(key, "sources") == 0) {
                for (yaml_node_item_t *it = val_node->data.sequence.items.start;
                     it < val_node->data.sequence.items.top; ++it) {
                    yaml_node_t *src_node = yaml_document_get_node(doc, *it);
                    if (!src_node) continue;
                    if (src_node->type == YAML_SCALAR_NODE) {
                        std::string v = scalar_value(src_node);
                        if (!v.empty())
                            out->sources.sources.push_back(std::move(v));
                    } else if (src_node->type == YAML_MAPPING_NODE) {
                        for (yaml_node_pair_t *sp =
                                 src_node->data.mapping.pairs.start;
                             sp < src_node->data.mapping.pairs.top; ++sp) {
                            yaml_node_t *sv = yaml_document_get_node(doc,
                                                                     sp->value);
                            if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                            std::string v = scalar_value(sv);
                            if (!v.empty()) {
                                out->sources.sources.push_back(std::move(v));
                                break;
                            }
                        }
                    }
                }
                continue;
            }
            if (strcmp(key, "traceability") == 0 ||
                strcmp(key, "links") == 0) {
                for (yaml_node_item_t *it = val_node->data.sequence.items.start;
                     it < val_node->data.sequence.items.top; ++it) {
                    yaml_node_t *link_map = yaml_document_get_node(doc, *it);
                    if (!link_map || link_map->type != YAML_MAPPING_NODE)
                        continue;
                    std::string target, relation;
                    for (yaml_node_pair_t *sp =
                             link_map->data.mapping.pairs.start;
                         sp < link_map->data.mapping.pairs.top; ++sp) {
                        yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                        yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                        if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                        if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                        const char *skey = reinterpret_cast<const char *>(
                            sk->data.scalar.value);
                        if (strcmp(skey, "id") == 0 ||
                            strcmp(skey, "artefact") == 0)
                            target = scalar_value(sv);
                        else if (strcmp(skey, "relation") == 0)
                            relation = scalar_value(sv);
                    }
                    if (!target.empty() && !relation.empty())
                        out->traceability.entries.push_back({target, relation});
                }
                continue;
            }
            if (strcmp(key, "preconditions") == 0) {
                collect_sequence_to_vector(doc, val_node,
                                           out->test_procedure.preconditions);
                continue;
            }
            if (strcmp(key, "steps") == 0) {
                for (yaml_node_item_t *it = val_node->data.sequence.items.start;
                     it < val_node->data.sequence.items.top; ++it) {
                    yaml_node_t *step_map = yaml_document_get_node(doc, *it);
                    if (!step_map || step_map->type != YAML_MAPPING_NODE)
                        continue;
                    std::string action, expected_output;
                    for (yaml_node_pair_t *sp =
                             step_map->data.mapping.pairs.start;
                         sp < step_map->data.mapping.pairs.top; ++sp) {
                        yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                        yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                        if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                        if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                        const char *skey = reinterpret_cast<const char *>(
                            sk->data.scalar.value);
                        if (strcmp(skey, "action") == 0)
                            action = scalar_value(sv);
                        else if (strcmp(skey, "expected-output") == 0)
                            expected_output = scalar_value(sv);
                    }
                    if (!action.empty())
                        out->test_procedure.steps.push_back(
                            {action, expected_output});
                }
                continue;
            }
            if (strcmp(key, "clauses") == 0) {
                for (yaml_node_item_t *it = val_node->data.sequence.items.start;
                     it < val_node->data.sequence.items.top; ++it) {
                    yaml_node_t *clause_map = yaml_document_get_node(doc, *it);
                    if (!clause_map || clause_map->type != YAML_MAPPING_NODE)
                        continue;
                    std::string clause_id, clause_title;
                    for (yaml_node_pair_t *sp =
                             clause_map->data.mapping.pairs.start;
                         sp < clause_map->data.mapping.pairs.top; ++sp) {
                        yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                        yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                        if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                        if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                        const char *skey = reinterpret_cast<const char *>(
                            sk->data.scalar.value);
                        if (strcmp(skey, "id") == 0)
                            clause_id = scalar_value(sv);
                        else if (strcmp(skey, "title") == 0)
                            clause_title = scalar_value(sv);
                    }
                    if (!clause_id.empty())
                        out->clause_collection.clauses.push_back(
                            {clause_id, clause_title});
                }
                continue;
            }
            if (strcmp(key, "attachments") == 0) {
                for (yaml_node_item_t *it = val_node->data.sequence.items.start;
                     it < val_node->data.sequence.items.top; ++it) {
                    yaml_node_t *att_map = yaml_document_get_node(doc, *it);
                    if (!att_map || att_map->type != YAML_MAPPING_NODE)
                        continue;
                    std::string att_path, att_description;
                    for (yaml_node_pair_t *sp =
                             att_map->data.mapping.pairs.start;
                         sp < att_map->data.mapping.pairs.top; ++sp) {
                        yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                        yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                        if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                        if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                        const char *skey = reinterpret_cast<const char *>(
                            sk->data.scalar.value);
                        if (strcmp(skey, "path") == 0)
                            att_path = scalar_value(sv);
                        else if (strcmp(skey, "description") == 0)
                            att_description = scalar_value(sv);
                    }
                    if (!att_path.empty())
                        out->attachment.attachments.push_back(
                            {att_path, att_description});
                }
                continue;
            }
        }

        /* --- Mapping fields ------------------------------------------ */
        if (val_node && val_node->type == YAML_MAPPING_NODE) {
            if (strcmp(key, "assumption") == 0) {
                for (yaml_node_pair_t *sp = val_node->data.mapping.pairs.start;
                     sp < val_node->data.mapping.pairs.top; ++sp) {
                    yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                    yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                    if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                    if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                    const char *skey = reinterpret_cast<const char *>(
                        sk->data.scalar.value);
                    if (strcmp(skey, "text") == 0)
                        out->assumption.text = scalar_value(sv);
                    else if (strcmp(skey, "status") == 0)
                        out->assumption.status = scalar_value(sv);
                    else if (strcmp(skey, "source") == 0)
                        out->assumption.source = scalar_value(sv);
                }
                continue;
            }
            if (strcmp(key, "constraint") == 0) {
                for (yaml_node_pair_t *sp = val_node->data.mapping.pairs.start;
                     sp < val_node->data.mapping.pairs.top; ++sp) {
                    yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                    yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                    if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                    if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                    const char *skey = reinterpret_cast<const char *>(
                        sk->data.scalar.value);
                    if (strcmp(skey, "text") == 0)
                        out->constraint.text = scalar_value(sv);
                    else if (strcmp(skey, "kind") == 0)
                        out->constraint.kind = scalar_value(sv);
                    else if (strcmp(skey, "source") == 0)
                        out->constraint.source = scalar_value(sv);
                }
                continue;
            }
            if (strcmp(key, "doc-meta") == 0) {
                for (yaml_node_pair_t *sp = val_node->data.mapping.pairs.start;
                     sp < val_node->data.mapping.pairs.top; ++sp) {
                    yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                    yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                    if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                    if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                    const char *skey = reinterpret_cast<const char *>(
                        sk->data.scalar.value);
                    if (strcmp(skey, "title") == 0)
                        out->doc_meta.title = scalar_value(sv);
                    else if (strcmp(skey, "doc-type") == 0)
                        out->doc_meta.doc_type = scalar_value(sv);
                    else if (strcmp(skey, "version") == 0)
                        out->doc_meta.version = scalar_value(sv);
                    else if (strcmp(skey, "client") == 0)
                        out->doc_meta.client = scalar_value(sv);
                    else if (strcmp(skey, "status") == 0)
                        out->doc_meta.status = scalar_value(sv);
                }
                continue;
            }
            if (strcmp(key, "variant-profile") == 0) {
                for (yaml_node_pair_t *sp = val_node->data.mapping.pairs.start;
                     sp < val_node->data.mapping.pairs.top; ++sp) {
                    yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                    yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                    if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                    if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                    const char *skey = reinterpret_cast<const char *>(
                        sk->data.scalar.value);
                    if (strcmp(skey, "customer") == 0)
                        out->variant_profile.customer = scalar_value(sv);
                    else if (strcmp(skey, "product") == 0 ||
                             strcmp(skey, "delivery") == 0)
                        out->variant_profile.product = scalar_value(sv);
                }
                continue;
            }
            if (strcmp(key, "composition-profile") == 0) {
                for (yaml_node_pair_t *sp = val_node->data.mapping.pairs.start;
                     sp < val_node->data.mapping.pairs.top; ++sp) {
                    yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                    yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                    if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                    if (!sv) continue;
                    const char *skey = reinterpret_cast<const char *>(
                        sk->data.scalar.value);
                    if (strcmp(skey, "order") == 0 &&
                        sv->type == YAML_SEQUENCE_NODE) {
                        collect_sequence_to_vector(doc, sv,
                                                   out->composition_profile.order);
                    }
                }
                continue;
            }
            if (strcmp(key, "render-profile") == 0) {
                for (yaml_node_pair_t *sp = val_node->data.mapping.pairs.start;
                     sp < val_node->data.mapping.pairs.top; ++sp) {
                    yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                    yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                    if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                    if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                    const char *skey = reinterpret_cast<const char *>(
                        sk->data.scalar.value);
                    if (strcmp(skey, "format") == 0)
                        out->render_profile.format = scalar_value(sv);
                }
                continue;
            }
        }
    }

    out->identity.kind = entity_kind_from_string(
        out->identity.type_raw.c_str());
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

int yaml_parse_entity(const char *path, Entity *out)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    *out = Entity{};
    out->identity.file_path = path;

    yaml_parser_t   parser;
    yaml_document_t doc;
    int             rc = -1;

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);

    if (yaml_parser_load(&parser, &doc)) {
        yaml_node_t *root = yaml_document_get_root_node(&doc);
        extract_entity_fields(&doc, root, out);
        yaml_document_delete(&doc);
        rc = !out->identity.id.empty() ? 0 : -1;
    }

    yaml_parser_delete(&parser);
    fclose(f);
    return rc;
}

int yaml_parse_entities(const char *path, EntityList *list)
{
    if (!path || !list)
        return -1;

    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    yaml_parser_t parser;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);

    int added   = 0;
    int doc_idx = 0;

    while (true) {
        yaml_document_t doc;
        if (!yaml_parser_load(&parser, &doc))
            break;

        yaml_node_t *root = yaml_document_get_root_node(&doc);
        if (!root) {
            yaml_document_delete(&doc);
            break;
        }

        Entity entity{};
        entity.identity.file_path = path;
        entity.identity.doc_index = doc_idx++;

        extract_entity_fields(&doc, root, &entity);
        yaml_document_delete(&doc);

        if (!entity.identity.id.empty()) {
            list->push_back(std::move(entity));
            added++;
        }
    }

    yaml_parser_delete(&parser);
    fclose(f);
    return added;
}

int entity_doc_membership_to_triplets(const Entity *entity, TripletStore *store)
{
    if (!entity || !store)
        return -1;

    if (entity->identity.id.empty())
        return 0;

    const char *subject = entity->identity.id.c_str();
    int added = 0;

    for (const auto &doc_id : entity->doc_membership.doc_ids) {
        if (doc_id.empty()) continue;
        size_t id = triplet_store_add(store, subject, "part-of",
                                      doc_id.c_str());
        if (id != TRIPLE_ID_INVALID)
            added++;
    }

    return added;
}

int entity_traceability_to_triplets(const Entity *entity, TripletStore *store)
{
    if (!entity || !store)
        return -1;

    if (entity->identity.id.empty())
        return 0;

    const char *subject = entity->identity.id.c_str();
    int added = 0;

    for (const auto &entry : entity->traceability.entries) {
        const std::string &target   = entry.first;
        const std::string &relation = entry.second;
        if (target.empty() || relation.empty()) continue;
        size_t id = triplet_store_add(store, subject, relation.c_str(),
                                      target.c_str());
        if (id != TRIPLE_ID_INVALID)
            added++;
    }

    return added;
}
