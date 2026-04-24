/**
 * @file test_entity.cpp
 * @brief Unit tests for entity.c and the new yaml_parse_entity /
 *        yaml_parse_entities functions in yaml_simple.c.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstring>

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

    strncpy(e.doc_body.body, "Some body text",
            sizeof(e.doc_body.body) - 1);
    EXPECT_EQ(entity_has_component(&e, "doc-body"), 1);
    EXPECT_EQ(entity_has_component(&e, "body"),     1);
}

TEST(EntityHasComponentTest, TraceabilityAbsentAndPresent)
{
    Entity e;
    memset(&e, 0, sizeof(e));
    EXPECT_EQ(entity_has_component(&e, "traceability"), 0);

    e.traceability.count = 1;
    EXPECT_EQ(entity_has_component(&e, "traceability"), 1);
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
