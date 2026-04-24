#define _POSIX_C_SOURCE 200809L  /* for strdup */

/*
 * @file entity_parser.c
 * @brief YAML entity field extraction and multi-document parsing.
 *
 * Implements yaml_parse_entity(), yaml_parse_entities(), and
 * entity_traceability_to_triplets() declared in yaml_simple.h.
 *
 * The static helper extract_entity_fields() performs a single-pass walk
 * over a YAML mapping node and populates every recognised ECS component.
 */

#include "../yaml_simple.h"
#include "yaml_helpers.h"
#include "../entity.h"
#include <yaml.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Internal field-extraction helpers
 * ---------------------------------------------------------------------- */

/*
 * Walk the top-level mapping of a YAML document and populate an Entity.
 *
 * All scalar, sequence, and mapping YAML keys recognised by the schema
 * are handled here.  Unknown keys are silently ignored.
 */
static void extract_entity_fields(yaml_document_t *doc, yaml_node_t *map,
                                   Entity *out)
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

        const char *key = (const char *)key_node->data.scalar.value;

        /* --- Scalar fields ------------------------------------------- */
        if (val_node && val_node->type == YAML_SCALAR_NODE) {
            const char *val = (const char *)val_node->data.scalar.value;

#define COPY_IF(field_key, dst, dst_size)                          \
            if (strcmp(key, field_key) == 0) {                     \
                yaml_copy_field(dst, dst_size, val);               \
                continue;                                          \
            }

            COPY_IF("id",       out->identity.id,             sizeof(out->identity.id))
            COPY_IF("title",    out->identity.title,          sizeof(out->identity.title))
            COPY_IF("type",     out->identity.type_raw,       sizeof(out->identity.type_raw))
            COPY_IF("status",   out->lifecycle.status,        sizeof(out->lifecycle.status))
            COPY_IF("priority", out->lifecycle.priority,      sizeof(out->lifecycle.priority))
            COPY_IF("owner",    out->lifecycle.owner,         sizeof(out->lifecycle.owner))
            COPY_IF("version",  out->lifecycle.version,       sizeof(out->lifecycle.version))
            COPY_IF("description", out->text.description,     sizeof(out->text.description))
            COPY_IF("rationale",   out->text.rationale,       sizeof(out->text.rationale))
            /* user-story component — canonical keys */
            COPY_IF("role",   out->user_story.role,   sizeof(out->user_story.role))
            COPY_IF("goal",   out->user_story.goal,   sizeof(out->user_story.goal))
            COPY_IF("reason", out->user_story.reason, sizeof(out->user_story.reason))
            /* user-story component — legacy aliases */
            COPY_IF("as_a",    out->user_story.role,   sizeof(out->user_story.role))
            COPY_IF("i_want",  out->user_story.goal,   sizeof(out->user_story.goal))
            COPY_IF("so_that", out->user_story.reason, sizeof(out->user_story.reason))
            /* epic-membership component */
            COPY_IF("epic", out->epic_membership.epic_id, sizeof(out->epic_membership.epic_id))

#undef COPY_IF

            /* doc_body — heap-allocated, use strdup */
            if (strcmp(key, "body") == 0) {
                free(out->doc_body.body);
                out->doc_body.body = strdup(val);
                if (!out->doc_body.body && val[0] != '\0')
                    fprintf(stderr, "warning: failed to allocate doc_body\n");
                continue;
            }

            /* test_procedure expected_result — heap-allocated */
            if (strcmp(key, "expected_result") == 0) {
                free(out->test_procedure.expected_result);
                out->test_procedure.expected_result = strdup(val);
                if (!out->test_procedure.expected_result && val[0] != '\0')
                    fprintf(stderr, "warning: failed to allocate expected_result\n");
                continue;
            }
        }

        /* --- Sequence fields ----------------------------------------- */
        if (val_node && val_node->type == YAML_SEQUENCE_NODE) {
            if (strcmp(key, "tags") == 0) {
                yaml_collect_sequence(doc, val_node,
                                      out->tags.tags, sizeof(out->tags.tags),
                                      &out->tags.count);
                continue;
            }
            if (strcmp(key, "acceptance_criteria") == 0) {
                yaml_collect_sequence(doc, val_node,
                                      out->acceptance_criteria.criteria,
                                      sizeof(out->acceptance_criteria.criteria),
                                      &out->acceptance_criteria.count);
                continue;
            }
            if (strcmp(key, "documents") == 0) {
                yaml_collect_sequence(doc, val_node,
                                      out->doc_membership.doc_ids,
                                      sizeof(out->doc_membership.doc_ids),
                                      &out->doc_membership.count);
                continue;
            }
            if (strcmp(key, "sources") == 0) {
                yaml_node_item_t *item = val_node->data.sequence.items.start;
                yaml_node_item_t *top  = val_node->data.sequence.items.top;
                for (; item < top; item++) {
                    yaml_node_t *src_node = yaml_document_get_node(doc, *item);
                    if (!src_node)
                        continue;
                    if (src_node->type == YAML_SCALAR_NODE) {
                        /* Plain scalar — store the value directly. */
                        const char *val = (const char *)src_node->data.scalar.value;
                        if (val[0] != '\0')
                            yaml_append_to_flat(out->sources.sources,
                                                sizeof(out->sources.sources),
                                                &out->sources.count, val);
                    } else if (src_node->type == YAML_MAPPING_NODE) {
                        /* Mapping — extract the value of the first key. */
                        yaml_node_pair_t *sp = src_node->data.mapping.pairs.start;
                        yaml_node_pair_t *se = src_node->data.mapping.pairs.top;
                        for (; sp < se; sp++) {
                            yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                            yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                            if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                            if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                            const char *sval = (const char *)sv->data.scalar.value;
                            if (sval[0] != '\0') {
                                yaml_append_to_flat(out->sources.sources,
                                                    sizeof(out->sources.sources),
                                                    &out->sources.count, sval);
                                break; /* only the first key-value per item */
                            }
                        }
                    }
                }
                continue;
            }
            if (strcmp(key, "traceability") == 0 || strcmp(key, "links") == 0) {
                yaml_node_item_t *item = val_node->data.sequence.items.start;
                yaml_node_item_t *top  = val_node->data.sequence.items.top;
                for (; item < top; item++) {
                    yaml_node_t *link_map = yaml_document_get_node(doc, *item);
                    if (!link_map || link_map->type != YAML_MAPPING_NODE)
                        continue;

                    char target[LINK_TARGET_LEN]     = {0};
                    char relation[LINK_RELATION_LEN] = {0};

                    yaml_node_pair_t *sp = link_map->data.mapping.pairs.start;
                    yaml_node_pair_t *se = link_map->data.mapping.pairs.top;
                    for (; sp < se; sp++) {
                        yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                        yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                        if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                        if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                        const char *skey = (const char *)sk->data.scalar.value;
                        const char *sval = (const char *)sv->data.scalar.value;
                        if (strcmp(skey, "id") == 0 || strcmp(skey, "artefact") == 0)
                            strncpy(target, sval, sizeof(target) - 1);
                        else if (strcmp(skey, "relation") == 0)
                            strncpy(relation, sval, sizeof(relation) - 1);
                    }
                    if (target[0] != '\0' && relation[0] != '\0') {
                        yaml_append_pair_entry(out->traceability.entries,
                                               sizeof(out->traceability.entries),
                                               &out->traceability.count,
                                               target, relation);
                    }
                }
                continue;
            }
            if (strcmp(key, "preconditions") == 0) {
                if (!out->test_procedure.preconditions) {
                    out->test_procedure.preconditions =
                        (char *)calloc(1, TEST_PROC_PRECOND_LEN);
                }
                if (out->test_procedure.preconditions) {
                    yaml_collect_sequence(doc, val_node,
                                          out->test_procedure.preconditions,
                                          TEST_PROC_PRECOND_LEN,
                                          &out->test_procedure.precondition_count);
                }
                continue;
            }
            if (strcmp(key, "steps") == 0) {
                if (!out->test_procedure.steps) {
                    out->test_procedure.steps =
                        (char *)calloc(1, TEST_PROC_STEPS_LEN);
                }
                yaml_node_item_t *item = val_node->data.sequence.items.start;
                yaml_node_item_t *top  = val_node->data.sequence.items.top;
                for (; item < top; item++) {
                    yaml_node_t *step_map = yaml_document_get_node(doc, *item);
                    if (!step_map || step_map->type != YAML_MAPPING_NODE)
                        continue;

                    char action[STEP_ACTION_LEN]          = {0};
                    char expected_output[STEP_OUTPUT_LEN] = {0};

                    yaml_node_pair_t *sp = step_map->data.mapping.pairs.start;
                    yaml_node_pair_t *se = step_map->data.mapping.pairs.top;
                    for (; sp < se; sp++) {
                        yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                        yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                        if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                        if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                        const char *skey = (const char *)sk->data.scalar.value;
                        const char *sval = (const char *)sv->data.scalar.value;
                        if (strcmp(skey, "action") == 0)
                            strncpy(action, sval, sizeof(action) - 1);
                        else if (strcmp(skey, "expected_output") == 0)
                            strncpy(expected_output, sval, sizeof(expected_output) - 1);
                    }
                    if (action[0] != '\0' && out->test_procedure.steps) {
                        yaml_append_pair_entry(out->test_procedure.steps,
                                               TEST_PROC_STEPS_LEN,
                                               &out->test_procedure.step_count,
                                               action, expected_output);
                    }
                }
                continue;
            }
            if (strcmp(key, "clauses") == 0) {
                if (!out->clause_collection.clauses) {
                    out->clause_collection.clauses =
                        (char *)calloc(1, CLAUSE_STORE_LEN);
                }
                yaml_node_item_t *item = val_node->data.sequence.items.start;
                yaml_node_item_t *top  = val_node->data.sequence.items.top;
                for (; item < top; item++) {
                    yaml_node_t *clause_map = yaml_document_get_node(doc, *item);
                    if (!clause_map || clause_map->type != YAML_MAPPING_NODE)
                        continue;

                    char clause_id[CLAUSE_ID_LEN]       = {0};
                    char clause_title[CLAUSE_TITLE_LEN] = {0};

                    yaml_node_pair_t *sp = clause_map->data.mapping.pairs.start;
                    yaml_node_pair_t *se = clause_map->data.mapping.pairs.top;
                    for (; sp < se; sp++) {
                        yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                        yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                        if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                        if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                        const char *skey = (const char *)sk->data.scalar.value;
                        const char *sval = (const char *)sv->data.scalar.value;
                        if (strcmp(skey, "id") == 0)
                            strncpy(clause_id, sval, sizeof(clause_id) - 1);
                        else if (strcmp(skey, "title") == 0)
                            strncpy(clause_title, sval, sizeof(clause_title) - 1);
                    }
                    if (clause_id[0] != '\0' && out->clause_collection.clauses) {
                        yaml_append_pair_entry(out->clause_collection.clauses,
                                               CLAUSE_STORE_LEN,
                                               &out->clause_collection.count,
                                               clause_id, clause_title);
                    }
                }
                continue;
            }
            if (strcmp(key, "attachments") == 0) {
                if (!out->attachment.attachments) {
                    out->attachment.attachments =
                        (char *)calloc(1, ATTACH_STORE_LEN);
                }
                yaml_node_item_t *item = val_node->data.sequence.items.start;
                yaml_node_item_t *top  = val_node->data.sequence.items.top;
                for (; item < top; item++) {
                    yaml_node_t *att_map = yaml_document_get_node(doc, *item);
                    if (!att_map || att_map->type != YAML_MAPPING_NODE)
                        continue;

                    char att_path[ATTACH_PATH_LEN]        = {0};
                    char att_description[ATTACH_DESC_LEN] = {0};

                    yaml_node_pair_t *sp = att_map->data.mapping.pairs.start;
                    yaml_node_pair_t *se = att_map->data.mapping.pairs.top;
                    for (; sp < se; sp++) {
                        yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                        yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                        if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                        if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                        const char *skey = (const char *)sk->data.scalar.value;
                        const char *sval = (const char *)sv->data.scalar.value;
                        if (strcmp(skey, "path") == 0)
                            strncpy(att_path, sval, sizeof(att_path) - 1);
                        else if (strcmp(skey, "description") == 0)
                            strncpy(att_description, sval, sizeof(att_description) - 1);
                    }
                    if (att_path[0] != '\0' && out->attachment.attachments) {
                        yaml_append_pair_entry(out->attachment.attachments,
                                               ATTACH_STORE_LEN,
                                               &out->attachment.count,
                                               att_path, att_description);
                    }
                }
                continue;
            }
        }

        /* --- Mapping fields ------------------------------------------ */
        if (val_node && val_node->type == YAML_MAPPING_NODE) {
            if (strcmp(key, "assumption") == 0) {
                yaml_node_pair_t *sp = val_node->data.mapping.pairs.start;
                yaml_node_pair_t *se = val_node->data.mapping.pairs.top;
                for (; sp < se; sp++) {
                    yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                    yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                    if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                    if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                    const char *skey = (const char *)sk->data.scalar.value;
                    const char *sval = (const char *)sv->data.scalar.value;
                    if (strcmp(skey, "text") == 0)
                        yaml_copy_field(out->assumption.text,
                                        sizeof(out->assumption.text), sval);
                    else if (strcmp(skey, "status") == 0)
                        yaml_copy_field(out->assumption.status,
                                        sizeof(out->assumption.status), sval);
                    else if (strcmp(skey, "source") == 0)
                        yaml_copy_field(out->assumption.source,
                                        sizeof(out->assumption.source), sval);
                }
                continue;
            }
            if (strcmp(key, "constraint") == 0) {
                yaml_node_pair_t *sp = val_node->data.mapping.pairs.start;
                yaml_node_pair_t *se = val_node->data.mapping.pairs.top;
                for (; sp < se; sp++) {
                    yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                    yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                    if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                    if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                    const char *skey = (const char *)sk->data.scalar.value;
                    const char *sval = (const char *)sv->data.scalar.value;
                    if (strcmp(skey, "text") == 0)
                        yaml_copy_field(out->constraint.text,
                                        sizeof(out->constraint.text), sval);
                    else if (strcmp(skey, "kind") == 0)
                        yaml_copy_field(out->constraint.kind,
                                        sizeof(out->constraint.kind), sval);
                    else if (strcmp(skey, "source") == 0)
                        yaml_copy_field(out->constraint.source,
                                        sizeof(out->constraint.source), sval);
                }
                continue;
            }
            if (strcmp(key, "doc_meta") == 0) {
                yaml_node_pair_t *sp = val_node->data.mapping.pairs.start;
                yaml_node_pair_t *se = val_node->data.mapping.pairs.top;
                for (; sp < se; sp++) {
                    yaml_node_t *sk = yaml_document_get_node(doc, sp->key);
                    yaml_node_t *sv = yaml_document_get_node(doc, sp->value);
                    if (!sk || sk->type != YAML_SCALAR_NODE) continue;
                    if (!sv || sv->type != YAML_SCALAR_NODE) continue;
                    const char *skey = (const char *)sk->data.scalar.value;
                    const char *sval = (const char *)sv->data.scalar.value;
                    if (strcmp(skey, "title") == 0)
                        yaml_copy_field(out->doc_meta.title,
                                        sizeof(out->doc_meta.title), sval);
                    else if (strcmp(skey, "doc_type") == 0)
                        yaml_copy_field(out->doc_meta.doc_type,
                                        sizeof(out->doc_meta.doc_type), sval);
                    else if (strcmp(skey, "version") == 0)
                        yaml_copy_field(out->doc_meta.version,
                                        sizeof(out->doc_meta.version), sval);
                    else if (strcmp(skey, "client") == 0)
                        yaml_copy_field(out->doc_meta.client,
                                        sizeof(out->doc_meta.client), sval);
                    else if (strcmp(skey, "status") == 0)
                        yaml_copy_field(out->doc_meta.status,
                                        sizeof(out->doc_meta.status), sval);
                }
                continue;
            }
        }
    }

    /* Derive kind from type_raw now that all fields have been read. */
    out->identity.kind = entity_kind_from_string(out->identity.type_raw);
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

int yaml_parse_entity(const char *path, Entity *out)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    memset(out, 0, sizeof(*out));
    yaml_copy_field(out->identity.file_path, sizeof(out->identity.file_path),
                    path);

    yaml_parser_t   parser;
    yaml_document_t doc;
    int             rc = -1;

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);

    if (yaml_parser_load(&parser, &doc)) {
        yaml_node_t *root = yaml_document_get_root_node(&doc);
        extract_entity_fields(&doc, root, out);
        yaml_document_delete(&doc);
        rc = (out->identity.id[0] != '\0') ? 0 : -1;
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

    while (1) {
        yaml_document_t doc;
        if (!yaml_parser_load(&parser, &doc))
            break; /* parse error */

        yaml_node_t *root = yaml_document_get_root_node(&doc);
        if (!root) {
            yaml_document_delete(&doc);
            break; /* end of stream */
        }

        Entity entity;
        memset(&entity, 0, sizeof(entity));
        yaml_copy_field(entity.identity.file_path,
                        sizeof(entity.identity.file_path), path);
        entity.identity.doc_index = doc_idx++;

        extract_entity_fields(&doc, root, &entity);
        yaml_document_delete(&doc);

        if (entity.identity.id[0] != '\0') {
            if (entity_list_add(list, &entity) == 0) {
                added++;
            } else {
                fprintf(stderr,
                        "warning: out of memory, skipping document in: %s\n",
                        path);
            }
        }
        /* entity_list_add() deep-copies all heap fields; free the local copy. */
        entity_free(&entity);
    }

    yaml_parser_delete(&parser);
    fclose(f);
    return added;
}

int entity_traceability_to_triplets(const Entity *entity, TripletStore *store)
{
    if (!entity || !store)
        return -1;

    const char *subject = entity->identity.id;
    if (subject[0] == '\0')
        return 0;

    const char *buf = entity->traceability.entries;
    if (buf[0] == '\0')
        return 0;

    int added = 0;
    const char *p = buf;

    while (*p) {
        /* Each entry: "target\trelation" separated by '\n' */
        const char *tab = strchr(p, '\t');
        if (!tab)
            break;

        const char *nl  = strchr(tab + 1, '\n');
        const char *end = nl ? nl : p + strlen(p);

        size_t t_len = (size_t)(tab - p);
        size_t r_len = (size_t)(end - (tab + 1));

        if (t_len > 0 && t_len < LINK_TARGET_LEN &&
            r_len > 0 && r_len < LINK_RELATION_LEN) {
            char target[LINK_TARGET_LEN]     = {0};
            char relation[LINK_RELATION_LEN] = {0};
            memcpy(target,   p,       t_len);
            memcpy(relation, tab + 1, r_len);

            size_t id = triplet_store_add(store, subject, relation, target);
            if (id != TRIPLE_ID_INVALID)
                added++;
        }

        p = nl ? nl + 1 : end;
    }

    return added;
}
