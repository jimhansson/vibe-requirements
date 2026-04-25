/**
 * @file test_entity.cpp
 * @brief Unit tests for entity.c and the new yaml_parse_entity /
 *        yaml_parse_entities functions in yaml_simple.c.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>
#include <string>

extern "C" {
#include "entity.h"
#include "yaml_simple.h"
}

/* -------------------------------------------------------------------------
 * Helpers — write temporary YAML files into /tmp
 * ---------------------------------------------------------------------- */

static const char *write_yaml(const char *filename, const char *content)
{
    static char path[512];
    snprintf(path, sizeof(path), "/tmp/%s", filename);
    FILE *f = fopen(path, "w");
    if (!f)
        return nullptr;
    fputs(content, f);
    fclose(f);
    return path;
}

/* =========================================================================
 * Tests — entity_kind_from_string
 * ======================================================================= */

TEST(EntityKindTest, NullAndEmptyMapsToRequirement)
{
    EXPECT_EQ(entity_kind_from_string(nullptr),  ENTITY_KIND_REQUIREMENT);
    EXPECT_EQ(entity_kind_from_string(""),        ENTITY_KIND_REQUIREMENT);
}

TEST(EntityKindTest, FunctionalVariants)
{
    EXPECT_EQ(entity_kind_from_string("requirement"),   ENTITY_KIND_REQUIREMENT);
    EXPECT_EQ(entity_kind_from_string("functional"),    ENTITY_KIND_REQUIREMENT);
    EXPECT_EQ(entity_kind_from_string("non-functional"), ENTITY_KIND_REQUIREMENT);
    EXPECT_EQ(entity_kind_from_string("nonfunctional"),  ENTITY_KIND_REQUIREMENT);
}

TEST(EntityKindTest, Group)
{
    EXPECT_EQ(entity_kind_from_string("group"), ENTITY_KIND_GROUP);
}

TEST(EntityKindTest, Story)
{
    EXPECT_EQ(entity_kind_from_string("story"),      ENTITY_KIND_STORY);
    EXPECT_EQ(entity_kind_from_string("user-story"), ENTITY_KIND_STORY);
}

TEST(EntityKindTest, DesignNote)
{
    EXPECT_EQ(entity_kind_from_string("design-note"), ENTITY_KIND_DESIGN_NOTE);
    EXPECT_EQ(entity_kind_from_string("design_note"), ENTITY_KIND_DESIGN_NOTE);
    EXPECT_EQ(entity_kind_from_string("design"),      ENTITY_KIND_DESIGN_NOTE);
}

TEST(EntityKindTest, Section)
{
    EXPECT_EQ(entity_kind_from_string("section"), ENTITY_KIND_SECTION);
}

TEST(EntityKindTest, Assumption)
{
    EXPECT_EQ(entity_kind_from_string("assumption"), ENTITY_KIND_ASSUMPTION);
}

TEST(EntityKindTest, Constraint)
{
    EXPECT_EQ(entity_kind_from_string("constraint"), ENTITY_KIND_CONSTRAINT);
}

TEST(EntityKindTest, TestCase)
{
    EXPECT_EQ(entity_kind_from_string("test-case"), ENTITY_KIND_TEST_CASE);
    EXPECT_EQ(entity_kind_from_string("test_case"), ENTITY_KIND_TEST_CASE);
    EXPECT_EQ(entity_kind_from_string("test"),      ENTITY_KIND_TEST_CASE);
}

TEST(EntityKindTest, External)
{
    EXPECT_EQ(entity_kind_from_string("external"),   ENTITY_KIND_EXTERNAL);
    EXPECT_EQ(entity_kind_from_string("directive"),  ENTITY_KIND_EXTERNAL);
    EXPECT_EQ(entity_kind_from_string("standard"),   ENTITY_KIND_EXTERNAL);
    EXPECT_EQ(entity_kind_from_string("regulation"), ENTITY_KIND_EXTERNAL);
}

TEST(EntityKindTest, UnknownFallback)
{
    EXPECT_EQ(entity_kind_from_string("something-made-up"), ENTITY_KIND_UNKNOWN);
    EXPECT_EQ(entity_kind_from_string("FUNCTIONAL"),        ENTITY_KIND_UNKNOWN); /* case-sensitive */
}

/* =========================================================================
 * Tests — entity_kind_label
 * ======================================================================= */

TEST(EntityKindTest, Labels)
{
    EXPECT_STREQ(entity_kind_label(ENTITY_KIND_REQUIREMENT), "requirement");
    EXPECT_STREQ(entity_kind_label(ENTITY_KIND_GROUP),       "group");
    EXPECT_STREQ(entity_kind_label(ENTITY_KIND_STORY),       "story");
    EXPECT_STREQ(entity_kind_label(ENTITY_KIND_DESIGN_NOTE), "design-note");
    EXPECT_STREQ(entity_kind_label(ENTITY_KIND_SECTION),     "section");
    EXPECT_STREQ(entity_kind_label(ENTITY_KIND_ASSUMPTION),  "assumption");
    EXPECT_STREQ(entity_kind_label(ENTITY_KIND_CONSTRAINT),  "constraint");
    EXPECT_STREQ(entity_kind_label(ENTITY_KIND_TEST_CASE),   "test-case");
    EXPECT_STREQ(entity_kind_label(ENTITY_KIND_EXTERNAL),    "external");
    EXPECT_STREQ(entity_kind_label(ENTITY_KIND_UNKNOWN),     "unknown");
}

/* =========================================================================
 * Tests — entity_list_add / entity_list_free lifecycle
 * ======================================================================= */

TEST(EntityListTest, InitAndFree)
{
    EntityList list;
    entity_list_init(&list);
    EXPECT_EQ(list.count,    0);
    EXPECT_EQ(list.capacity, 0);
    EXPECT_EQ(list.items,    nullptr);
    entity_list_free(&list);
    EXPECT_EQ(list.count,    0);
    EXPECT_EQ(list.items,    nullptr);
}

TEST(EntityListTest, AddSingleEntity)
{
    EntityList list;
    entity_list_init(&list);

    Entity e;
    memset(&e, 0, sizeof(e));
    strncpy(e.identity.id,    "ENT-001", sizeof(e.identity.id) - 1);
    strncpy(e.identity.title, "My entity", sizeof(e.identity.title) - 1);
    e.identity.kind = ENTITY_KIND_REQUIREMENT;

    EXPECT_EQ(entity_list_add(&list, &e), 0);
    EXPECT_EQ(list.count, 1);
    EXPECT_STREQ(list.items[0].identity.id,    "ENT-001");
    EXPECT_STREQ(list.items[0].identity.title, "My entity");

    entity_list_free(&list);
}

TEST(EntityListTest, AddMultipleEntitiesGrowsArray)
{
    EntityList list;
    entity_list_init(&list);

    for (int i = 0; i < 20; i++) {
        Entity e;
        memset(&e, 0, sizeof(e));
        snprintf(e.identity.id, sizeof(e.identity.id), "ENT-%03d", i);
        EXPECT_EQ(entity_list_add(&list, &e), 0);
    }
    EXPECT_EQ(list.count, 20);

    entity_list_free(&list);
}

/* =========================================================================
 * Tests — yaml_parse_entity
 * ======================================================================= */

TEST(YamlParseEntityTest, MinimalRequirementFile)
{
    const char *path = write_yaml("ent_req.yaml",
        "id: REQ-001\n"
        "title: Basic requirement\n"
        "type: functional\n"
        "status: draft\n"
        "priority: must\n"
        "description: A simple description.\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(e.identity.id,       "REQ-001");
    EXPECT_STREQ(e.identity.title,    "Basic requirement");
    EXPECT_STREQ(e.identity.type_raw, "functional");
    EXPECT_EQ(e.identity.kind,        ENTITY_KIND_REQUIREMENT);
    EXPECT_STREQ(e.lifecycle.status,   "draft");
    EXPECT_STREQ(e.lifecycle.priority, "must");
    EXPECT_STREQ(e.text.description,   "A simple description.");
}

TEST(YamlParseEntityTest, StoryFile)
{
    const char *path = write_yaml("ent_story.yaml",
        "id: STORY-001\n"
        "title: Login story\n"
        "type: story\n"
        "status: draft\n"
        "role: registered user\n"
        "goal: to log in with my email\n"
        "reason: I can access my account\n"
        "acceptance_criteria:\n"
        "  - Login form is shown\n"
        "  - Error displayed on wrong password\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(e.identity.id,        "STORY-001");
    EXPECT_EQ(e.identity.kind,         ENTITY_KIND_STORY);
    EXPECT_STREQ(e.user_story.role,    "registered user");
    EXPECT_STREQ(e.user_story.goal,    "to log in with my email");
    EXPECT_STREQ(e.user_story.reason,  "I can access my account");
    EXPECT_EQ(e.acceptance_criteria.count, 2);
    /* The two criteria should appear in the flat string. */
    EXPECT_NE(strstr(e.acceptance_criteria.criteria, "Login form is shown"), nullptr);
    EXPECT_NE(strstr(e.acceptance_criteria.criteria, "Error displayed on wrong password"), nullptr);
}

TEST(YamlParseEntityTest, StoryFileLegacyAliases)
{
    /* Verify that the old as_a / i_want / so_that keys still work. */
    const char *path = write_yaml("ent_story_legacy.yaml",
        "id: STORY-002\n"
        "title: Legacy story\n"
        "type: story\n"
        "as_a: developer\n"
        "i_want: faster builds\n"
        "so_that: I save time\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(e.user_story.role,   "developer");
    EXPECT_STREQ(e.user_story.goal,   "faster builds");
    EXPECT_STREQ(e.user_story.reason, "I save time");
}

TEST(YamlParseEntityTest, EpicMembership)
{
    /* Any entity can carry the epic-membership component. */
    const char *path = write_yaml("ent_epic.yaml",
        "id: STORY-003\n"
        "title: Story with epic\n"
        "type: story\n"
        "role: tester\n"
        "goal: run automated tests\n"
        "reason: quality is assured\n"
        "epic: EPIC-001\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(e.epic_membership.epic_id, "EPIC-001");
}

TEST(YamlParseEntityTest, UserStoryComponentOnNonStoryEntity)
{
    /* A requirement entity can also carry user-story and AC components. */
    const char *path = write_yaml("ent_req_story.yaml",
        "id: REQ-STORY-001\n"
        "title: Requirement with user story framing\n"
        "type: functional\n"
        "role: admin\n"
        "goal: manage users\n"
        "reason: the system stays secure\n"
        "epic: EPIC-ADMIN-001\n"
        "acceptance_criteria:\n"
        "  - Admin can create users\n"
        "  - Admin can delete users\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.kind,         ENTITY_KIND_REQUIREMENT);
    EXPECT_STREQ(e.user_story.role,    "admin");
    EXPECT_STREQ(e.user_story.goal,    "manage users");
    EXPECT_STREQ(e.user_story.reason,  "the system stays secure");
    EXPECT_STREQ(e.epic_membership.epic_id, "EPIC-ADMIN-001");
    EXPECT_EQ(e.acceptance_criteria.count, 2);
    EXPECT_NE(strstr(e.acceptance_criteria.criteria, "Admin can create users"), nullptr);
    EXPECT_NE(strstr(e.acceptance_criteria.criteria, "Admin can delete users"), nullptr);
}

TEST(YamlParseEntityTest, AssumptionFile)
{
    const char *path = write_yaml("ent_assumption.yaml",
        "id: ASSUM-001\n"
        "title: Network always available\n"
        "type: assumption\n"
        "assumption:\n"
        "  text: The network connection is always available during operation.\n"
        "  status: open\n"
        "  source: NET-SPEC-001\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(e.identity.id,   "ASSUM-001");
    EXPECT_EQ(e.identity.kind,    ENTITY_KIND_ASSUMPTION);
    EXPECT_NE(e.assumption.text[0], '\0');
    EXPECT_NE(strstr(e.assumption.text, "network connection"), nullptr);
    EXPECT_STREQ(e.assumption.status, "open");
    EXPECT_STREQ(e.assumption.source, "NET-SPEC-001");
    /* Constraint component must be zero (not set). */
    EXPECT_EQ(e.constraint.text[0], '\0');
}

TEST(YamlParseEntityTest, ConstraintFile)
{
    const char *path = write_yaml("ent_constraint.yaml",
        "id: CONSTR-001\n"
        "title: Must use TLS 1.3\n"
        "type: constraint\n"
        "constraint:\n"
        "  text: All communication must use TLS version 1.3 or higher.\n"
        "  kind: technical\n"
        "  source: SEC-POLICY-001\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(e.identity.id,    "CONSTR-001");
    EXPECT_EQ(e.identity.kind,     ENTITY_KIND_CONSTRAINT);
    EXPECT_STREQ(e.constraint.kind, "technical");
    EXPECT_NE(e.constraint.text[0], '\0');
    EXPECT_NE(strstr(e.constraint.text, "TLS version 1.3"), nullptr);
    EXPECT_STREQ(e.constraint.source, "SEC-POLICY-001");
    /* Assumption component must be zero (not set). */
    EXPECT_EQ(e.assumption.text[0], '\0');
}

TEST(YamlParseEntityTest, AnyEntityCanCarryAssumptionAndConstraint)
{
    /* A functional requirement that also carries both components. */
    const char *path = write_yaml("ent_req_with_components.yaml",
        "id: REQ-002\n"
        "title: Response time under load\n"
        "type: functional\n"
        "status: draft\n"
        "assumption:\n"
        "  text: The database server is always reachable.\n"
        "  status: open\n"
        "  source: INFRA-SPEC-001\n"
        "constraint:\n"
        "  text: Response time must not exceed 200 ms under peak load.\n"
        "  kind: technical\n"
        "  source: PERF-POLICY-001\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(e.identity.id,   "REQ-002");
    EXPECT_EQ(e.identity.kind,    ENTITY_KIND_REQUIREMENT);

    /* Assumption component populated despite kind != ENTITY_KIND_ASSUMPTION. */
    EXPECT_NE(e.assumption.text[0], '\0');
    EXPECT_NE(strstr(e.assumption.text, "database server"), nullptr);
    EXPECT_STREQ(e.assumption.status, "open");
    EXPECT_STREQ(e.assumption.source, "INFRA-SPEC-001");

    /* Constraint component populated despite kind != ENTITY_KIND_CONSTRAINT. */
    EXPECT_NE(e.constraint.text[0], '\0');
    EXPECT_NE(strstr(e.constraint.text, "200 ms"), nullptr);
    EXPECT_STREQ(e.constraint.kind,   "technical");
    EXPECT_STREQ(e.constraint.source, "PERF-POLICY-001");
}

TEST(YamlParseEntityTest, NoIdReturnsError)
{
    const char *path = write_yaml("ent_noid.yaml",
        "title: No id field\n"
        "type: functional\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, -1);
}

TEST(YamlParseEntityTest, NonexistentFile)
{
    Entity e;
    int rc = yaml_parse_entity("/tmp/no_such_entity_xyz.yaml", &e);
    EXPECT_EQ(rc, -1);
}

TEST(YamlParseEntityTest, ExtendedLifecycleFields)
{
    const char *path = write_yaml("ent_lifecycle.yaml",
        "id: REQ-EXT-001\n"
        "title: Extended fields\n"
        "type: functional\n"
        "owner: alice\n"
        "version: 1.2\n"
        "rationale: Needed for compliance.\n"
        "tags:\n"
        "  - safety\n"
        "  - compliance\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(e.lifecycle.owner,   "alice");
    EXPECT_STREQ(e.lifecycle.version, "1.2");
    EXPECT_STREQ(e.text.rationale,    "Needed for compliance.");
    EXPECT_EQ(e.tags.count, 2);
    EXPECT_NE(strstr(e.tags.tags, "safety"),     nullptr);
    EXPECT_NE(strstr(e.tags.tags, "compliance"), nullptr);
}

/* =========================================================================
 * Tests — yaml_parse_entities (multi-document)
 * ======================================================================= */

TEST(YamlParseEntitiesTest, MixedKindsMultiDoc)
{
    const char *path = write_yaml("ent_multi.yaml",
        "id: REQ-001\n"
        "title: A requirement\n"
        "type: functional\n"
        "status: draft\n"
        "---\n"
        "id: STORY-001\n"
        "title: A story\n"
        "type: story\n"
        "status: draft\n"
        "role: developer\n"
        "goal: faster builds\n"
        "reason: I save time\n"
        "---\n"
        "id: ASSUM-001\n"
        "title: An assumption\n"
        "type: assumption\n"
        "assumption:\n"
        "  text: The cloud is reachable.\n"
        "  status: open\n");
    ASSERT_NE(path, nullptr);

    EntityList list;
    entity_list_init(&list);

    int rc = yaml_parse_entities(path, &list);
    EXPECT_EQ(rc, 3);
    EXPECT_EQ(list.count, 3);

    EXPECT_EQ(list.items[0].identity.kind, ENTITY_KIND_REQUIREMENT);
    EXPECT_STREQ(list.items[0].identity.id, "REQ-001");

    EXPECT_EQ(list.items[1].identity.kind, ENTITY_KIND_STORY);
    EXPECT_STREQ(list.items[1].identity.id, "STORY-001");
    EXPECT_STREQ(list.items[1].user_story.role, "developer");

    EXPECT_EQ(list.items[2].identity.kind, ENTITY_KIND_ASSUMPTION);
    EXPECT_STREQ(list.items[2].identity.id, "ASSUM-001");

    entity_list_free(&list);
}

TEST(YamlParseEntitiesTest, SkipsDocumentsWithoutId)
{
    const char *path = write_yaml("ent_multi_noid.yaml",
        "id: REQ-001\n"
        "title: Has id\n"
        "---\n"
        "title: No id here\n"
        "---\n"
        "id: CONSTR-001\n"
        "title: Also has id\n"
        "type: constraint\n");
    ASSERT_NE(path, nullptr);

    EntityList list;
    entity_list_init(&list);

    int rc = yaml_parse_entities(path, &list);
    EXPECT_EQ(rc, 2);
    EXPECT_STREQ(list.items[0].identity.id, "REQ-001");
    EXPECT_STREQ(list.items[1].identity.id, "CONSTR-001");

    entity_list_free(&list);
}

TEST(YamlParseEntitiesTest, DocIndexIsSet)
{
    const char *path = write_yaml("ent_docidx.yaml",
        "id: ENT-A\n"
        "title: First\n"
        "---\n"
        "id: ENT-B\n"
        "title: Second\n");
    ASSERT_NE(path, nullptr);

    EntityList list;
    entity_list_init(&list);

    yaml_parse_entities(path, &list);
    ASSERT_EQ(list.count, 2);
    EXPECT_EQ(list.items[0].identity.doc_index, 0);
    EXPECT_EQ(list.items[1].identity.doc_index, 1);

    entity_list_free(&list);
}

TEST(YamlParseEntitiesTest, NonexistentFile)
{
    EntityList list;
    entity_list_init(&list);

    int rc = yaml_parse_entities("/tmp/no_such_file_xyz.yaml", &list);
    EXPECT_EQ(rc, -1);
    EXPECT_EQ(list.count, 0);

    entity_list_free(&list);
}

/* =========================================================================
 * Tests — TraceabilityComponent (yaml_parse_entity + entity_traceability_to_triplets)
 * ======================================================================= */

TEST(YamlParseEntityTest, TraceabilityComponent)
{
    /* Any entity can carry a traceability component. */
    const char *path = write_yaml("ent_traceability.yaml",
        "id: REQ-SW-001\n"
        "title: A traced requirement\n"
        "type: functional\n"
        "traceability:\n"
        "  - id: REQ-SYS-005\n"
        "    relation: derived-from\n"
        "  - id: TC-SW-001\n"
        "    relation: verified-by\n"
        "  - artefact: src/auth/login.c\n"
        "    relation: implemented-in\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.traceability.count, 3);
    EXPECT_NE(strstr(e.traceability.entries, "REQ-SYS-005"),    nullptr);
    EXPECT_NE(strstr(e.traceability.entries, "derived-from"),   nullptr);
    EXPECT_NE(strstr(e.traceability.entries, "TC-SW-001"),      nullptr);
    EXPECT_NE(strstr(e.traceability.entries, "verified-by"),    nullptr);
    EXPECT_NE(strstr(e.traceability.entries, "src/auth/login.c"), nullptr);
    EXPECT_NE(strstr(e.traceability.entries, "implemented-in"), nullptr);
}

TEST(YamlParseEntityTest, TraceabilityOnAnyEntityKind)
{
    /* Traceability component can be attached to any entity kind. */
    const char *path = write_yaml("ent_story_trace.yaml",
        "id: STORY-010\n"
        "title: Story with traceability\n"
        "type: story\n"
        "role: developer\n"
        "goal: submit code\n"
        "reason: features are delivered\n"
        "traceability:\n"
        "  - id: REQ-SW-010\n"
        "    relation: implements\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.kind, ENTITY_KIND_STORY);
    EXPECT_EQ(e.traceability.count, 1);
    EXPECT_NE(strstr(e.traceability.entries, "REQ-SW-010"), nullptr);
    EXPECT_NE(strstr(e.traceability.entries, "implements"), nullptr);
}

TEST(YamlParseEntityTest, TraceabilityEmptyWhenAbsent)
{
    /* Entities without a traceability key have a zero-initialised component. */
    const char *path = write_yaml("ent_no_trace.yaml",
        "id: REQ-003\n"
        "title: No traceability\n"
        "type: functional\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.traceability.count, 0);
    EXPECT_EQ(e.traceability.entries[0], '\0');
}

TEST(YamlParseEntityTest, SourcesComponentMappingItems)
{
    /* Any entity can carry a sources component with mapping items. */
    const char *path = write_yaml("ent_sources_mapping.yaml",
        "id: REQ-SW-001\n"
        "title: User authentication\n"
        "type: functional\n"
        "sources:\n"
        "  - external: EU-2016-679:article:32\n"
        "  - id: REQ-SYS-005\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.sources.count, 2);
    EXPECT_NE(strstr(e.sources.sources, "EU-2016-679:article:32"), nullptr);
    EXPECT_NE(strstr(e.sources.sources, "REQ-SYS-005"),            nullptr);
}

TEST(YamlParseEntityTest, SourcesComponentScalarItems)
{
    /* Sources component accepts plain scalar items too. */
    const char *path = write_yaml("ent_sources_scalar.yaml",
        "id: REQ-SW-002\n"
        "title: Session timeout\n"
        "type: functional\n"
        "sources:\n"
        "  - EN-ISO-13849-2023:clause:4.5.2\n"
        "  - REQ-SYS-001\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.sources.count, 2);
    EXPECT_NE(strstr(e.sources.sources, "EN-ISO-13849-2023:clause:4.5.2"), nullptr);
    EXPECT_NE(strstr(e.sources.sources, "REQ-SYS-001"),                    nullptr);
}

TEST(YamlParseEntityTest, SourcesEmptyWhenAbsent)
{
    /* Entities without a sources key have a zero-initialised component. */
    const char *path = write_yaml("ent_no_sources.yaml",
        "id: REQ-004\n"
        "title: No sources\n"
        "type: functional\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.sources.count, 0);
    EXPECT_EQ(e.sources.sources[0], '\0');
}

/* =========================================================================
 * Tests — entity_traceability_to_triplets
 * ======================================================================= */

TEST(TraceabilityToTripletsTest, LoadsEntriesIntoStore)
{
    const char *path = write_yaml("ent_trace_triplets.yaml",
        "id: REQ-SW-020\n"
        "title: Requirement with traceability\n"
        "type: functional\n"
        "traceability:\n"
        "  - id: REQ-SYS-010\n"
        "    relation: derived-from\n"
        "  - id: TC-SW-020\n"
        "    relation: verified-by\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(e.traceability.count, 2);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    int added = entity_traceability_to_triplets(&e, store);
    EXPECT_EQ(added, 2);
    EXPECT_EQ(triplet_store_count(store), 2u);

    /* Verify the triples are queryable by subject. */
    CTripleList list = triplet_store_find_by_subject(store, "REQ-SW-020");
    ASSERT_EQ(list.count, 2u);
    triplet_store_list_free(list);

    triplet_store_destroy(store);
}

TEST(TraceabilityToTripletsTest, NullInputsReturnMinusOne)
{
    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_traceability_to_triplets(nullptr, store), -1);
    EXPECT_EQ(entity_traceability_to_triplets(&e, nullptr),    -1);

    triplet_store_destroy(store);
}

TEST(TraceabilityToTripletsTest, EmptyTraceabilityReturnsZero)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    strncpy(e.identity.id, "ENT-001", sizeof(e.identity.id) - 1);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    EXPECT_EQ(entity_traceability_to_triplets(&e, store), 0);
    EXPECT_EQ(triplet_store_count(store), 0u);

    triplet_store_destroy(store);
}

TEST(TraceabilityToTripletsTest, DuplicatesNotAdded)
{
    const char *path = write_yaml("ent_trace_dedup.yaml",
        "id: REQ-SW-030\n"
        "title: Dedup test\n"
        "type: functional\n"
        "traceability:\n"
        "  - id: REQ-SYS-030\n"
        "    relation: derived-from\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    ASSERT_EQ(yaml_parse_entity(path, &e), 0);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    EXPECT_EQ(entity_traceability_to_triplets(&e, store), 1);
    /* Loading again must add 0 new triples (TripletStore deduplicates). */
    EXPECT_EQ(entity_traceability_to_triplets(&e, store), 0);
    EXPECT_EQ(triplet_store_count(store), 1u);

    triplet_store_destroy(store);
}

/* =========================================================================
 * Tests — entity_doc_membership_to_triplets
 * ======================================================================= */

TEST(DocMembershipToTripletsTest, LoadsMembershipsAsPartOfTriples)
{
    const char *path = write_yaml("ent_doc_member_triplets.yaml",
        "id: REQ-SW-100\n"
        "title: Requirement in two documents\n"
        "type: functional\n"
        "documents:\n"
        "  - SRS-CLIENT-001\n"
        "  - SDD-SYS-001\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(e.doc_membership.count, 2);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    int added = entity_doc_membership_to_triplets(&e, store);
    EXPECT_EQ(added, 2);
    EXPECT_EQ(triplet_store_count(store), 2u);

    /* Both triples must use "part-of" as the predicate. */
    CTripleList list = triplet_store_find_by_subject(store, "REQ-SW-100");
    ASSERT_EQ(list.count, 2u);
    int part_of_count = 0;
    for (size_t i = 0; i < list.count; i++) {
        if (strcmp(list.triples[i].predicate, "part-of") == 0)
            part_of_count++;
    }
    EXPECT_EQ(part_of_count, 2);
    triplet_store_list_free(list);

    /* The objects must be the two document IDs. */
    CTripleList by_obj1 = triplet_store_find_by_object(store, "SRS-CLIENT-001");
    EXPECT_GE(by_obj1.count, 1u);
    triplet_store_list_free(by_obj1);

    CTripleList by_obj2 = triplet_store_find_by_object(store, "SDD-SYS-001");
    EXPECT_GE(by_obj2.count, 1u);
    triplet_store_list_free(by_obj2);

    triplet_store_destroy(store);
}

TEST(DocMembershipToTripletsTest, NullInputsReturnMinusOne)
{
    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_doc_membership_to_triplets(nullptr, store), -1);
    EXPECT_EQ(entity_doc_membership_to_triplets(&e, nullptr),    -1);

    triplet_store_destroy(store);
}

TEST(DocMembershipToTripletsTest, EmptyMembershipReturnsZero)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    strncpy(e.identity.id, "REQ-SW-200", sizeof(e.identity.id) - 1);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    EXPECT_EQ(entity_doc_membership_to_triplets(&e, store), 0);
    EXPECT_EQ(triplet_store_count(store), 0u);

    triplet_store_destroy(store);
}

TEST(DocMembershipToTripletsTest, DuplicatesNotAdded)
{
    const char *path = write_yaml("ent_doc_member_dedup.yaml",
        "id: REQ-SW-300\n"
        "title: Dedup doc membership test\n"
        "type: functional\n"
        "documents:\n"
        "  - SRS-DEDUP-001\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    ASSERT_EQ(yaml_parse_entity(path, &e), 0);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    EXPECT_EQ(entity_doc_membership_to_triplets(&e, store), 1);
    /* Loading again must add 0 new triples (TripletStore deduplicates). */
    EXPECT_EQ(entity_doc_membership_to_triplets(&e, store), 0);
    EXPECT_EQ(triplet_store_count(store), 1u);

    triplet_store_destroy(store);
}

TEST(DocMembershipToTripletsTest, InverseContainsIsInferred)
{
    const char *path = write_yaml("ent_doc_member_inverse.yaml",
        "id: REQ-SW-400\n"
        "title: Requirement for inverse test\n"
        "type: functional\n"
        "documents:\n"
        "  - SRS-INV-001\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    ASSERT_EQ(yaml_parse_entity(path, &e), 0);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    ASSERT_EQ(entity_doc_membership_to_triplets(&e, store), 1);
    triplet_store_infer_inverses(store);

    /* The inverse (SRS-INV-001, contains, REQ-SW-400) must be inferred. */
    CTripleList by_doc = triplet_store_find_by_subject(store, "SRS-INV-001");
    int contains_count = 0;
    for (size_t i = 0; i < by_doc.count; i++) {
        if (by_doc.triples[i].inferred &&
            strcmp(by_doc.triples[i].predicate, "contains") == 0 &&
            strcmp(by_doc.triples[i].object, "REQ-SW-400") == 0)
            contains_count++;
    }
    EXPECT_EQ(contains_count, 1);
    triplet_store_list_free(by_doc);

    triplet_store_destroy(store);
}

/* =========================================================================
 * Tests — ENTITY_KIND_DOCUMENT / entity_kind_from_string
 * ======================================================================= */

TEST(EntityKindTest, Document)
{
    EXPECT_EQ(entity_kind_from_string("document"), ENTITY_KIND_DOCUMENT);
    EXPECT_EQ(entity_kind_from_string("srs"),      ENTITY_KIND_DOCUMENT);
    EXPECT_EQ(entity_kind_from_string("sdd"),      ENTITY_KIND_DOCUMENT);
}

TEST(EntityKindTest, DocumentLabel)
{
    EXPECT_STREQ(entity_kind_label(ENTITY_KIND_DOCUMENT), "document");
}

/* =========================================================================
 * Tests — DocumentMetaComponent (yaml_parse_entity)
 * ======================================================================= */

TEST(YamlParseEntityTest, DocumentMetaFile)
{
    /* A document entity carries the doc_meta component. */
    const char *path = write_yaml("ent_doc_meta.yaml",
        "id: SRS-CLIENT-001\n"
        "title: SRS for Client Project\n"
        "type: document\n"
        "doc_meta:\n"
        "  doc_type: SRS\n"
        "  version: 1.2\n"
        "  client: ClientCorp\n"
        "  status: approved\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(e.identity.id,        "SRS-CLIENT-001");
    EXPECT_EQ(e.identity.kind,         ENTITY_KIND_DOCUMENT);
    EXPECT_STREQ(e.doc_meta.doc_type,  "SRS");
    EXPECT_STREQ(e.doc_meta.version,   "1.2");
    EXPECT_STREQ(e.doc_meta.client,    "ClientCorp");
    EXPECT_STREQ(e.doc_meta.status,    "approved");
    /* Membership component must be zero when not set. */
    EXPECT_EQ(e.doc_membership.count,  0);
    EXPECT_EQ(e.doc_membership.doc_ids[0], '\0');
}

TEST(YamlParseEntityTest, DocumentMetaWithOptionalTitle)
{
    /* doc_meta may carry its own title field. */
    const char *path = write_yaml("ent_doc_meta_title.yaml",
        "id: SDD-SYS-001\n"
        "title: SDD for System\n"
        "type: sdd\n"
        "doc_meta:\n"
        "  title: Software Design Description v2\n"
        "  doc_type: SDD\n"
        "  version: 2.0\n"
        "  client: AcmeCorp\n"
        "  status: draft\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.kind,         ENTITY_KIND_DOCUMENT);
    EXPECT_STREQ(e.doc_meta.title,     "Software Design Description v2");
    EXPECT_STREQ(e.doc_meta.doc_type,  "SDD");
    EXPECT_STREQ(e.doc_meta.version,   "2.0");
    EXPECT_STREQ(e.doc_meta.client,    "AcmeCorp");
    EXPECT_STREQ(e.doc_meta.status,    "draft");
}

TEST(YamlParseEntityTest, DocumentMetaEmptyWhenAbsent)
{
    /* Entities without a doc_meta key have a zero-initialised component. */
    const char *path = write_yaml("ent_no_doc_meta.yaml",
        "id: REQ-010\n"
        "title: Plain requirement\n"
        "type: functional\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.doc_meta.doc_type[0], '\0');
    EXPECT_EQ(e.doc_meta.client[0],   '\0');
}

/* =========================================================================
 * Tests — DocumentMembershipComponent (yaml_parse_entity)
 * ======================================================================= */

TEST(YamlParseEntityTest, DocumentMembership)
{
    /* Any entity can belong to one or more documents. */
    const char *path = write_yaml("ent_doc_member.yaml",
        "id: REQ-SW-100\n"
        "title: Requirement in two documents\n"
        "type: functional\n"
        "documents:\n"
        "  - SRS-CLIENT-001\n"
        "  - SDD-SYS-001\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.doc_membership.count, 2);
    EXPECT_NE(strstr(e.doc_membership.doc_ids, "SRS-CLIENT-001"), nullptr);
    EXPECT_NE(strstr(e.doc_membership.doc_ids, "SDD-SYS-001"),    nullptr);
}

TEST(YamlParseEntityTest, DocumentMembershipSingleDoc)
{
    const char *path = write_yaml("ent_doc_member_single.yaml",
        "id: STORY-050\n"
        "title: Story in one document\n"
        "type: story\n"
        "role: developer\n"
        "goal: ship feature\n"
        "reason: the release goes out\n"
        "documents:\n"
        "  - SRS-MAIN-001\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.doc_membership.count, 1);
    EXPECT_NE(strstr(e.doc_membership.doc_ids, "SRS-MAIN-001"), nullptr);
}

TEST(YamlParseEntityTest, DocumentMembershipEmptyWhenAbsent)
{
    const char *path = write_yaml("ent_no_doc_member.yaml",
        "id: REQ-011\n"
        "title: Unattached requirement\n"
        "type: functional\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.doc_membership.count,      0);
    EXPECT_EQ(e.doc_membership.doc_ids[0], '\0');
}

TEST(YamlParseEntityTest, AnyEntityCanCarryDocumentComponents)
{
    /* A requirement entity carrying both doc_meta and doc_membership. */
    const char *path = write_yaml("ent_req_doc_components.yaml",
        "id: REQ-012\n"
        "title: Requirement with document components\n"
        "type: functional\n"
        "doc_meta:\n"
        "  doc_type: SRS\n"
        "  version: 3.0\n"
        "  client: MegaCorp\n"
        "  status: draft\n"
        "documents:\n"
        "  - SRS-MEGA-001\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.kind,          ENTITY_KIND_REQUIREMENT);
    EXPECT_STREQ(e.doc_meta.doc_type,   "SRS");
    EXPECT_STREQ(e.doc_meta.version,    "3.0");
    EXPECT_STREQ(e.doc_meta.client,     "MegaCorp");
    EXPECT_EQ(e.doc_membership.count,   1);
    EXPECT_NE(strstr(e.doc_membership.doc_ids, "SRS-MEGA-001"), nullptr);
}

/* =========================================================================
 * Tests — DocumentBodyComponent (yaml_parse_entity)
 * ======================================================================= */

TEST(YamlParseEntityTest, DocumentBodyShortContent)
{
    /* A design-note entity with a short body field. */
    const char *path = write_yaml("ent_doc_body_short.yaml",
        "id: DN-001\n"
        "title: Short design note\n"
        "type: design-note\n"
        "body: This is a short body text.\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(e.identity.id,    "DN-001");
    EXPECT_EQ(e.identity.kind,     ENTITY_KIND_DESIGN_NOTE);
    ASSERT_NE(e.doc_body.body,     nullptr);
    EXPECT_STREQ(e.doc_body.body,  "This is a short body text.");
    EXPECT_EQ(entity_has_component(&e, "doc-body"), 1);
    EXPECT_EQ(entity_has_component(&e, "body"),     1);
    entity_free(&e);
}

TEST(YamlParseEntityTest, DocumentBodyLargeContent)
{
    /* Build a body string larger than the old 4096-byte limit. */
    std::string large_body(8192, 'A');  /* 8 KB — exceeds old 4096-byte cap */
    std::string yaml_content =
        "id: DN-LARGE-001\n"
        "title: Large design note\n"
        "type: design-note\n"
        "body: " + large_body + "\n";

    const char *path = write_yaml("ent_doc_body_large.yaml",
                                   yaml_content.c_str());
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(e.identity.id, "DN-LARGE-001");
    EXPECT_EQ(e.identity.kind,  ENTITY_KIND_DESIGN_NOTE);
    /* Body must be stored in full — length matches the generated string. */
    ASSERT_NE(e.doc_body.body, nullptr);
    EXPECT_EQ((int)strlen(e.doc_body.body), 8192);
    EXPECT_EQ(entity_has_component(&e, "doc-body"), 1);
    entity_free(&e);
}

TEST(YamlParseEntityTest, DocumentBodyNearMaxContent)
{
    /* Build a body string near the new DOCBODY_LEN (65536) limit. */
    std::string large_body(60000, 'B');  /* ~60 KB — near the 64 KB cap */
    std::string yaml_content =
        "id: DN-NEARMAX-001\n"
        "title: Near-max design note\n"
        "type: design-note\n"
        "body: " + large_body + "\n";

    const char *path = write_yaml("ent_doc_body_nearmax.yaml",
                                   yaml_content.c_str());
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(e.identity.id, "DN-NEARMAX-001");
    EXPECT_EQ(e.identity.kind,  ENTITY_KIND_DESIGN_NOTE);
    /* Body must be stored in full — length matches the generated string. */
    ASSERT_NE(e.doc_body.body, nullptr);
    EXPECT_EQ((int)strlen(e.doc_body.body), 60000);
    EXPECT_EQ(entity_has_component(&e, "doc-body"), 1);
    entity_free(&e);
}

TEST(YamlParseEntityTest, DocumentBodyNullWhenAbsent)
{
    /* Entities without a body key have a NULL doc_body.body pointer. */
    const char *path = write_yaml("ent_no_doc_body.yaml",
        "id: REQ-013\n"
        "title: Requirement without body\n"
        "type: functional\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.doc_body.body, nullptr);
    EXPECT_EQ(entity_has_component(&e, "doc-body"), 0);
    entity_free(&e);
}

/* =========================================================================
 * Tests — entity_has_component
 * ======================================================================= */

TEST(EntityHasComponentTest, NullAndEmptyCompAlwaysMatch)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, nullptr), 1);
    EXPECT_EQ(entity_has_component(&e, ""),      1);
}

TEST(EntityHasComponentTest, UserStoryAbsentAndPresent)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, "user-story"),  0);
    EXPECT_EQ(entity_has_component(&e, "user_story"),  0);

    strncpy(e.user_story.role, "developer", sizeof(e.user_story.role) - 1);
    EXPECT_EQ(entity_has_component(&e, "user-story"),  1);
    EXPECT_EQ(entity_has_component(&e, "user_story"),  1);
}

TEST(EntityHasComponentTest, AcceptanceCriteriaAbsentAndPresent)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, "acceptance-criteria"), 0);
    EXPECT_EQ(entity_has_component(&e, "acceptance_criteria"), 0);

    e.acceptance_criteria.count = 1;
    EXPECT_EQ(entity_has_component(&e, "acceptance-criteria"), 1);
    EXPECT_EQ(entity_has_component(&e, "acceptance_criteria"), 1);
}

TEST(EntityHasComponentTest, EpicMembershipAbsentAndPresent)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, "epic"),            0);
    EXPECT_EQ(entity_has_component(&e, "epic-membership"), 0);
    EXPECT_EQ(entity_has_component(&e, "epic_membership"), 0);

    strncpy(e.epic_membership.epic_id, "EPIC-001",
            sizeof(e.epic_membership.epic_id) - 1);
    EXPECT_EQ(entity_has_component(&e, "epic"),            1);
    EXPECT_EQ(entity_has_component(&e, "epic-membership"), 1);
    EXPECT_EQ(entity_has_component(&e, "epic_membership"), 1);
}

TEST(EntityHasComponentTest, AssumptionAbsentAndPresent)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, "assumption"), 0);

    strncpy(e.assumption.text, "Some assumption",
            sizeof(e.assumption.text) - 1);
    EXPECT_EQ(entity_has_component(&e, "assumption"), 1);
}

TEST(EntityHasComponentTest, ConstraintAbsentAndPresent)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, "constraint"), 0);

    strncpy(e.constraint.text, "Some constraint",
            sizeof(e.constraint.text) - 1);
    EXPECT_EQ(entity_has_component(&e, "constraint"), 1);
}

TEST(EntityHasComponentTest, DocMetaAbsentAndPresent)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, "doc-meta"), 0);
    EXPECT_EQ(entity_has_component(&e, "doc_meta"), 0);

    strncpy(e.doc_meta.doc_type, "SRS", sizeof(e.doc_meta.doc_type) - 1);
    EXPECT_EQ(entity_has_component(&e, "doc-meta"), 1);
    EXPECT_EQ(entity_has_component(&e, "doc_meta"), 1);
}

TEST(EntityHasComponentTest, DocMembershipAbsentAndPresent)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, "doc-membership"), 0);
    EXPECT_EQ(entity_has_component(&e, "documents"),      0);

    e.doc_membership.count = 1;
    EXPECT_EQ(entity_has_component(&e, "doc-membership"), 1);
    EXPECT_EQ(entity_has_component(&e, "documents"),      1);
}

TEST(EntityHasComponentTest, DocBodyAbsentAndPresent)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, "doc-body"), 0);
    EXPECT_EQ(entity_has_component(&e, "body"),     0);

    e.doc_body.body = strdup("Some body text");
    ASSERT_NE(e.doc_body.body, nullptr);
    EXPECT_EQ(entity_has_component(&e, "doc-body"), 1);
    EXPECT_EQ(entity_has_component(&e, "body"),     1);
    entity_free(&e);
}

TEST(EntityHasComponentTest, TraceabilityAbsentAndPresent)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, "traceability"), 0);

    e.traceability.count = 1;
    EXPECT_EQ(entity_has_component(&e, "traceability"), 1);
}

TEST(EntityHasComponentTest, SourcesAbsentAndPresent)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, "sources"), 0);

    e.sources.count = 1;
    EXPECT_EQ(entity_has_component(&e, "sources"), 1);
}

TEST(EntityHasComponentTest, TagsAbsentAndPresent)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, "tags"), 0);

    e.tags.count = 2;
    EXPECT_EQ(entity_has_component(&e, "tags"), 1);
}

TEST(EntityHasComponentTest, UnknownComponentNameReturnsFalse)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    /* Fill everything so we can confirm unknown names still return 0. */
    strncpy(e.assumption.text, "x", sizeof(e.assumption.text) - 1);
    EXPECT_EQ(entity_has_component(&e, "no-such-component"), 0);
}

TEST(EntityHasComponentTest, TestProcedureAbsentAndPresent)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, "test-procedure"), 0);
    EXPECT_EQ(entity_has_component(&e, "test_procedure"), 0);

    e.test_procedure.step_count = 1;
    EXPECT_EQ(entity_has_component(&e, "test-procedure"), 1);
    EXPECT_EQ(entity_has_component(&e, "test_procedure"), 1);
}

TEST(EntityHasComponentTest, ClauseCollectionAbsentAndPresent)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, "clause-collection"), 0);
    EXPECT_EQ(entity_has_component(&e, "clause_collection"), 0);
    EXPECT_EQ(entity_has_component(&e, "clauses"),           0);

    e.clause_collection.count = 2;
    EXPECT_EQ(entity_has_component(&e, "clause-collection"), 1);
    EXPECT_EQ(entity_has_component(&e, "clause_collection"), 1);
    EXPECT_EQ(entity_has_component(&e, "clauses"),           1);
}

TEST(EntityHasComponentTest, AttachmentAbsentAndPresent)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, "attachment"),  0);
    EXPECT_EQ(entity_has_component(&e, "attachments"), 0);

    e.attachment.count = 1;
    EXPECT_EQ(entity_has_component(&e, "attachment"),  1);
    EXPECT_EQ(entity_has_component(&e, "attachments"), 1);
}

/* =========================================================================
 * Tests — TestProcedureComponent (yaml_parse_entity)
 * ======================================================================= */

TEST(YamlParseEntityTest, TestProcedureFullParse)
{
    /* A test-case entity with all TestProcedureComponent fields present. */
    const char *path = write_yaml("ent_test_procedure.yaml",
        "id: TC-SW-002\n"
        "title: Login with valid credentials\n"
        "type: test-case\n"
        "status: approved\n"
        "preconditions:\n"
        "  - A registered user account exists.\n"
        "  - The endpoint is reachable.\n"
        "steps:\n"
        "  - step: 1\n"
        "    action: Submit login request.\n"
        "    expected_output: System returns HTTP 200.\n"
        "  - step: 2\n"
        "    action: Access protected resource.\n"
        "    expected_output: Resource content is returned.\n"
        "expected_result: User gains access to the protected resource.\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.kind,                      ENTITY_KIND_TEST_CASE);
    EXPECT_STREQ(e.identity.id,                     "TC-SW-002");
    EXPECT_EQ(e.test_procedure.precondition_count,  2);
    ASSERT_NE(e.test_procedure.preconditions,       nullptr);
    EXPECT_NE(strstr(e.test_procedure.preconditions, "A registered user account exists."), nullptr);
    EXPECT_NE(strstr(e.test_procedure.preconditions, "The endpoint is reachable."),        nullptr);
    EXPECT_EQ(e.test_procedure.step_count,          2);
    ASSERT_NE(e.test_procedure.steps,               nullptr);
    EXPECT_NE(strstr(e.test_procedure.steps, "Submit login request."),         nullptr);
    EXPECT_NE(strstr(e.test_procedure.steps, "System returns HTTP 200."),      nullptr);
    EXPECT_NE(strstr(e.test_procedure.steps, "Access protected resource."),    nullptr);
    EXPECT_NE(strstr(e.test_procedure.steps, "Resource content is returned."), nullptr);
    ASSERT_NE(e.test_procedure.expected_result,     nullptr);
    EXPECT_STREQ(e.test_procedure.expected_result,
                 "User gains access to the protected resource.");
    EXPECT_EQ(entity_has_component(&e, "test-procedure"), 1);
    EXPECT_EQ(entity_has_component(&e, "test_procedure"), 1);
    entity_free(&e);
}

TEST(YamlParseEntityTest, TestProcedurePreconditionsOnly)
{
    /* A test-case entity with preconditions but no steps or expected_result. */
    const char *path = write_yaml("ent_test_precond_only.yaml",
        "id: TC-SW-003\n"
        "title: Preconditions only\n"
        "type: test-case\n"
        "preconditions:\n"
        "  - System is running.\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.test_procedure.precondition_count, 1);
    ASSERT_NE(e.test_procedure.preconditions,      nullptr);
    EXPECT_NE(strstr(e.test_procedure.preconditions, "System is running."), nullptr);
    EXPECT_EQ(e.test_procedure.step_count,         0);
    EXPECT_EQ(e.test_procedure.expected_result,    nullptr);
    EXPECT_EQ(entity_has_component(&e, "test-procedure"), 1);
    entity_free(&e);
}

TEST(YamlParseEntityTest, TestProcedureAbsentWhenMissing)
{
    /* An entity without any TestProcedureComponent fields. */
    const char *path = write_yaml("ent_no_test_proc.yaml",
        "id: REQ-014\n"
        "title: Requirement without test procedure\n"
        "type: functional\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.test_procedure.precondition_count,   0);
    EXPECT_EQ(e.test_procedure.step_count,           0);
    EXPECT_EQ(e.test_procedure.preconditions,        nullptr);
    EXPECT_EQ(e.test_procedure.steps,                nullptr);
    EXPECT_EQ(e.test_procedure.expected_result,      nullptr);
    EXPECT_EQ(entity_has_component(&e, "test-procedure"), 0);
    entity_free(&e);
}

TEST(YamlParseEntityTest, TestProcedureOnNonTestCaseEntity)
{
    /* Any entity kind can carry TestProcedureComponent. */
    const char *path = write_yaml("ent_req_test_proc.yaml",
        "id: REQ-015\n"
        "title: Requirement with embedded test procedure\n"
        "type: functional\n"
        "expected_result: System behaves correctly.\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.kind, ENTITY_KIND_REQUIREMENT);
    ASSERT_NE(e.test_procedure.expected_result, nullptr);
    EXPECT_STREQ(e.test_procedure.expected_result, "System behaves correctly.");
    EXPECT_EQ(entity_has_component(&e, "test-procedure"), 1);
    entity_free(&e);
}

/* =========================================================================
 * Tests — ClauseCollectionComponent (yaml_parse_entity)
 * ======================================================================= */

TEST(YamlParseEntityTest, ClauseCollectionFullParse)
{
    /* An external entity with a clauses sequence. */
    const char *path = write_yaml("ent_clause_collection.yaml",
        "id: EXT-MACH-DIR\n"
        "title: EU Machinery Directive 2006/42/EC\n"
        "type: directive\n"
        "clauses:\n"
        "  - id: annex-I-1.1.2\n"
        "    title: Principles of safety integration\n"
        "  - id: annex-I-1.2.1\n"
        "    title: Safety and reliability of control systems\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.kind,           ENTITY_KIND_EXTERNAL);
    EXPECT_STREQ(e.identity.id,          "EXT-MACH-DIR");
    EXPECT_EQ(e.clause_collection.count, 2);
    ASSERT_NE(e.clause_collection.clauses, nullptr);
    EXPECT_NE(strstr(e.clause_collection.clauses, "annex-I-1.1.2"),                  nullptr);
    EXPECT_NE(strstr(e.clause_collection.clauses, "Principles of safety integration"), nullptr);
    EXPECT_NE(strstr(e.clause_collection.clauses, "annex-I-1.2.1"),                  nullptr);
    EXPECT_EQ(entity_has_component(&e, "clause-collection"), 1);
    EXPECT_EQ(entity_has_component(&e, "clause_collection"), 1);
    EXPECT_EQ(entity_has_component(&e, "clauses"),           1);
    entity_free(&e);
}

TEST(YamlParseEntityTest, ClauseCollectionIdOnlyClause)
{
    /* A clause with id but no title is stored with empty title. */
    const char *path = write_yaml("ent_clause_id_only.yaml",
        "id: EXT-STD-001\n"
        "title: Some Standard\n"
        "type: standard\n"
        "clauses:\n"
        "  - id: section-4.5\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.clause_collection.count, 1);
    ASSERT_NE(e.clause_collection.clauses, nullptr);
    EXPECT_NE(strstr(e.clause_collection.clauses, "section-4.5"), nullptr);
    EXPECT_EQ(entity_has_component(&e, "clauses"), 1);
    entity_free(&e);
}

TEST(YamlParseEntityTest, ClauseCollectionNullWhenMissing)
{
    /* An entity without clauses has a NULL clause_collection.clauses pointer. */
    const char *path = write_yaml("ent_no_clauses.yaml",
        "id: EXT-002\n"
        "title: External without clauses\n"
        "type: external\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.clause_collection.count,   0);
    EXPECT_EQ(e.clause_collection.clauses, nullptr);
    EXPECT_EQ(entity_has_component(&e, "clause-collection"), 0);
    entity_free(&e);
}

/* =========================================================================
 * Tests — AttachmentComponent (yaml_parse_entity)
 * ======================================================================= */

TEST(YamlParseEntityTest, AttachmentFullParse)
{
    /* An entity with an attachments sequence. */
    const char *path = write_yaml("ent_attachment.yaml",
        "id: SRS-CLIENT-002\n"
        "title: SRS with attachments\n"
        "type: document\n"
        "attachments:\n"
        "  - path: docs/spec.pdf\n"
        "    description: Original specification document\n"
        "  - path: images/diagram.png\n"
        "    description: Architecture overview diagram\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.kind,     ENTITY_KIND_DOCUMENT);
    EXPECT_STREQ(e.identity.id,    "SRS-CLIENT-002");
    EXPECT_EQ(e.attachment.count,  2);
    ASSERT_NE(e.attachment.attachments, nullptr);
    EXPECT_NE(strstr(e.attachment.attachments, "docs/spec.pdf"),                nullptr);
    EXPECT_NE(strstr(e.attachment.attachments, "Original specification document"), nullptr);
    EXPECT_NE(strstr(e.attachment.attachments, "images/diagram.png"),           nullptr);
    EXPECT_NE(strstr(e.attachment.attachments, "Architecture overview diagram"), nullptr);
    EXPECT_EQ(entity_has_component(&e, "attachment"),  1);
    EXPECT_EQ(entity_has_component(&e, "attachments"), 1);
    entity_free(&e);
}

TEST(YamlParseEntityTest, AttachmentPathOnly)
{
    /* An attachment entry with path but no description. */
    const char *path = write_yaml("ent_attach_path_only.yaml",
        "id: TC-ATT-001\n"
        "title: Test case with attachment\n"
        "type: test-case\n"
        "attachments:\n"
        "  - path: reports/test_run.xml\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.attachment.count, 1);
    ASSERT_NE(e.attachment.attachments, nullptr);
    EXPECT_NE(strstr(e.attachment.attachments, "reports/test_run.xml"), nullptr);
    EXPECT_EQ(entity_has_component(&e, "attachment"), 1);
    entity_free(&e);
}

TEST(YamlParseEntityTest, AttachmentNullWhenMissing)
{
    /* An entity without attachments has a NULL attachment.attachments pointer. */
    const char *path = write_yaml("ent_no_attachment.yaml",
        "id: REQ-016\n"
        "title: Requirement without attachments\n"
        "type: functional\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.attachment.count,       0);
    EXPECT_EQ(e.attachment.attachments, nullptr);
    EXPECT_EQ(entity_has_component(&e, "attachment"), 0);
    entity_free(&e);
}

TEST(YamlParseEntityTest, AllThreeNewComponentsOnOneEntity)
{
    /* An external test-case entity carrying TestProcedure, ClauseCollection,
     * and Attachment components simultaneously. */
    const char *path = write_yaml("ent_combined_new.yaml",
        "id: TC-FULL-001\n"
        "title: Fully equipped test case\n"
        "type: test-case\n"
        "preconditions:\n"
        "  - Setup complete.\n"
        "steps:\n"
        "  - step: 1\n"
        "    action: Run the test.\n"
        "    expected_output: Test passes.\n"
        "expected_result: All assertions pass.\n"
        "clauses:\n"
        "  - id: sec-4.1\n"
        "    title: Scope of test\n"
        "attachments:\n"
        "  - path: results/report.html\n"
        "    description: Test execution report\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.kind,                      ENTITY_KIND_TEST_CASE);
    EXPECT_EQ(e.test_procedure.precondition_count,  1);
    EXPECT_EQ(e.test_procedure.step_count,          1);
    ASSERT_NE(e.test_procedure.expected_result,     nullptr);
    EXPECT_STREQ(e.test_procedure.expected_result,  "All assertions pass.");
    EXPECT_EQ(e.clause_collection.count,            1);
    ASSERT_NE(e.clause_collection.clauses,          nullptr);
    EXPECT_NE(strstr(e.clause_collection.clauses, "sec-4.1"), nullptr);
    EXPECT_EQ(e.attachment.count,                   1);
    ASSERT_NE(e.attachment.attachments,             nullptr);
    EXPECT_NE(strstr(e.attachment.attachments, "results/report.html"), nullptr);
    EXPECT_EQ(entity_has_component(&e, "test-procedure"),    1);
    EXPECT_EQ(entity_has_component(&e, "clause-collection"), 1);
    EXPECT_EQ(entity_has_component(&e, "attachment"),        1);
    entity_free(&e);
}

/* =========================================================================
 * Tests — entity_cmp_by_id
 * ======================================================================= */

TEST(EntityCmpByIdTest, SortsTwoEntities)
{
    Entity a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    strncpy(a.identity.id, "REQ-002", sizeof(a.identity.id) - 1);
    strncpy(b.identity.id, "REQ-001", sizeof(b.identity.id) - 1);

    /* a > b alphabetically */
    EXPECT_GT(entity_cmp_by_id(&a, &b), 0);
    EXPECT_LT(entity_cmp_by_id(&b, &a), 0);
    EXPECT_EQ(entity_cmp_by_id(&a, &a), 0);
}

TEST(EntityCmpByIdTest, UsableWithQsort)
{
    EntityList list;
    entity_list_init(&list);

    Entity e;
    memset(&e, 0, sizeof(e));

    strncpy(e.identity.id, "REQ-003", sizeof(e.identity.id) - 1);
    entity_list_add(&list, &e);
    strncpy(e.identity.id, "REQ-001", sizeof(e.identity.id) - 1);
    entity_list_add(&list, &e);
    strncpy(e.identity.id, "REQ-002", sizeof(e.identity.id) - 1);
    entity_list_add(&list, &e);

    qsort(list.items, (size_t)list.count, sizeof(Entity), entity_cmp_by_id);

    EXPECT_STREQ(list.items[0].identity.id, "REQ-001");
    EXPECT_STREQ(list.items[1].identity.id, "REQ-002");
    EXPECT_STREQ(list.items[2].identity.id, "REQ-003");

    entity_list_free(&list);
}

/* =========================================================================
 * Tests — entity_apply_filter
 * ======================================================================= */

static Entity make_test_entity(const char *id, EntityKind kind,
                                const char *status, const char *priority)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    strncpy(e.identity.id, id, sizeof(e.identity.id) - 1);
    e.identity.kind = kind;
    strncpy(e.lifecycle.status,   status,   sizeof(e.lifecycle.status)   - 1);
    strncpy(e.lifecycle.priority, priority, sizeof(e.lifecycle.priority) - 1);
    return e;
}

TEST(EntityApplyFilterTest, NullFiltersPassesAll)
{
    EntityList src, dst;
    entity_list_init(&src);
    entity_list_init(&dst);

    Entity e1 = make_test_entity("REQ-001", ENTITY_KIND_REQUIREMENT,
                                  "draft", "must");
    Entity e2 = make_test_entity("TC-001",  ENTITY_KIND_TEST_CASE,
                                  "approved", "should");
    entity_list_add(&src, &e1);
    entity_list_add(&src, &e2);

    entity_apply_filter(&src, &dst, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(dst.count, 2);

    entity_list_free(&src);
    entity_list_free(&dst);
}

TEST(EntityApplyFilterTest, KindFilterSelectsOnlyRequirements)
{
    EntityList src, dst;
    entity_list_init(&src);
    entity_list_init(&dst);

    Entity e1 = make_test_entity("REQ-001", ENTITY_KIND_REQUIREMENT,
                                  "draft", "must");
    Entity e2 = make_test_entity("TC-001",  ENTITY_KIND_TEST_CASE,
                                  "draft", "must");
    entity_list_add(&src, &e1);
    entity_list_add(&src, &e2);

    entity_apply_filter(&src, &dst, "requirement", nullptr, nullptr, nullptr);
    EXPECT_EQ(dst.count, 1);
    EXPECT_STREQ(dst.items[0].identity.id, "REQ-001");

    entity_list_free(&src);
    entity_list_free(&dst);
}

TEST(EntityApplyFilterTest, StatusFilterSelectsApproved)
{
    EntityList src, dst;
    entity_list_init(&src);
    entity_list_init(&dst);

    Entity e1 = make_test_entity("REQ-001", ENTITY_KIND_REQUIREMENT,
                                  "approved", "must");
    Entity e2 = make_test_entity("REQ-002", ENTITY_KIND_REQUIREMENT,
                                  "draft", "must");
    entity_list_add(&src, &e1);
    entity_list_add(&src, &e2);

    entity_apply_filter(&src, &dst, nullptr, nullptr, "approved", nullptr);
    EXPECT_EQ(dst.count, 1);
    EXPECT_STREQ(dst.items[0].identity.id, "REQ-001");

    entity_list_free(&src);
    entity_list_free(&dst);
}

TEST(EntityApplyFilterTest, PriorityFilterSelectsMust)
{
    EntityList src, dst;
    entity_list_init(&src);
    entity_list_init(&dst);

    Entity e1 = make_test_entity("REQ-001", ENTITY_KIND_REQUIREMENT,
                                  "draft", "must");
    Entity e2 = make_test_entity("REQ-002", ENTITY_KIND_REQUIREMENT,
                                  "draft", "should");
    entity_list_add(&src, &e1);
    entity_list_add(&src, &e2);

    entity_apply_filter(&src, &dst, nullptr, nullptr, nullptr, "must");
    EXPECT_EQ(dst.count, 1);
    EXPECT_STREQ(dst.items[0].identity.id, "REQ-001");

    entity_list_free(&src);
    entity_list_free(&dst);
}

TEST(EntityApplyFilterTest, CombinedKindAndStatusFilters)
{
    EntityList src, dst;
    entity_list_init(&src);
    entity_list_init(&dst);

    Entity e1 = make_test_entity("REQ-001", ENTITY_KIND_REQUIREMENT,
                                  "approved", "must");
    Entity e2 = make_test_entity("REQ-002", ENTITY_KIND_REQUIREMENT,
                                  "draft", "must");
    Entity e3 = make_test_entity("TC-001",  ENTITY_KIND_TEST_CASE,
                                  "approved", "must");
    entity_list_add(&src, &e1);
    entity_list_add(&src, &e2);
    entity_list_add(&src, &e3);

    entity_apply_filter(&src, &dst, "requirement", nullptr, "approved", nullptr);
    EXPECT_EQ(dst.count, 1);
    EXPECT_STREQ(dst.items[0].identity.id, "REQ-001");

    entity_list_free(&src);
    entity_list_free(&dst);
}

TEST(EntityApplyFilterTest, NoMatchGivesEmptyDst)
{
    EntityList src, dst;
    entity_list_init(&src);
    entity_list_init(&dst);

    Entity e1 = make_test_entity("REQ-001", ENTITY_KIND_REQUIREMENT,
                                  "draft", "must");
    entity_list_add(&src, &e1);

    entity_apply_filter(&src, &dst, "test-case", nullptr, nullptr, nullptr);
    EXPECT_EQ(dst.count, 0);

    entity_list_free(&src);
    entity_list_free(&dst);
}

TEST(EntityApplyFilterTest, EmptySourceGivesEmptyDst)
{
    EntityList src, dst;
    entity_list_init(&src);
    entity_list_init(&dst);

    entity_apply_filter(&src, &dst, "requirement", nullptr, nullptr, nullptr);
    EXPECT_EQ(dst.count, 0);

    entity_list_free(&src);
    entity_list_free(&dst);
}

/* =========================================================================
 * Tests — entity_free / entity_copy / memory management
 * ======================================================================= */

TEST(EntityFreeTest, FreeOnZeroInitialisedEntityIsSafe)
{
    /* entity_free() on a fully-zeroed entity must not crash. */
    Entity e;
    memset(&e, 0, sizeof(e));
    entity_free(&e);
    /* All heap pointers must still be NULL after free. */
    EXPECT_EQ(e.doc_body.body,                 nullptr);
    EXPECT_EQ(e.test_procedure.preconditions,  nullptr);
    EXPECT_EQ(e.test_procedure.steps,          nullptr);
    EXPECT_EQ(e.test_procedure.expected_result, nullptr);
    EXPECT_EQ(e.clause_collection.clauses,     nullptr);
    EXPECT_EQ(e.attachment.attachments,        nullptr);
}

TEST(EntityFreeTest, FreeNullEntityIsSafe)
{
    /* entity_free(NULL) must not crash. */
    entity_free(nullptr);
}

TEST(EntityFreeTest, FreeSetsPointersToNull)
{
    /* After entity_free(), heap pointers are set to NULL. */
    Entity e;
    memset(&e, 0, sizeof(e));
    e.doc_body.body                  = strdup("body content");
    e.test_procedure.expected_result = strdup("expected");
    e.test_procedure.preconditions   = (char *)calloc(1, 64);
    e.test_procedure.steps           = (char *)calloc(1, 64);
    e.clause_collection.clauses      = (char *)calloc(1, 64);
    e.attachment.attachments         = (char *)calloc(1, 64);
    ASSERT_NE(e.doc_body.body,                  nullptr);
    ASSERT_NE(e.test_procedure.expected_result, nullptr);
    ASSERT_NE(e.test_procedure.preconditions,   nullptr);
    ASSERT_NE(e.test_procedure.steps,           nullptr);
    ASSERT_NE(e.clause_collection.clauses,      nullptr);
    ASSERT_NE(e.attachment.attachments,         nullptr);

    entity_free(&e);

    EXPECT_EQ(e.doc_body.body,                 nullptr);
    EXPECT_EQ(e.test_procedure.preconditions,  nullptr);
    EXPECT_EQ(e.test_procedure.steps,          nullptr);
    EXPECT_EQ(e.test_procedure.expected_result, nullptr);
    EXPECT_EQ(e.clause_collection.clauses,     nullptr);
    EXPECT_EQ(e.attachment.attachments,        nullptr);
}

TEST(EntityFreeTest, FreeTwiceIsSafe)
{
    /* Calling entity_free() twice on the same entity must not crash
     * (pointers are NULLed on first free). */
    Entity e;
    memset(&e, 0, sizeof(e));
    e.doc_body.body = strdup("hello");
    ASSERT_NE(e.doc_body.body, nullptr);
    entity_free(&e);
    entity_free(&e);  /* second call: all pointers already NULL */
    EXPECT_EQ(e.doc_body.body, nullptr);
}

TEST(EntityCopyTest, CopyZeroEntityIsOk)
{
    Entity src, dst;
    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));
    strncpy(src.identity.id, "REQ-CPY-001", sizeof(src.identity.id) - 1);

    int rc = entity_copy(&dst, &src);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(dst.identity.id, "REQ-CPY-001");
    /* No heap fields were set — all pointers should still be NULL. */
    EXPECT_EQ(dst.doc_body.body, nullptr);
    EXPECT_EQ(dst.test_procedure.expected_result, nullptr);
    EXPECT_EQ(dst.clause_collection.clauses, nullptr);
    EXPECT_EQ(dst.attachment.attachments, nullptr);

    entity_free(&dst);
}

TEST(EntityCopyTest, CopyDeepCopiesHeapFields)
{
    /* Build a source entity with all heap fields populated. */
    Entity src;
    memset(&src, 0, sizeof(src));
    strncpy(src.identity.id, "DN-CPY-001", sizeof(src.identity.id) - 1);
    src.doc_body.body                  = strdup("body text");
    src.test_procedure.expected_result = strdup("result");
    src.test_procedure.preconditions   = (char *)calloc(1, 64);
    src.clause_collection.clauses      = (char *)calloc(1, 64);
    src.attachment.attachments         = (char *)calloc(1, 64);
    ASSERT_NE(src.doc_body.body,                  nullptr);
    ASSERT_NE(src.test_procedure.expected_result, nullptr);
    ASSERT_NE(src.test_procedure.preconditions,   nullptr);
    ASSERT_NE(src.clause_collection.clauses,      nullptr);
    ASSERT_NE(src.attachment.attachments,         nullptr);
    strncpy(src.test_procedure.preconditions, "precond one", 63);
    src.test_procedure.precondition_count = 1;
    strncpy(src.clause_collection.clauses, "cl-1\ttitle one", 63);
    src.clause_collection.count = 1;
    strncpy(src.attachment.attachments, "file.pdf\tdesc", 63);
    src.attachment.count = 1;

    Entity dst;
    memset(&dst, 0, sizeof(dst));
    int rc = entity_copy(&dst, &src);
    EXPECT_EQ(rc, 0);

    /* dst must have its own copies — different pointers. */
    ASSERT_NE(dst.doc_body.body, nullptr);
    EXPECT_NE(dst.doc_body.body, src.doc_body.body);
    EXPECT_STREQ(dst.doc_body.body, "body text");

    ASSERT_NE(dst.test_procedure.expected_result, nullptr);
    EXPECT_NE(dst.test_procedure.expected_result, src.test_procedure.expected_result);
    EXPECT_STREQ(dst.test_procedure.expected_result, "result");

    ASSERT_NE(dst.test_procedure.preconditions, nullptr);
    EXPECT_NE(dst.test_procedure.preconditions, src.test_procedure.preconditions);
    EXPECT_EQ(dst.test_procedure.precondition_count, 1);

    ASSERT_NE(dst.clause_collection.clauses, nullptr);
    EXPECT_NE(dst.clause_collection.clauses, src.clause_collection.clauses);
    EXPECT_EQ(dst.clause_collection.count, 1);

    ASSERT_NE(dst.attachment.attachments, nullptr);
    EXPECT_NE(dst.attachment.attachments, src.attachment.attachments);
    EXPECT_EQ(dst.attachment.count, 1);

    entity_free(&src);
    entity_free(&dst);
}

TEST(EntityCopyTest, MutatingCopyDoesNotAffectSource)
{
    /* Verify true independence: modifying dst's heap buffer does not affect src. */
    Entity src;
    memset(&src, 0, sizeof(src));
    src.doc_body.body = strdup("original");
    ASSERT_NE(src.doc_body.body, nullptr);

    Entity dst;
    memset(&dst, 0, sizeof(dst));
    ASSERT_EQ(entity_copy(&dst, &src), 0);

    /* Overwrite dst's body. */
    free(dst.doc_body.body);
    dst.doc_body.body = strdup("modified");
    ASSERT_NE(dst.doc_body.body, nullptr);

    EXPECT_STREQ(src.doc_body.body, "original");
    EXPECT_STREQ(dst.doc_body.body, "modified");

    entity_free(&src);
    entity_free(&dst);
}

TEST(EntityListAddTest, ListAddDeepCopiesHeapFields)
{
    /* Verify that entity_list_add() performs a deep copy and the source
     * entity can be freed independently after adding. */
    EntityList list;
    entity_list_init(&list);

    Entity src;
    memset(&src, 0, sizeof(src));
    strncpy(src.identity.id, "DN-LIST-001", sizeof(src.identity.id) - 1);
    src.doc_body.body = strdup("list body");
    ASSERT_NE(src.doc_body.body, nullptr);

    EXPECT_EQ(entity_list_add(&list, &src), 0);
    EXPECT_EQ(list.count, 1);

    /* Free the source — the list copy must still be valid. */
    entity_free(&src);
    EXPECT_EQ(src.doc_body.body, nullptr);

    /* List item must still have its own copy. */
    ASSERT_NE(list.items[0].doc_body.body, nullptr);
    EXPECT_STREQ(list.items[0].doc_body.body, "list body");

    entity_list_free(&list);
}

TEST(EntityListFreeTest, FreeReleasesAllEntityHeapFields)
{
    /* Build a list from yaml_parse_entities with a body field, then free. */
    const char *yaml =
        "id: DN-MEM-001\n"
        "title: Design note for memory test\n"
        "type: design-note\n"
        "body: heap allocated body text\n"
        "---\n"
        "id: TC-MEM-001\n"
        "title: Test case for memory test\n"
        "type: test-case\n"
        "preconditions:\n"
        "  - System is ready.\n"
        "expected_result: Test passes.\n";

    /* Write to tmp */
    FILE *f = fopen("/tmp/mem_test.yaml", "w");
    ASSERT_NE(f, nullptr);
    fputs(yaml, f);
    fclose(f);

    EntityList list;
    entity_list_init(&list);
    int added = yaml_parse_entities("/tmp/mem_test.yaml", &list);
    EXPECT_EQ(added, 2);
    EXPECT_EQ(list.count, 2);

    /* Verify heap fields were set. */
    EXPECT_NE(list.items[0].doc_body.body, nullptr);
    EXPECT_NE(list.items[1].test_procedure.preconditions, nullptr);
    EXPECT_NE(list.items[1].test_procedure.expected_result, nullptr);

    /* This must free all heap fields without leaking. */
    entity_list_free(&list);
    EXPECT_EQ(list.items,    nullptr);
    EXPECT_EQ(list.count,    0);
    EXPECT_EQ(list.capacity, 0);
}

TEST(EntityMemoryTest, ParseEntityBodyIsHeapAllocated)
{
    /* Regression: doc_body.body is heap-allocated and the caller must
     * call entity_free() to release it. */
    const char *path = write_yaml("mem_parse_body.yaml",
        "id: DN-HEAP-001\n"
        "title: Heap body\n"
        "type: design-note\n"
        "body: some content\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    ASSERT_NE(e.doc_body.body, nullptr);
    EXPECT_STREQ(e.doc_body.body, "some content");

    entity_free(&e);
    EXPECT_EQ(e.doc_body.body, nullptr);
}

TEST(EntityMemoryTest, ParsedEntityHeapFieldsAreIndependentFromList)
{
    /* Parsing the same file twice and adding to the same list must not
     * corrupt either entry (each list slot has its own deep copy). */
    const char *path = write_yaml("mem_two_copies.yaml",
        "id: REQ-DUP-001\n"
        "title: Duplicate test\n"
        "type: design-note\n"
        "body: shared body\n");
    ASSERT_NE(path, nullptr);

    EntityList list;
    entity_list_init(&list);

    /* Add the same entity twice via two separate parses. */
    Entity e1, e2;
    ASSERT_EQ(yaml_parse_entity(path, &e1), 0);
    ASSERT_EQ(yaml_parse_entity(path, &e2), 0);
    EXPECT_EQ(entity_list_add(&list, &e1), 0);
    EXPECT_EQ(entity_list_add(&list, &e2), 0);
    entity_free(&e1);
    entity_free(&e2);

    EXPECT_EQ(list.count, 2);
    /* Both items have independent heap pointers. */
    ASSERT_NE(list.items[0].doc_body.body, nullptr);
    ASSERT_NE(list.items[1].doc_body.body, nullptr);
    EXPECT_NE(list.items[0].doc_body.body, list.items[1].doc_body.body);
    EXPECT_STREQ(list.items[0].doc_body.body, "shared body");
    EXPECT_STREQ(list.items[1].doc_body.body, "shared body");

    entity_list_free(&list);
}
