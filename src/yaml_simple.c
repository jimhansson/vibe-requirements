#include "yaml_simple.h"
#include "entity.h"
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

int yaml_parse_requirements(const char *path, RequirementList *list)
{
    if (!path || !list)
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
            break; /* parse error */

        yaml_node_t *root = yaml_document_get_root_node(&doc);
        if (!root) {
            yaml_document_delete(&doc);
            break; /* end of stream */
        }

        Requirement req;
        memset(&req, 0, sizeof(req));
        copy_field(req.file_path, REQ_PATH_LEN, path);
        extract_fields(&doc, root, &req);
        yaml_document_delete(&doc);

        if (req.id[0] != '\0') {
            if (req_list_add(list, &req) == 0) {
                added++;
            } else {
                fprintf(stderr, "warning: out of memory, skipping document in: %s\n", path);
            }
        }
    }

    yaml_parser_delete(&parser);
    fclose(f);
    return added;
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

/* =========================================================================
 * Entity parsing — ECS-based parsing functions
 * ======================================================================= */

/*
 * Append text to a newline-separated flat buffer.
 * Returns 1 if the text was appended, 0 if there was not enough space.
 */
static int append_to_flat(char *buf, size_t buf_size, int *count, const char *text)
{
    size_t cur_len  = strlen(buf);
    size_t text_len = strlen(text);

    /* Need room for text + newline (or just text if buffer is empty) */
    size_t need = text_len + (cur_len > 0 ? 1u : 0u);

    if (cur_len + need >= buf_size)
        return 0; /* not enough space */

    if (cur_len > 0)
        buf[cur_len++] = '\n';

    memcpy(buf + cur_len, text, text_len);
    buf[cur_len + text_len] = '\0';
    (*count)++;
    return 1;
}

/*
 * Walk a YAML sequence node and collect scalar items into a flat buffer.
 */
static void collect_sequence(yaml_document_t *doc, yaml_node_t *seq,
                              char *buf, size_t buf_size, int *count)
{
    if (!seq || seq->type != YAML_SEQUENCE_NODE)
        return;

    yaml_node_item_t *item = seq->data.sequence.items.start;
    yaml_node_item_t *top  = seq->data.sequence.items.top;

    for (; item < top; item++) {
        yaml_node_t *node = yaml_document_get_node(doc, *item);
        if (!node || node->type != YAML_SCALAR_NODE)
            continue;
        const char *val = (const char *)node->data.scalar.value;
        append_to_flat(buf, buf_size, count, val);
    }
}

/*
 * Walk the top-level mapping of a YAML document and populate an Entity.
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

        /* Scalar fields */
        if (val_node && val_node->type == YAML_SCALAR_NODE) {
            const char *val = (const char *)val_node->data.scalar.value;

#define COPY_FIELD_IF_MATCH(field_key, dst, dst_size)              \
            if (strcmp(key, field_key) == 0) {        \
                copy_field(dst, dst_size, val);       \
                continue;                             \
            }

            COPY_FIELD_IF_MATCH("id",              out->identity.id,              sizeof(out->identity.id))
            COPY_FIELD_IF_MATCH("title",           out->identity.title,           sizeof(out->identity.title))
            COPY_FIELD_IF_MATCH("type",            out->identity.type_raw,        sizeof(out->identity.type_raw))
            COPY_FIELD_IF_MATCH("status",          out->lifecycle.status,         sizeof(out->lifecycle.status))
            COPY_FIELD_IF_MATCH("priority",        out->lifecycle.priority,       sizeof(out->lifecycle.priority))
            COPY_FIELD_IF_MATCH("owner",           out->lifecycle.owner,          sizeof(out->lifecycle.owner))
            COPY_FIELD_IF_MATCH("version",         out->lifecycle.version,        sizeof(out->lifecycle.version))
            COPY_FIELD_IF_MATCH("description",     out->text.description,         sizeof(out->text.description))
            COPY_FIELD_IF_MATCH("rationale",       out->text.rationale,           sizeof(out->text.rationale))
            /* user-story component — canonical keys */
            COPY_FIELD_IF_MATCH("role",            out->user_story.role,          sizeof(out->user_story.role))
            COPY_FIELD_IF_MATCH("goal",            out->user_story.goal,          sizeof(out->user_story.goal))
            COPY_FIELD_IF_MATCH("reason",          out->user_story.reason,        sizeof(out->user_story.reason))
            /* user-story component — legacy aliases for backward compatibility */
            COPY_FIELD_IF_MATCH("as_a",            out->user_story.role,          sizeof(out->user_story.role))
            COPY_FIELD_IF_MATCH("i_want",          out->user_story.goal,          sizeof(out->user_story.goal))
            COPY_FIELD_IF_MATCH("so_that",         out->user_story.reason,        sizeof(out->user_story.reason))
            /* epic-membership component */
            COPY_FIELD_IF_MATCH("epic",            out->epic_membership.epic_id,  sizeof(out->epic_membership.epic_id))
            COPY_FIELD_IF_MATCH("risk_if_false",   out->assumption.risk_if_false, sizeof(out->assumption.risk_if_false))
            COPY_FIELD_IF_MATCH("constraint_type", out->constraint.constraint_type, sizeof(out->constraint.constraint_type))
            COPY_FIELD_IF_MATCH("body",            out->doc_body.body,            sizeof(out->doc_body.body))

            /* "statement" goes to assumption or constraint depending on kind
             * (kind is derived after all fields are read; store in both and
             * let the caller use the right one, or we defer kind derivation).
             * We store it in both slots and zero the wrong one afterwards.   */
            if (strcmp(key, "statement") == 0) {
                copy_field(out->assumption.statement,  sizeof(out->assumption.statement),  val);
                copy_field(out->constraint.statement,  sizeof(out->constraint.statement),  val);
                continue;
            }

#undef COPY_FIELD_IF_MATCH
        }

        /* Sequence fields */
        if (val_node && val_node->type == YAML_SEQUENCE_NODE) {
            if (strcmp(key, "tags") == 0) {
                collect_sequence(doc, val_node,
                                 out->tags.tags, sizeof(out->tags.tags),
                                 &out->tags.count);
                continue;
            }
            if (strcmp(key, "acceptance_criteria") == 0) {
                collect_sequence(doc, val_node,
                                 out->acceptance_criteria.criteria,
                                 sizeof(out->acceptance_criteria.criteria),
                                 &out->acceptance_criteria.count);
                continue;
            }
        }
    }

    /* Derive kind from type_raw now that all scalar fields have been read. */
    out->identity.kind = entity_kind_from_string(out->identity.type_raw);

    /* Zero the "statement" field in the component that does not apply. */
    if (out->identity.kind != ENTITY_KIND_CONSTRAINT)
        memset(out->constraint.statement, 0, sizeof(out->constraint.statement));
    if (out->identity.kind != ENTITY_KIND_ASSUMPTION)
        memset(out->assumption.statement, 0, sizeof(out->assumption.statement));
}

int yaml_parse_entity(const char *path, Entity *out)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    memset(out, 0, sizeof(*out));
    copy_field(out->identity.file_path, sizeof(out->identity.file_path), path);

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
        copy_field(entity.identity.file_path, sizeof(entity.identity.file_path), path);
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
    }

    yaml_parser_delete(&parser);
    fclose(f);
    return added;
}
