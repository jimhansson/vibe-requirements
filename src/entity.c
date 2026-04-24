#define _POSIX_C_SOURCE 200809L  /* for strdup */
#include "entity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Entity heap lifecycle
 * --------------------------------------------------------------------- */

void entity_free(Entity *entity)
{
    if (!entity)
        return;

    free(entity->doc_body.body);
    entity->doc_body.body = NULL;

    free(entity->test_procedure.preconditions);
    entity->test_procedure.preconditions = NULL;
    free(entity->test_procedure.steps);
    entity->test_procedure.steps = NULL;
    free(entity->test_procedure.expected_result);
    entity->test_procedure.expected_result = NULL;

    free(entity->clause_collection.clauses);
    entity->clause_collection.clauses = NULL;

    free(entity->attachment.attachments);
    entity->attachment.attachments = NULL;
}

int entity_copy(Entity *dst, const Entity *src)
{
    /* Shallow copy covers all fixed-size fields. */
    *dst = *src;

    /* Null out heap pointers in dst before deep copying, so that a partial
     * failure in entity_free(dst) does not double-free src's buffers. */
    dst->doc_body.body                 = NULL;
    dst->test_procedure.preconditions  = NULL;
    dst->test_procedure.steps          = NULL;
    dst->test_procedure.expected_result = NULL;
    dst->clause_collection.clauses     = NULL;
    dst->attachment.attachments        = NULL;

#define DUP_FIELD(ptr) \
    do { \
        if (src->ptr) { \
            dst->ptr = strdup(src->ptr); \
            if (!dst->ptr) goto oom; \
        } \
    } while (0)

    DUP_FIELD(doc_body.body);
    DUP_FIELD(test_procedure.preconditions);
    DUP_FIELD(test_procedure.steps);
    DUP_FIELD(test_procedure.expected_result);
    DUP_FIELD(clause_collection.clauses);
    DUP_FIELD(attachment.attachments);

#undef DUP_FIELD
    return 0;

oom:
    entity_free(dst);
    return -1;
}

/* -----------------------------------------------------------------------
 * EntityList lifecycle
 * --------------------------------------------------------------------- */

void entity_list_init(EntityList *list)
{
    list->items    = NULL;
    list->count    = 0;
    list->capacity = 0;
}

int entity_list_add(EntityList *list, const Entity *entity)
{
    if (list->count == list->capacity) {
        int new_cap = (list->capacity == 0) ? 16 : list->capacity * 2;
        Entity *tmp = realloc(list->items, (size_t)new_cap * sizeof(Entity));
        if (!tmp)
            return -1;
        list->items    = tmp;
        list->capacity = new_cap;
    }
    if (entity_copy(&list->items[list->count], entity) != 0)
        return -1;
    list->count++;
    return 0;
}

void entity_list_free(EntityList *list)
{
    for (int i = 0; i < list->count; i++)
        entity_free(&list->items[i]);
    free(list->items);
    list->items    = NULL;
    list->count    = 0;
    list->capacity = 0;
}

/* -----------------------------------------------------------------------
 * Kind helpers
 * --------------------------------------------------------------------- */

EntityKind entity_kind_from_string(const char *type_str)
{
    if (!type_str || type_str[0] == '\0')
        return ENTITY_KIND_REQUIREMENT;

    if (strcmp(type_str, "requirement")   == 0 ||
        strcmp(type_str, "functional")    == 0 ||
        strcmp(type_str, "non-functional") == 0 ||
        strcmp(type_str, "nonfunctional")  == 0)
        return ENTITY_KIND_REQUIREMENT;

    if (strcmp(type_str, "group") == 0)
        return ENTITY_KIND_GROUP;

    if (strcmp(type_str, "story")      == 0 ||
        strcmp(type_str, "user-story") == 0)
        return ENTITY_KIND_STORY;

    if (strcmp(type_str, "design-note") == 0 ||
        strcmp(type_str, "design_note") == 0 ||
        strcmp(type_str, "design")      == 0)
        return ENTITY_KIND_DESIGN_NOTE;

    if (strcmp(type_str, "section") == 0)
        return ENTITY_KIND_SECTION;

    if (strcmp(type_str, "assumption") == 0)
        return ENTITY_KIND_ASSUMPTION;

    if (strcmp(type_str, "constraint") == 0)
        return ENTITY_KIND_CONSTRAINT;

    if (strcmp(type_str, "test-case") == 0 ||
        strcmp(type_str, "test_case") == 0 ||
        strcmp(type_str, "test")      == 0)
        return ENTITY_KIND_TEST_CASE;

    if (strcmp(type_str, "external")   == 0 ||
        strcmp(type_str, "directive")  == 0 ||
        strcmp(type_str, "standard")   == 0 ||
        strcmp(type_str, "regulation") == 0)
        return ENTITY_KIND_EXTERNAL;

    if (strcmp(type_str, "document") == 0 ||
        strcmp(type_str, "srs")      == 0 ||
        strcmp(type_str, "sdd")      == 0)
        return ENTITY_KIND_DOCUMENT;

    return ENTITY_KIND_UNKNOWN;
}

const char *entity_kind_label(EntityKind kind)
{
    switch (kind) {
    case ENTITY_KIND_REQUIREMENT: return "requirement";
    case ENTITY_KIND_GROUP:       return "group";
    case ENTITY_KIND_STORY:       return "story";
    case ENTITY_KIND_DESIGN_NOTE: return "design-note";
    case ENTITY_KIND_SECTION:     return "section";
    case ENTITY_KIND_ASSUMPTION:  return "assumption";
    case ENTITY_KIND_CONSTRAINT:  return "constraint";
    case ENTITY_KIND_TEST_CASE:   return "test-case";
    case ENTITY_KIND_EXTERNAL:    return "external";
    case ENTITY_KIND_DOCUMENT:    return "document";
    default:                      return "unknown";
    }
}

int entity_has_component(const Entity *entity, const char *comp)
{
    if (!comp || comp[0] == '\0')
        return 1; /* no filter — always matches */

    if (strcmp(comp, "user-story") == 0 || strcmp(comp, "user_story") == 0)
        return entity->user_story.role[0] != '\0' ||
               entity->user_story.goal[0] != '\0';

    if (strcmp(comp, "acceptance-criteria") == 0 ||
        strcmp(comp, "acceptance_criteria") == 0)
        return entity->acceptance_criteria.count > 0;

    if (strcmp(comp, "epic") == 0 ||
        strcmp(comp, "epic-membership") == 0 ||
        strcmp(comp, "epic_membership") == 0)
        return entity->epic_membership.epic_id[0] != '\0';

    if (strcmp(comp, "assumption") == 0)
        return entity->assumption.text[0] != '\0';

    if (strcmp(comp, "constraint") == 0)
        return entity->constraint.text[0] != '\0';

    if (strcmp(comp, "doc-meta") == 0 || strcmp(comp, "doc_meta") == 0)
        return entity->doc_meta.doc_type[0] != '\0' ||
               entity->doc_meta.title[0] != '\0';

    if (strcmp(comp, "doc-membership") == 0 || strcmp(comp, "documents") == 0)
        return entity->doc_membership.count > 0;

    if (strcmp(comp, "doc-body") == 0 || strcmp(comp, "body") == 0)
        return entity->doc_body.body != NULL && entity->doc_body.body[0] != '\0';

    if (strcmp(comp, "traceability") == 0)
        return entity->traceability.count > 0;

    if (strcmp(comp, "sources") == 0)
        return entity->sources.count > 0;

    if (strcmp(comp, "tags") == 0)
        return entity->tags.count > 0;

    if (strcmp(comp, "test-procedure") == 0 ||
        strcmp(comp, "test_procedure") == 0)
        return entity->test_procedure.precondition_count > 0 ||
               entity->test_procedure.step_count > 0 ||
               (entity->test_procedure.expected_result != NULL &&
                entity->test_procedure.expected_result[0] != '\0');

    if (strcmp(comp, "clause-collection") == 0 ||
        strcmp(comp, "clause_collection") == 0 ||
        strcmp(comp, "clauses") == 0)
        return entity->clause_collection.count > 0;

    if (strcmp(comp, "attachment") == 0 ||
        strcmp(comp, "attachments") == 0)
        return entity->attachment.count > 0;

    return 0; /* unrecognised component name */
}

/* -----------------------------------------------------------------------
 * Sorting comparator
 * --------------------------------------------------------------------- */

int entity_cmp_by_id(const void *a, const void *b)
{
    const Entity *ea = (const Entity *)a;
    const Entity *eb = (const Entity *)b;
    return strcmp(ea->identity.id, eb->identity.id);
}

/* -----------------------------------------------------------------------
 * Entity list filtering
 * --------------------------------------------------------------------- */

void entity_apply_filter(const EntityList *src, EntityList *dst,
                         const char *filter_kind,
                         const char *filter_comp,
                         const char *filter_status,
                         const char *filter_priority)
{
    /* Resolve kind filter once up-front. */
    int has_kind = (filter_kind && filter_kind[0] != '\0');
    EntityKind kind_val = ENTITY_KIND_UNKNOWN;
    if (has_kind) {
        kind_val = entity_kind_from_string(filter_kind);
        if (kind_val == ENTITY_KIND_UNKNOWN &&
            strcmp(filter_kind, "unknown") != 0) {
            fprintf(stderr, "warning: unrecognised kind '%s'\n", filter_kind);
        }
    }

    for (int i = 0; i < src->count; i++) {
        const Entity *e = &src->items[i];

        if (has_kind && e->identity.kind != kind_val)
            continue;

        if (filter_comp && filter_comp[0] != '\0' &&
            !entity_has_component(e, filter_comp)) {
            continue;
        }

        if (filter_status && filter_status[0] != '\0' &&
            strcmp(e->lifecycle.status, filter_status) != 0) {
            continue;
        }

        if (filter_priority && filter_priority[0] != '\0' &&
            strcmp(e->lifecycle.priority, filter_priority) != 0) {
            continue;
        }

        entity_list_add(dst, e);
    }
}
