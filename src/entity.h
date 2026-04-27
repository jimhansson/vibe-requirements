/**
 * @file entity.h
 * @brief ECS-inspired entity model for vibe-req (C++ edition).
 */

#ifndef VIBE_ENTITY_H
#define VIBE_ENTITY_H

#include <string>
#include <vector>
#include <utility>

enum EntityKind {
    ENTITY_KIND_REQUIREMENT = 0,
    ENTITY_KIND_GROUP,
    ENTITY_KIND_STORY,
    ENTITY_KIND_DESIGN_NOTE,
    ENTITY_KIND_SECTION,
    ENTITY_KIND_ASSUMPTION,
    ENTITY_KIND_CONSTRAINT,
    ENTITY_KIND_TEST_CASE,
    ENTITY_KIND_EXTERNAL,
    ENTITY_KIND_DOCUMENT,
    ENTITY_KIND_DOCUMENT_SCHEMA,
    ENTITY_KIND_UNKNOWN,
};

struct IdentityComponent {
    std::string  id;
    std::string  title;
    std::string  type_raw;
    std::string  file_path;
    int          doc_index = 0;
    EntityKind   kind      = ENTITY_KIND_UNKNOWN;
};

struct LifecycleComponent {
    std::string status;
    std::string priority;
    std::string owner;
    std::string version;
};

struct TextComponent {
    std::string description;
    std::string rationale;
};

struct TagComponent {
    std::vector<std::string> tags;
};

struct UserStoryComponent {
    std::string role;
    std::string goal;
    std::string reason;
};

struct AcceptanceCriteriaComponent {
    std::vector<std::string> criteria;
};

struct EpicMembershipComponent {
    std::string epic_id;
};

struct AssumptionComponent {
    std::string text;
    std::string status;
    std::string source;
};

struct ConstraintComponent {
    std::string text;
    std::string kind;
    std::string source;
};

struct DocumentMetaComponent {
    std::string title;
    std::string doc_type;
    std::string version;
    std::string client;
    std::string status;
};

struct DocumentMembershipComponent {
    std::vector<std::string> doc_ids;
};

struct DocumentBodyComponent {
    std::string body;
};

struct TraceabilityComponent {
    std::vector<std::pair<std::string, std::string>> entries;
};

struct SourceComponent {
    std::vector<std::string> sources;
};

struct TestProcedureComponent {
    std::vector<std::string>                          preconditions;
    std::vector<std::pair<std::string, std::string>>  steps;
    std::string                                        expected_result;
};

struct ClauseCollectionComponent {
    std::vector<std::pair<std::string, std::string>> clauses;
};

struct AttachmentComponent {
    std::vector<std::pair<std::string, std::string>> attachments;
};

struct AppliesToComponent {
    std::vector<std::string> applies_to;
};

struct VariantProfileComponent {
    std::string customer;
    std::string product;
};

struct CompositionProfileComponent {
    std::vector<std::string> order;
};

struct RenderProfileComponent {
    std::string format;
};

struct Entity {
    IdentityComponent           identity;
    LifecycleComponent          lifecycle;
    TextComponent               text;
    TagComponent                tags;
    UserStoryComponent          user_story;
    AcceptanceCriteriaComponent acceptance_criteria;
    EpicMembershipComponent     epic_membership;
    AssumptionComponent         assumption;
    ConstraintComponent         constraint;
    DocumentMetaComponent       doc_meta;
    DocumentMembershipComponent doc_membership;
    DocumentBodyComponent       doc_body;
    TraceabilityComponent       traceability;
    SourceComponent             sources;
    TestProcedureComponent      test_procedure;
    ClauseCollectionComponent   clause_collection;
    AttachmentComponent         attachment;
    AppliesToComponent          applies_to;
    VariantProfileComponent     variant_profile;
    CompositionProfileComponent composition_profile;
    RenderProfileComponent      render_profile;
};

using EntityList = std::vector<Entity>;

EntityKind  entity_kind_from_string(const char *type_str);
const char *entity_kind_label(EntityKind kind);

int  entity_has_component(const Entity *entity, const char *comp);
int  entity_cmp_by_id(const void *a, const void *b);
void entity_apply_filter(const EntityList *src, EntityList *dst,
                         const char *filter_kind,
                         const char *filter_comp,
                         const char *filter_status,
                         const char *filter_priority);

#endif /* VIBE_ENTITY_H */
