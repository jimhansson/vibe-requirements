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

#include "entity.h"
#include "yaml_simple.h"

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
    EXPECT_EQ((int)list.size(),    0);
    EXPECT_EQ(list.capacity, 0);
    EXPECT_EQ(list.items,    nullptr);
    EXPECT_EQ((int)list.size(),    0);
    EXPECT_EQ(list.items,    nullptr);
}

TEST(EntityListTest, AddSingleEntity)
{
    EntityList list;

    Entity e{};
    strncpy(e.identity.id,    "ENT-001", sizeof(e.identity.id) - 1);
    strncpy(e.identity.title, "My entity", sizeof(e.identity.title) - 1);
    e.identity.kind = ENTITY_KIND_REQUIREMENT;

    EXPECT_EQ(list.push_back(e), 0);
    EXPECT_EQ((int)list.size(), 1);
    EXPECT_EQ(list[0].identity.id, std::string("ENT-001"));
    EXPECT_EQ(list[0].identity.title, std::string("My entity"));
}

TEST(EntityListTest, AddMultipleEntitiesGrowsArray)
{
    EntityList list;

    for (int i = 0; i < 20; i++) {
        Entity e{};
        snprintf(e.identity.id, sizeof(e.identity.id), "ENT-%03d", i);
        EXPECT_EQ(list.push_back(e), 0);
    }
    EXPECT_EQ((int)list.size(), 20);
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
    EXPECT_EQ(e.identity.id, std::string("REQ-001"));
    EXPECT_EQ(e.identity.title, std::string("Basic requirement"));
    EXPECT_EQ(e.identity.type_raw, std::string("functional"));
    EXPECT_EQ(e.identity.kind,        ENTITY_KIND_REQUIREMENT);
    EXPECT_EQ(e.lifecycle.status, std::string("draft"));
    EXPECT_EQ(e.lifecycle.priority, std::string("must"));
    EXPECT_EQ(e.text.description, std::string("A simple description."));
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
        "acceptance-criteria:\n"
        "  - Login form is shown\n"
        "  - Error displayed on wrong password\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.id, std::string("STORY-001"));
    EXPECT_EQ(e.identity.kind,         ENTITY_KIND_STORY);
    EXPECT_EQ(e.user_story.role, std::string("registered user"));
    EXPECT_EQ(e.user_story.goal, std::string("to log in with my email"));
    EXPECT_EQ(e.user_story.reason, std::string("I can access my account"));
    EXPECT_EQ(e.(int)acceptance_criteria.size(), 2);
    /* The two criteria should appear in the flat string. */
    EXPECT_NE(strstr(e.acceptance_criteria.criteria, "Login form is shown"), nullptr);
    EXPECT_NE(strstr(e.acceptance_criteria.criteria, "Error displayed on wrong password"), nullptr);
}

TEST(YamlParseEntityTest, StoryFileAlternativeKeys)
{
    /* Verify that as-a / i-want / so-that hyphenated keys work. */
    const char *path = write_yaml("ent_story_legacy.yaml",
        "id: STORY-002\n"
        "title: Legacy story\n"
        "type: story\n"
        "as-a: developer\n"
        "i-want: faster builds\n"
        "so-that: I save time\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.user_story.role, std::string("developer"));
    EXPECT_EQ(e.user_story.goal, std::string("faster builds"));
    EXPECT_EQ(e.user_story.reason, std::string("I save time"));
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
        "acceptance-criteria:\n"
        "  - Admin can create users\n"
        "  - Admin can delete users\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.kind,         ENTITY_KIND_REQUIREMENT);
    EXPECT_EQ(e.user_story.role, std::string("admin"));
    EXPECT_EQ(e.user_story.goal, std::string("manage users"));
    EXPECT_EQ(e.user_story.reason, std::string("the system stays secure"));
    EXPECT_STREQ(e.epic_membership.epic_id, "EPIC-ADMIN-001");
    EXPECT_EQ(e.(int)acceptance_criteria.size(), 2);
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
    EXPECT_EQ(e.identity.id, std::string("ASSUM-001"));
    EXPECT_EQ(e.identity.kind,    ENTITY_KIND_ASSUMPTION);
    EXPECT_NE(e.assumption.text[0], '\0');
    EXPECT_NE(strstr(e.assumption.text, "network connection"), nullptr);
    EXPECT_EQ(e.assumption.status, std::string("open"));
    EXPECT_EQ(e.assumption.source, std::string("NET-SPEC-001"));
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
    EXPECT_EQ(e.identity.id, std::string("CONSTR-001"));
    EXPECT_EQ(e.identity.kind,     ENTITY_KIND_CONSTRAINT);
    EXPECT_EQ(e.constraint.kind, std::string("technical"));
    EXPECT_NE(e.constraint.text[0], '\0');
    EXPECT_NE(strstr(e.constraint.text, "TLS version 1.3"), nullptr);
    EXPECT_EQ(e.constraint.source, std::string("SEC-POLICY-001"));
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
    EXPECT_EQ(e.identity.id, std::string("REQ-002"));
    EXPECT_EQ(e.identity.kind,    ENTITY_KIND_REQUIREMENT);

    /* Assumption component populated despite kind != ENTITY_KIND_ASSUMPTION. */
    EXPECT_NE(e.assumption.text[0], '\0');
    EXPECT_NE(strstr(e.assumption.text, "database server"), nullptr);
    EXPECT_EQ(e.assumption.status, std::string("open"));
    EXPECT_EQ(e.assumption.source, std::string("INFRA-SPEC-001"));

    /* Constraint component populated despite kind != ENTITY_KIND_CONSTRAINT. */
    EXPECT_NE(e.constraint.text[0], '\0');
    EXPECT_NE(strstr(e.constraint.text, "200 ms"), nullptr);
    EXPECT_EQ(e.constraint.kind, std::string("technical"));
    EXPECT_EQ(e.constraint.source, std::string("PERF-POLICY-001"));
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
    EXPECT_EQ(e.lifecycle.owner, std::string("alice"));
    EXPECT_EQ(e.lifecycle.version, std::string("1.2"));
    EXPECT_EQ(e.text.rationale, std::string("Needed for compliance."));
    EXPECT_EQ(e.(int)tags.size(), 2);
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

    int rc = yaml_parse_entities(path, &list);
    EXPECT_EQ(rc, 3);
    EXPECT_EQ((int)list.size(), 3);

    EXPECT_EQ(list[0].identity.kind, ENTITY_KIND_REQUIREMENT);
    EXPECT_EQ(list[0].identity.id, std::string("REQ-001"));

    EXPECT_EQ(list[1].identity.kind, ENTITY_KIND_STORY);
    EXPECT_EQ(list[1].identity.id, std::string("STORY-001"));
    EXPECT_EQ(list[1].user_story.role, std::string("developer"));

    EXPECT_EQ(list[2].identity.kind, ENTITY_KIND_ASSUMPTION);
    EXPECT_EQ(list[2].identity.id, std::string("ASSUM-001"));
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

    int rc = yaml_parse_entities(path, &list);
    EXPECT_EQ(rc, 2);
    EXPECT_EQ(list[0].identity.id, std::string("REQ-001"));
    EXPECT_EQ(list[1].identity.id, std::string("CONSTR-001"));
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

    yaml_parse_entities(path, &list);
    ASSERT_EQ((int)list.size(), 2);
    EXPECT_EQ(list[0].identity.doc_index, 0);
    EXPECT_EQ(list[1].identity.doc_index, 1);
}

TEST(YamlParseEntitiesTest, NonexistentFile)
{
    EntityList list;

    int rc = yaml_parse_entities("/tmp/no_such_file_xyz.yaml", &list);
    EXPECT_EQ(rc, -1);
    EXPECT_EQ((int)list.size(), 0);
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
    EXPECT_EQ(e.(int)traceability.size(), 3);
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
    EXPECT_EQ(e.(int)traceability.size(), 1);
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
    EXPECT_EQ(e.(int)traceability.size(), 0);
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
    EXPECT_EQ(e.(int)sources.size(), 2);
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
    EXPECT_EQ(e.(int)sources.size(), 2);
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
    EXPECT_EQ(e.(int)sources.size(), 0);
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
    ASSERT_EQ(e.(int)traceability.size(), 2);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    int added = entity_traceability_to_triplets(&e, store);
    EXPECT_EQ(added, 2);
    EXPECT_EQ(triplet_store_count(store), 2u);

    /* Verify the triples are queryable by subject. */
    CTripleList list = triplet_store_find_by_subject(store, "REQ-SW-020");
    ASSERT_EQ((int)list.size(), 2u);
    triplet_store_list_free(list);

    triplet_store_destroy(store);
}

TEST(TraceabilityToTripletsTest, NullInputsReturnMinusOne)
{
    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    Entity e{};
    EXPECT_EQ(entity_traceability_to_triplets(nullptr, store), -1);
    EXPECT_EQ(entity_traceability_to_triplets(&e, nullptr),    -1);

    triplet_store_destroy(store);
}

TEST(TraceabilityToTripletsTest, EmptyTraceabilityReturnsZero)
{
    Entity e{};
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
    ASSERT_EQ(e.(int)doc_membership.size(), 2);

    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    int added = entity_doc_membership_to_triplets(&e, store);
    EXPECT_EQ(added, 2);
    EXPECT_EQ(triplet_store_count(store), 2u);

    /* Both triples must use "part-of" as the predicate. */
    CTripleList list = triplet_store_find_by_subject(store, "REQ-SW-100");
    ASSERT_EQ((int)list.size(), 2u);
    int part_of_count = 0;
    for (size_t i = 0; i < (int)list.size(); i++) {
        if (strcmp(list.triples[i].predicate, "part-of") == 0)
            part_of_count++;
    }
    EXPECT_EQ(part_of_count, 2);
    triplet_store_list_free(list);

    /* The objects must be the two document IDs. */
    CTripleList by_obj1 = triplet_store_find_by_object(store, "SRS-CLIENT-001");
    EXPECT_GE((int)by_obj1.size(), 1u);
    triplet_store_list_free(by_obj1);

    CTripleList by_obj2 = triplet_store_find_by_object(store, "SDD-SYS-001");
    EXPECT_GE((int)by_obj2.size(), 1u);
    triplet_store_list_free(by_obj2);

    triplet_store_destroy(store);
}

TEST(DocMembershipToTripletsTest, NullInputsReturnMinusOne)
{
    TripletStore *store = triplet_store_create();
    ASSERT_NE(store, nullptr);

    Entity e{};
    EXPECT_EQ(entity_doc_membership_to_triplets(nullptr, store), -1);
    EXPECT_EQ(entity_doc_membership_to_triplets(&e, nullptr),    -1);

    triplet_store_destroy(store);
}

TEST(DocMembershipToTripletsTest, EmptyMembershipReturnsZero)
{
    Entity e{};
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
    for (size_t i = 0; i < (int)by_doc.size(); i++) {
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
    /* A document entity carries the doc-meta component. */
    const char *path = write_yaml("ent_doc_meta.yaml",
        "id: SRS-CLIENT-001\n"
        "title: SRS for Client Project\n"
        "type: document\n"
        "doc-meta:\n"
        "  doc-type: SRS\n"
        "  version: 1.2\n"
        "  client: ClientCorp\n"
        "  status: approved\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.id, std::string("SRS-CLIENT-001"));
    EXPECT_EQ(e.identity.kind,         ENTITY_KIND_DOCUMENT);
    EXPECT_EQ(e.doc_meta.doc_type, std::string("SRS"));
    EXPECT_EQ(e.doc_meta.version, std::string("1.2"));
    EXPECT_EQ(e.doc_meta.client, std::string("ClientCorp"));
    EXPECT_EQ(e.doc_meta.status, std::string("approved"));
    /* Membership component must be zero when not set. */
    EXPECT_EQ(e.(int)doc_membership.size(),  0);
    EXPECT_EQ(e.doc_membership.doc_ids[0], '\0');
}

TEST(YamlParseEntityTest, DocumentMetaWithOptionalTitle)
{
    /* doc-meta may carry its own title field. */
    const char *path = write_yaml("ent_doc_meta_title.yaml",
        "id: SDD-SYS-001\n"
        "title: SDD for System\n"
        "type: sdd\n"
        "doc-meta:\n"
        "  title: Software Design Description v2\n"
        "  doc-type: SDD\n"
        "  version: 2.0\n"
        "  client: AcmeCorp\n"
        "  status: draft\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.kind,         ENTITY_KIND_DOCUMENT);
    EXPECT_EQ(e.doc_meta.title, std::string("Software Design Description v2"));
    EXPECT_EQ(e.doc_meta.doc_type, std::string("SDD"));
    EXPECT_EQ(e.doc_meta.version, std::string("2.0"));
    EXPECT_EQ(e.doc_meta.client, std::string("AcmeCorp"));
    EXPECT_EQ(e.doc_meta.status, std::string("draft"));
}

TEST(YamlParseEntityTest, DocumentMetaEmptyWhenAbsent)
{
    /* Entities without a doc-meta key have a zero-initialised component. */
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
    EXPECT_EQ(e.(int)doc_membership.size(), 2);
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
    EXPECT_EQ(e.(int)doc_membership.size(), 1);
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
    EXPECT_EQ(e.(int)doc_membership.size(),      0);
    EXPECT_EQ(e.doc_membership.doc_ids[0], '\0');
}

TEST(YamlParseEntityTest, AnyEntityCanCarryDocumentComponents)
{
    /* A requirement entity carrying both doc-meta and doc-membership. */
    const char *path = write_yaml("ent_req_doc_components.yaml",
        "id: REQ-012\n"
        "title: Requirement with document components\n"
        "type: functional\n"
        "doc-meta:\n"
        "  doc-type: SRS\n"
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
    EXPECT_EQ(e.doc_meta.doc_type, std::string("SRS"));
    EXPECT_EQ(e.doc_meta.version, std::string("3.0"));
    EXPECT_EQ(e.doc_meta.client, std::string("MegaCorp"));
    EXPECT_EQ(e.(int)doc_membership.size(),   1);
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
    EXPECT_EQ(e.identity.id, std::string("DN-001"));
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
    EXPECT_EQ(e.identity.id, std::string("DN-LARGE-001"));
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
    EXPECT_EQ(e.identity.id, std::string("DN-NEARMAX-001"));
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
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, nullptr), 1);
    EXPECT_EQ(entity_has_component(&e, ""),      1);
}

TEST(EntityHasComponentTest, UserStoryAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "user-story"),  0);

    strncpy(e.user_story.role, "developer", sizeof(e.user_story.role) - 1);
    EXPECT_EQ(entity_has_component(&e, "user-story"),  1);
}

TEST(EntityHasComponentTest, AcceptanceCriteriaAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "acceptance-criteria"), 0);

    e.(int)acceptance_criteria.size() = 1;
    EXPECT_EQ(entity_has_component(&e, "acceptance-criteria"), 1);
}

TEST(EntityHasComponentTest, EpicMembershipAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "epic"),            0);
    EXPECT_EQ(entity_has_component(&e, "epic-membership"), 0);

    strncpy(e.epic_membership.epic_id, "EPIC-001",
            sizeof(e.epic_membership.epic_id) - 1);
    EXPECT_EQ(entity_has_component(&e, "epic"),            1);
    EXPECT_EQ(entity_has_component(&e, "epic-membership"), 1);
}

TEST(EntityHasComponentTest, AssumptionAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "assumption"), 0);

    strncpy(e.assumption.text, "Some assumption",
            sizeof(e.assumption.text) - 1);
    EXPECT_EQ(entity_has_component(&e, "assumption"), 1);
}

TEST(EntityHasComponentTest, ConstraintAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "constraint"), 0);

    strncpy(e.constraint.text, "Some constraint",
            sizeof(e.constraint.text) - 1);
    EXPECT_EQ(entity_has_component(&e, "constraint"), 1);
}

TEST(EntityHasComponentTest, DocMetaAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "doc-meta"), 0);

    strncpy(e.doc_meta.doc_type, "SRS", sizeof(e.doc_meta.doc_type) - 1);
    EXPECT_EQ(entity_has_component(&e, "doc-meta"), 1);
}

TEST(EntityHasComponentTest, DocMembershipAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "doc-membership"), 0);
    EXPECT_EQ(entity_has_component(&e, "documents"),      0);

    e.(int)doc_membership.size() = 1;
    EXPECT_EQ(entity_has_component(&e, "doc-membership"), 1);
    EXPECT_EQ(entity_has_component(&e, "documents"),      1);
}

TEST(EntityHasComponentTest, DocBodyAbsentAndPresent)
{
    Entity e{};
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
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "traceability"), 0);

    e.(int)traceability.size() = 1;
    EXPECT_EQ(entity_has_component(&e, "traceability"), 1);
}

TEST(EntityHasComponentTest, SourcesAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "sources"), 0);

    e.(int)sources.size() = 1;
    EXPECT_EQ(entity_has_component(&e, "sources"), 1);
}

TEST(EntityHasComponentTest, TagsAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "tags"), 0);

    e.(int)tags.size() = 2;
    EXPECT_EQ(entity_has_component(&e, "tags"), 1);
}

TEST(EntityHasComponentTest, UnknownComponentNameReturnsFalse)
{
    Entity e{};
    /* Fill everything so we can confirm unknown names still return 0. */
    strncpy(e.assumption.text, "x", sizeof(e.assumption.text) - 1);
    EXPECT_EQ(entity_has_component(&e, "no-such-component"), 0);
}

TEST(EntityHasComponentTest, TestProcedureAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "test-procedure"), 0);

    e.test_procedure.step_count = 1;
    EXPECT_EQ(entity_has_component(&e, "test-procedure"), 1);
}

TEST(EntityHasComponentTest, ClauseCollectionAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "clause-collection"), 0);
    EXPECT_EQ(entity_has_component(&e, "clauses"),           0);

    e.(int)clause_collection.size() = 2;
    EXPECT_EQ(entity_has_component(&e, "clause-collection"), 1);
    EXPECT_EQ(entity_has_component(&e, "clauses"),           1);
}

TEST(EntityHasComponentTest, AttachmentAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "attachment"),  0);
    EXPECT_EQ(entity_has_component(&e, "attachments"), 0);

    e.(int)attachment.size() = 1;
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
        "    expected-output: System returns HTTP 200.\n"
        "  - step: 2\n"
        "    action: Access protected resource.\n"
        "    expected-output: Resource content is returned.\n"
        "expected-result: User gains access to the protected resource.\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.kind,                      ENTITY_KIND_TEST_CASE);
    EXPECT_EQ(e.identity.id, std::string("TC-SW-002"));
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
        "expected-result: System behaves correctly.\n");
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
    EXPECT_EQ(e.identity.id, std::string("EXT-MACH-DIR"));
    EXPECT_EQ(e.(int)clause_collection.size(), 2);
    ASSERT_NE(e.clause_collection.clauses, nullptr);
    EXPECT_NE(strstr(e.clause_collection.clauses, "annex-I-1.1.2"),                  nullptr);
    EXPECT_NE(strstr(e.clause_collection.clauses, "Principles of safety integration"), nullptr);
    EXPECT_NE(strstr(e.clause_collection.clauses, "annex-I-1.2.1"),                  nullptr);
    EXPECT_EQ(entity_has_component(&e, "clause-collection"), 1);
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
    EXPECT_EQ(e.(int)clause_collection.size(), 1);
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
    EXPECT_EQ(e.(int)clause_collection.size(),   0);
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
    EXPECT_EQ(e.identity.id, std::string("SRS-CLIENT-002"));
    EXPECT_EQ(e.(int)attachment.size(),  2);
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
    EXPECT_EQ(e.(int)attachment.size(), 1);
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
    EXPECT_EQ(e.(int)attachment.size(),       0);
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
        "    expected-output: Test passes.\n"
        "expected-result: All assertions pass.\n"
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
    EXPECT_EQ(e.(int)clause_collection.size(),            1);
    ASSERT_NE(e.clause_collection.clauses,          nullptr);
    EXPECT_NE(strstr(e.clause_collection.clauses, "sec-4.1"), nullptr);
    EXPECT_EQ(e.(int)attachment.size(),                   1);
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

    Entity e{};

    strncpy(e.identity.id, "REQ-003", sizeof(e.identity.id) - 1);
    list.push_back(e);
    strncpy(e.identity.id, "REQ-001", sizeof(e.identity.id) - 1);
    list.push_back(e);
    strncpy(e.identity.id, "REQ-002", sizeof(e.identity.id) - 1);
    list.push_back(e);

    qsort(list.items, (size_t)(int)list.size(), sizeof(Entity), entity_cmp_by_id);

    EXPECT_EQ(list[0].identity.id, std::string("REQ-001"));
    EXPECT_EQ(list[1].identity.id, std::string("REQ-002"));
    EXPECT_EQ(list[2].identity.id, std::string("REQ-003"));
}

/* =========================================================================
 * Tests — entity_apply_filter
 * ======================================================================= */

static Entity make_test_entity(const char *id, EntityKind kind,
                                const char *status, const char *priority)
{
    Entity e{};
    e.identity.id = id;
    e.identity.kind = kind;
    e.lifecycle.status = status;
    e.lifecycle.priority = priority;
    return e;
}

TEST(EntityApplyFilterTest, NullFiltersPassesAll)
{
    EntityList src, dst;


    Entity e1 = make_test_entity("REQ-001", ENTITY_KIND_REQUIREMENT,
                                  "draft", "must");
    Entity e2 = make_test_entity("TC-001",  ENTITY_KIND_TEST_CASE,
                                  "approved", "should");
    src.push_back(e1);
    src.push_back(e2);

    entity_apply_filter(&src, &dst, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ((int)dst.size(), 2);

}

TEST(EntityApplyFilterTest, KindFilterSelectsOnlyRequirements)
{
    EntityList src, dst;


    Entity e1 = make_test_entity("REQ-001", ENTITY_KIND_REQUIREMENT,
                                  "draft", "must");
    Entity e2 = make_test_entity("TC-001",  ENTITY_KIND_TEST_CASE,
                                  "draft", "must");
    src.push_back(e1);
    src.push_back(e2);

    entity_apply_filter(&src, &dst, "requirement", nullptr, nullptr, nullptr);
    EXPECT_EQ((int)dst.size(), 1);
    EXPECT_EQ(dst[0].identity.id, std::string("REQ-001"));

}

TEST(EntityApplyFilterTest, StatusFilterSelectsApproved)
{
    EntityList src, dst;


    Entity e1 = make_test_entity("REQ-001", ENTITY_KIND_REQUIREMENT,
                                  "approved", "must");
    Entity e2 = make_test_entity("REQ-002", ENTITY_KIND_REQUIREMENT,
                                  "draft", "must");
    src.push_back(e1);
    src.push_back(e2);

    entity_apply_filter(&src, &dst, nullptr, nullptr, "approved", nullptr);
    EXPECT_EQ((int)dst.size(), 1);
    EXPECT_EQ(dst[0].identity.id, std::string("REQ-001"));

}

TEST(EntityApplyFilterTest, PriorityFilterSelectsMust)
{
    EntityList src, dst;


    Entity e1 = make_test_entity("REQ-001", ENTITY_KIND_REQUIREMENT,
                                  "draft", "must");
    Entity e2 = make_test_entity("REQ-002", ENTITY_KIND_REQUIREMENT,
                                  "draft", "should");
    src.push_back(e1);
    src.push_back(e2);

    entity_apply_filter(&src, &dst, nullptr, nullptr, nullptr, "must");
    EXPECT_EQ((int)dst.size(), 1);
    EXPECT_EQ(dst[0].identity.id, std::string("REQ-001"));

}

TEST(EntityApplyFilterTest, CombinedKindAndStatusFilters)
{
    EntityList src, dst;


    Entity e1 = make_test_entity("REQ-001", ENTITY_KIND_REQUIREMENT,
                                  "approved", "must");
    Entity e2 = make_test_entity("REQ-002", ENTITY_KIND_REQUIREMENT,
                                  "draft", "must");
    Entity e3 = make_test_entity("TC-001",  ENTITY_KIND_TEST_CASE,
                                  "approved", "must");
    src.push_back(e1);
    src.push_back(e2);
    src.push_back(e3);

    entity_apply_filter(&src, &dst, "requirement", nullptr, "approved", nullptr);
    EXPECT_EQ((int)dst.size(), 1);
    EXPECT_EQ(dst[0].identity.id, std::string("REQ-001"));

}

TEST(EntityApplyFilterTest, NoMatchGivesEmptyDst)
{
    EntityList src, dst;


    Entity e1 = make_test_entity("REQ-001", ENTITY_KIND_REQUIREMENT,
                                  "draft", "must");
    src.push_back(e1);

    entity_apply_filter(&src, &dst, "test-case", nullptr, nullptr, nullptr);
    EXPECT_EQ((int)dst.size(), 0);

}

TEST(EntityApplyFilterTest, EmptySourceGivesEmptyDst)
{
    EntityList src, dst;


    entity_apply_filter(&src, &dst, "requirement", nullptr, nullptr, nullptr);
    EXPECT_EQ((int)dst.size(), 0);

}

/* =========================================================================
 * Tests — entity_free / entity_copy / memory management
 * ======================================================================= */

TEST(EntityFreeTest, FreeOnZeroInitialisedEntityIsSafe)
{
    /* entity_free() on a fully-zeroed entity must not crash. */
    Entity e{};
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
    Entity e{};
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
    Entity e{};
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
    EXPECT_EQ(dst.identity.id, std::string("REQ-CPY-001"));
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
    src.(int)clause_collection.size() = 1;
    strncpy(src.attachment.attachments, "file.pdf\tdesc", 63);
    src.(int)attachment.size() = 1;

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
    EXPECT_EQ(dst.(int)clause_collection.size(), 1);

    ASSERT_NE(dst.attachment.attachments, nullptr);
    EXPECT_NE(dst.attachment.attachments, src.attachment.attachments);
    EXPECT_EQ(dst.(int)attachment.size(), 1);

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

    Entity src;
    memset(&src, 0, sizeof(src));
    strncpy(src.identity.id, "DN-LIST-001", sizeof(src.identity.id) - 1);
    src.doc_body.body = strdup("list body");
    ASSERT_NE(src.doc_body.body, nullptr);

    EXPECT_EQ(list.push_back(src), 0);
    EXPECT_EQ((int)list.size(), 1);

    /* Free the source — the list copy must still be valid. */
    entity_free(&src);
    EXPECT_EQ(src.doc_body.body, nullptr);

    /* List item must still have its own copy. */
    ASSERT_NE(list[0].doc_body.body, nullptr);
    EXPECT_STREQ(list[0].doc_body.body, "list body");
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
        "expected-result: Test passes.\n";

    /* Write to tmp */
    FILE *f = fopen("/tmp/mem_test.yaml", "w");
    ASSERT_NE(f, nullptr);
    fputs(yaml, f);
    fclose(f);

    EntityList list;
    int added = yaml_parse_entities("/tmp/mem_test.yaml", &list);
    EXPECT_EQ(added, 2);
    EXPECT_EQ((int)list.size(), 2);

    /* Verify heap fields were set. */
    EXPECT_NE(list[0].doc_body.body, nullptr);
    EXPECT_NE(list[1].test_procedure.preconditions, nullptr);
    EXPECT_NE(list[1].test_procedure.expected_result, nullptr);

    /* This must free all heap fields without leaking. */
    EXPECT_EQ(list.items,    nullptr);
    EXPECT_EQ((int)list.size(),    0);
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

    /* Add the same entity twice via two separate parses. */
    Entity e1, e2;
    ASSERT_EQ(yaml_parse_entity(path, &e1), 0);
    ASSERT_EQ(yaml_parse_entity(path, &e2), 0);
    EXPECT_EQ(list.push_back(e1), 0);
    EXPECT_EQ(list.push_back(e2), 0);
    entity_free(&e1);
    entity_free(&e2);

    EXPECT_EQ((int)list.size(), 2);
    /* Both items have independent heap pointers. */
    ASSERT_NE(list[0].doc_body.body, nullptr);
    ASSERT_NE(list[1].doc_body.body, nullptr);
    EXPECT_NE(list[0].doc_body.body, list[1].doc_body.body);
    EXPECT_STREQ(list[0].doc_body.body, "shared body");
    EXPECT_STREQ(list[1].doc_body.body, "shared body");
}

/* =========================================================================
 * Tests — ENTITY_KIND_DOCUMENT_SCHEMA / entity_kind_from_string
 * ======================================================================= */

TEST(EntityKindTest, DocumentSchema)
{
    EXPECT_EQ(entity_kind_from_string("document-schema"),
              ENTITY_KIND_DOCUMENT_SCHEMA);
}

TEST(EntityKindTest, DocumentSchemaLabel)
{
    EXPECT_STREQ(entity_kind_label(ENTITY_KIND_DOCUMENT_SCHEMA),
                 "document-schema");
}

/* =========================================================================
 * Tests — AppliesToComponent (yaml_parse_entity)
 * ======================================================================= */

TEST(YamlParseEntityTest, AppliesToScalar)
{
    /* applies-to given as a scalar string. */
    const char *path = write_yaml("ent_applies_scalar.yaml",
        "id: REQ-ACME-001\n"
        "title: Acme-specific requirement\n"
        "type: functional\n"
        "applies-to: acme\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.(int)applies_to.size(), 1);
    EXPECT_NE(strstr(e.applies_to.applies_to, "acme"), nullptr);
    EXPECT_EQ(entity_has_component(&e, "applies-to"), 1);
}

TEST(YamlParseEntityTest, AppliesToSequence)
{
    /* applies-to given as a sequence of applicability tags. */
    const char *path = write_yaml("ent_applies_seq.yaml",
        "id: REQ-SHARED-001\n"
        "title: Shared requirement\n"
        "type: functional\n"
        "applies-to:\n"
        "  - acme\n"
        "  - bmw\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.(int)applies_to.size(), 2);
    EXPECT_NE(strstr(e.applies_to.applies_to, "acme"), nullptr);
    EXPECT_NE(strstr(e.applies_to.applies_to, "bmw"),  nullptr);
    EXPECT_EQ(entity_has_component(&e, "applies-to"), 1);
}

TEST(YamlParseEntityTest, AppliesToAbsentWhenMissing)
{
    /* Entities without applies-to have a zero-initialised component. */
    const char *path = write_yaml("ent_no_applies.yaml",
        "id: REQ-GEN-001\n"
        "title: Generic requirement\n"
        "type: functional\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.(int)applies_to.size(),          0);
    EXPECT_EQ(e.applies_to.applies_to[0],  '\0');
    EXPECT_EQ(entity_has_component(&e, "applies-to"), 0);
}

/* =========================================================================
 * Tests — VariantProfileComponent (yaml_parse_entity)
 * ======================================================================= */

TEST(YamlParseEntityTest, VariantProfileCustomerAndDelivery)
{
    /* A document-schema entity with variant-profile using "delivery" alias. */
    const char *path = write_yaml("ent_variant_profile.yaml",
        "id: SCHEMA-ACME-001\n"
        "title: Acme Document Schema\n"
        "type: document-schema\n"
        "status: draft\n"
        "variant-profile:\n"
        "  customer: acme\n"
        "  delivery: v1.0\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.kind, ENTITY_KIND_DOCUMENT_SCHEMA);
    EXPECT_STREQ(e.variant_profile.customer, "acme");
    EXPECT_STREQ(e.variant_profile.product,  "v1.0");
    EXPECT_EQ(entity_has_component(&e, "variant-profile"), 1);
}

TEST(YamlParseEntityTest, VariantProfileCustomerAndProduct)
{
    /* variant-profile using the canonical "product" key. */
    const char *path = write_yaml("ent_variant_product.yaml",
        "id: SCHEMA-BMW-001\n"
        "title: BMW Document Schema\n"
        "type: document-schema\n"
        "variant-profile:\n"
        "  customer: bmw\n"
        "  product: platform-x\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(e.variant_profile.customer, "bmw");
    EXPECT_STREQ(e.variant_profile.product,  "platform-x");
    EXPECT_EQ(entity_has_component(&e, "variant-profile"), 1);
}

TEST(YamlParseEntityTest, VariantProfileAbsentWhenMissing)
{
    /* Entities without variant-profile have a zero-initialised component. */
    const char *path = write_yaml("ent_no_variant.yaml",
        "id: REQ-017\n"
        "title: Requirement without variant profile\n"
        "type: functional\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.variant_profile.customer[0], '\0');
    EXPECT_EQ(e.variant_profile.product[0],  '\0');
    EXPECT_EQ(entity_has_component(&e, "variant-profile"), 0);
}

/* =========================================================================
 * Tests — CompositionProfileComponent (yaml_parse_entity)
 * ======================================================================= */

TEST(YamlParseEntityTest, CompositionProfileOrderSequence)
{
    /* A document-schema entity with a composition-profile.order sequence. */
    const char *path = write_yaml("ent_comp_profile.yaml",
        "id: SCHEMA-SRS-001\n"
        "title: SRS Composition Schema\n"
        "type: document-schema\n"
        "composition-profile:\n"
        "  order:\n"
        "    - SEC-INTRO\n"
        "    - SEC-FUNC\n"
        "    - SEC-TRACE\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.kind, ENTITY_KIND_DOCUMENT_SCHEMA);
    EXPECT_EQ(e.composition_profile.order_count, 3);
    EXPECT_NE(strstr(e.composition_profile.order, "SEC-INTRO"), nullptr);
    EXPECT_NE(strstr(e.composition_profile.order, "SEC-FUNC"),  nullptr);
    EXPECT_NE(strstr(e.composition_profile.order, "SEC-TRACE"), nullptr);
    EXPECT_EQ(entity_has_component(&e, "composition-profile"), 1);
}

TEST(YamlParseEntityTest, CompositionProfileAbsentWhenMissing)
{
    /* Entities without composition-profile have a zero-initialised component. */
    const char *path = write_yaml("ent_no_comp_profile.yaml",
        "id: REQ-018\n"
        "title: Requirement without composition profile\n"
        "type: functional\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.composition_profile.order_count, 0);
    EXPECT_EQ(e.composition_profile.order[0],    '\0');
    EXPECT_EQ(entity_has_component(&e, "composition-profile"), 0);
}

/* =========================================================================
 * Tests — RenderProfileComponent (yaml_parse_entity)
 * ======================================================================= */

TEST(YamlParseEntityTest, RenderProfileMarkdown)
{
    /* A document-schema entity with render-profile.format = markdown. */
    const char *path = write_yaml("ent_render_profile.yaml",
        "id: SCHEMA-MD-001\n"
        "title: Markdown Schema\n"
        "type: document-schema\n"
        "render-profile:\n"
        "  format: markdown\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(e.render_profile.format, "markdown");
    EXPECT_EQ(entity_has_component(&e, "render-profile"), 1);
}

TEST(YamlParseEntityTest, RenderProfileHtml)
{
    /* render-profile with html format. */
    const char *path = write_yaml("ent_render_html.yaml",
        "id: SCHEMA-HTML-001\n"
        "title: HTML Schema\n"
        "type: document-schema\n"
        "render-profile:\n"
        "  format: html\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_STREQ(e.render_profile.format, "html");
    EXPECT_EQ(entity_has_component(&e, "render-profile"), 1);
}

TEST(YamlParseEntityTest, RenderProfileAbsentWhenMissing)
{
    /* Entities without render-profile have a zero-initialised component. */
    const char *path = write_yaml("ent_no_render.yaml",
        "id: REQ-019\n"
        "title: Requirement without render profile\n"
        "type: functional\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.render_profile.format[0], '\0');
    EXPECT_EQ(entity_has_component(&e, "render-profile"), 0);
}

/* =========================================================================
 * Tests — Full document-schema entity parse (all new components together)
 * ======================================================================= */

TEST(YamlParseEntityTest, DocumentSchemaFullParse)
{
    /* A complete document-schema entity as proposed in the issue. */
    const char *path = write_yaml("ent_doc_schema_full.yaml",
        "id: SRS-ACME-001\n"
        "title: Acme SRS Document Structure\n"
        "type: document-schema\n"
        "status: draft\n"
        "applies-to:\n"
        "  - acme\n"
        "variant-profile:\n"
        "  customer: acme\n"
        "  delivery: v1.0\n"
        "composition-profile:\n"
        "  order:\n"
        "    - SEC-INTRO\n"
        "    - SEC-FUNC\n"
        "    - SEC-TRACE\n"
        "render-profile:\n"
        "  format: markdown\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(e.identity.id, std::string("SRS-ACME-001"));
    EXPECT_EQ(e.identity.kind,  ENTITY_KIND_DOCUMENT_SCHEMA);

    /* applies_to */
    EXPECT_EQ(e.(int)applies_to.size(), 1);
    EXPECT_NE(strstr(e.applies_to.applies_to, "acme"), nullptr);

    /* variant_profile */
    EXPECT_STREQ(e.variant_profile.customer, "acme");
    EXPECT_STREQ(e.variant_profile.product,  "v1.0");

    /* composition_profile */
    EXPECT_EQ(e.composition_profile.order_count, 3);
    EXPECT_NE(strstr(e.composition_profile.order, "SEC-INTRO"), nullptr);
    EXPECT_NE(strstr(e.composition_profile.order, "SEC-FUNC"),  nullptr);
    EXPECT_NE(strstr(e.composition_profile.order, "SEC-TRACE"), nullptr);

    /* render_profile */
    EXPECT_STREQ(e.render_profile.format, "markdown");

    EXPECT_EQ(entity_has_component(&e, "applies-to"),          1);
    EXPECT_EQ(entity_has_component(&e, "variant-profile"),     1);
    EXPECT_EQ(entity_has_component(&e, "composition-profile"), 1);
    EXPECT_EQ(entity_has_component(&e, "render-profile"),      1);
}

/* =========================================================================
 * Tests — entity_has_component: new components absent/present
 * ======================================================================= */

TEST(EntityHasComponentTest, AppliesToAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "applies-to"), 0);

    e.(int)applies_to.size() = 1;
    EXPECT_EQ(entity_has_component(&e, "applies-to"), 1);
}

TEST(EntityHasComponentTest, VariantProfileAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "variant-profile"), 0);

    strncpy(e.variant_profile.customer, "acme",
            sizeof(e.variant_profile.customer) - 1);
    EXPECT_EQ(entity_has_component(&e, "variant-profile"), 1);
}

TEST(EntityHasComponentTest, CompositionProfileAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "composition-profile"), 0);

    e.composition_profile.order_count = 2;
    EXPECT_EQ(entity_has_component(&e, "composition-profile"), 1);
}

TEST(EntityHasComponentTest, RenderProfileAbsentAndPresent)
{
    Entity e{};
    EXPECT_EQ(entity_has_component(&e, "render-profile"), 0);

    strncpy(e.render_profile.format, "html",
            sizeof(e.render_profile.format) - 1);
    EXPECT_EQ(entity_has_component(&e, "render-profile"), 1);
}

TEST(EntityApplyFilterTest, KindFilterSelectsDocumentSchema)
{
    EntityList src, dst;


    Entity e1 = make_test_entity("SCHEMA-001", ENTITY_KIND_DOCUMENT_SCHEMA,
                                  "draft", "must");
    Entity e2 = make_test_entity("REQ-001", ENTITY_KIND_REQUIREMENT,
                                  "draft", "must");
    src.push_back(e1);
    src.push_back(e2);

    entity_apply_filter(&src, &dst, "document-schema", nullptr, nullptr,
                        nullptr);
    EXPECT_EQ((int)dst.size(), 1);
    EXPECT_EQ(dst[0].identity.id, std::string("SCHEMA-001"));

}
