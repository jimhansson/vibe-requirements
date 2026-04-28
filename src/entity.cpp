/**
 * @file entity.cpp
 * @brief Entity helper functions (C++ edition).
 */

#include "entity.h"
#include <cstring>
#include <string>

EntityKind entity_kind_from_string(const char *type_str)
{
    if (!type_str) return ENTITY_KIND_UNKNOWN;
    if (strcmp(type_str, "requirement") == 0 ||
        strcmp(type_str, "functional")  == 0 ||
        strcmp(type_str, "non-functional") == 0)
        return ENTITY_KIND_REQUIREMENT;
    if (strcmp(type_str, "group") == 0)
        return ENTITY_KIND_GROUP;
    if (strcmp(type_str, "story") == 0 ||
        strcmp(type_str, "user-story") == 0)
        return ENTITY_KIND_STORY;
    if (strcmp(type_str, "design-note") == 0 ||
        strcmp(type_str, "design") == 0)
        return ENTITY_KIND_DESIGN_NOTE;
    if (strcmp(type_str, "section") == 0)
        return ENTITY_KIND_SECTION;
    if (strcmp(type_str, "assumption") == 0)
        return ENTITY_KIND_ASSUMPTION;
    if (strcmp(type_str, "constraint") == 0)
        return ENTITY_KIND_CONSTRAINT;
    if (strcmp(type_str, "test-case") == 0 ||
        strcmp(type_str, "test") == 0)
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
    if (strcmp(type_str, "document-schema") == 0)
        return ENTITY_KIND_DOCUMENT_SCHEMA;
    return ENTITY_KIND_UNKNOWN;
}

const char *entity_kind_label(EntityKind kind)
{
    switch (kind) {
    case ENTITY_KIND_REQUIREMENT:      return "requirement";
    case ENTITY_KIND_GROUP:            return "group";
    case ENTITY_KIND_STORY:            return "story";
    case ENTITY_KIND_DESIGN_NOTE:      return "design-note";
    case ENTITY_KIND_SECTION:          return "section";
    case ENTITY_KIND_ASSUMPTION:       return "assumption";
    case ENTITY_KIND_CONSTRAINT:       return "constraint";
    case ENTITY_KIND_TEST_CASE:        return "test-case";
    case ENTITY_KIND_EXTERNAL:         return "external";
    case ENTITY_KIND_DOCUMENT:         return "document";
    case ENTITY_KIND_DOCUMENT_SCHEMA:  return "document-schema";
    case ENTITY_KIND_UNKNOWN:          /* fall through */
    default:                           return "unknown";
    }
}

int entity_has_component(const Entity *entity, const char *comp)
{
    if (!entity || !comp) return 0;
    if (strcmp(comp, "user-story") == 0)
        return (!entity->user_story.role.empty() ||
                !entity->user_story.goal.empty() ||
                !entity->user_story.reason.empty()) ? 1 : 0;
    if (strcmp(comp, "acceptance-criteria") == 0)
        return !entity->acceptance_criteria.criteria.empty() ? 1 : 0;
    if (strcmp(comp, "epic") == 0)
        return !entity->epic_membership.epic_id.empty() ? 1 : 0;
    if (strcmp(comp, "assumption") == 0)
        return !entity->assumption.text.empty() ? 1 : 0;
    if (strcmp(comp, "constraint") == 0)
        return !entity->constraint.text.empty() ? 1 : 0;
    if (strcmp(comp, "doc-meta") == 0)
        return (!entity->doc_meta.doc_type.empty() ||
                !entity->doc_meta.title.empty()) ? 1 : 0;
    if (strcmp(comp, "doc-membership") == 0)
        return !entity->doc_membership.doc_ids.empty() ? 1 : 0;
    if (strcmp(comp, "doc-body") == 0)
        return !entity->doc_body.body.empty() ? 1 : 0;
    if (strcmp(comp, "traceability") == 0)
        return !entity->traceability.entries.empty() ? 1 : 0;
    if (strcmp(comp, "tags") == 0)
        return !entity->tags.tags.empty() ? 1 : 0;
    if (strcmp(comp, "sources") == 0)
        return !entity->sources.sources.empty() ? 1 : 0;
    if (strcmp(comp, "test-procedure") == 0)
        return (!entity->test_procedure.steps.empty() ||
                !entity->test_procedure.preconditions.empty() ||
                !entity->test_procedure.expected_result.empty()) ? 1 : 0;
    if (strcmp(comp, "clauses") == 0)
        return !entity->clause_collection.clauses.empty() ? 1 : 0;
    if (strcmp(comp, "attachments") == 0)
        return !entity->attachment.attachments.empty() ? 1 : 0;
    if (strcmp(comp, "applies-to") == 0)
        return !entity->applies_to.applies_to.empty() ? 1 : 0;
    if (strcmp(comp, "composition-profile") == 0)
        return !entity->composition_profile.order.empty() ? 1 : 0;
    if (strcmp(comp, "render-profile") == 0)
        return !entity->render_profile.format.empty() ? 1 : 0;
    if (strcmp(comp, "variant-profile") == 0)
        return (!entity->variant_profile.customer.empty() ||
                !entity->variant_profile.product.empty()) ? 1 : 0;
    return 0;
}

int entity_cmp_by_id(const void *a, const void *b)
{
    const Entity *ea = static_cast<const Entity *>(a);
    const Entity *eb = static_cast<const Entity *>(b);
    return ea->identity.id.compare(eb->identity.id);
}

void entity_apply_filter(const EntityList *src, EntityList *dst,
                         const char *filter_kind,
                         const char *filter_comp,
                         const char *filter_status,
                         const char *filter_priority)
{
    if (!src || !dst) return;
    for (const auto &e : *src) {
        if (filter_kind && *filter_kind) {
            EntityKind wanted = entity_kind_from_string(filter_kind);
            if (e.identity.kind != wanted)
                continue;
        }
        if (filter_comp && *filter_comp) {
            if (!entity_has_component(&e, filter_comp))
                continue;
        }
        if (filter_status && *filter_status) {
            if (e.lifecycle.status != filter_status)
                continue;
        }
        if (filter_priority && *filter_priority) {
            if (e.lifecycle.priority != filter_priority)
                continue;
        }
        dst->push_back(e);
    }
}
