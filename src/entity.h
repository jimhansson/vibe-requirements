#ifndef VIBE_ENTITY_H
#define VIBE_ENTITY_H

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * EntityKind — discriminates the domain role of every entity.
 * The numeric values are stable; do NOT reorder existing entries.
 * --------------------------------------------------------------------- */

/** All recognised domain-object kinds. */
typedef enum {
    ENTITY_KIND_REQUIREMENT = 0, /**< functional / non-functional requirement */
    ENTITY_KIND_GROUP,           /**< requirement group / section heading      */
    ENTITY_KIND_STORY,           /**< user story                               */
    ENTITY_KIND_DESIGN_NOTE,     /**< SDD free-form design content             */
    ENTITY_KIND_SECTION,         /**< non-normative document section           */
    ENTITY_KIND_ASSUMPTION,      /**< an assumption recorded in SRS/SDD        */
    ENTITY_KIND_CONSTRAINT,      /**< technical / regulatory / business const. */
    ENTITY_KIND_TEST_CASE,       /**< explicit test case                       */
    ENTITY_KIND_EXTERNAL,        /**< external normative source                */
    ENTITY_KIND_UNKNOWN          /**< fallback for unrecognised 'type' values  */
} EntityKind;

/* -----------------------------------------------------------------------
 * Component structs
 * Every field is a fixed-size char array so that the whole Entity can be
 * stack-allocated and memset to zero with no heap involvement.
 * --------------------------------------------------------------------- */

/** Core identity — every entity has exactly one of these. */
typedef struct {
    char       id[64];        /**< unique identifier (YAML "id:")            */
    char       title[256];    /**< human-readable title (YAML "title:")      */
    EntityKind kind;          /**< derived from YAML "type" field            */
    char       type_raw[32];  /**< verbatim "type" string from YAML          */
    char       file_path[512];/**< filesystem path of the source YAML file   */
    int        doc_index;     /**< 0-based position in YAML stream           */
} IdentityComponent;

/** Lifecycle / workflow metadata — present on most entities. */
typedef struct {
    char status[32];    /**< e.g. "draft", "approved", "deprecated"  */
    char priority[32];  /**< e.g. "must", "should", "could"          */
    char owner[128];    /**< person or team responsible               */
    char version[32];   /**< schema / document version string         */
} LifecycleComponent;

/** Free-text narrative fields. */
typedef struct {
    char description[2048]; /**< detailed description (YAML "description:") */
    char rationale[1024];   /**< justification / rationale (YAML "rationale:") */
} TextComponent;

/** Maximum byte size of the flat tag store. */
#define TAG_STORE_LEN 1024

/** Variable-length tag list stored as a newline-separated flat string. */
typedef struct {
    char tags[TAG_STORE_LEN]; /**< newline-separated list of tag strings */
    int  count;               /**< number of tags stored                 */
} TagComponent;

/**
 * User-story component — can be attached to any entity.
 * An entity becomes a user story by carrying this component.
 */
typedef struct {
    char role[256];    /**< "As a <role>"        (YAML: "role" or legacy "as_a")   */
    char goal[512];    /**< "I want <goal>"      (YAML: "goal" or legacy "i_want") */
    char reason[512];  /**< "So that <reason>"   (YAML: "reason" or legacy "so_that") */
} UserStoryComponent;

/** Maximum byte size of the acceptance-criteria store. */
#define AC_STORE_LEN 2048

/**
 * Acceptance-criteria component — can be attached to any entity.
 * Stores each criterion as a newline-separated flat string.
 */
typedef struct {
    char criteria[AC_STORE_LEN]; /**< newline-separated acceptance criteria */
    int  count;                  /**< number of criteria stored             */
} AcceptanceCriteriaComponent;

/** Epic-membership component — records which epic an entity belongs to. */
typedef struct {
    char epic_id[64]; /**< entity-id of the parent epic (YAML: "epic") */
} EpicMembershipComponent;

/**
 * Assumption component — can be attached to any entity.
 *
 * Schema: {text, status, source}
 *   text   — the assumption being made
 *   status — e.g. "open", "confirmed", "invalid"
 *   source — reference to a document or entity that validates the assumption
 */
typedef struct {
    char text[1024];   /**< the assumption being made                        */
    char status[32];   /**< e.g. "open", "confirmed", "invalid"              */
    char source[256];  /**< reference to validating document or entity ID    */
} AssumptionComponent;

/**
 * Constraint component — can be attached to any entity.
 *
 * Schema: {text, kind, source}
 *   text   — the constraint wording
 *   kind   — e.g. "legal", "technical", "environmental"
 *   source — reference to the document or regulation imposing the constraint
 */
typedef struct {
    char text[1024];   /**< constraint wording                               */
    char kind[64];     /**< "legal", "technical", "environmental", …         */
    char source[256];  /**< reference to the imposing document or entity ID  */
} ConstraintComponent;

/** Maximum byte size of the document body store. */
#define DOCBODY_LEN 4096

/** Document body — present when kind == ENTITY_KIND_DESIGN_NOTE / SECTION. */
typedef struct {
    char body[DOCBODY_LEN]; /**< free-form body text (YAML "body:") */
} DocumentBodyComponent;

/** Maximum byte size of the traceability link store. */
#define TRACE_STORE_LEN 4096

/**
 * Traceability component — outgoing relation links on any entity.
 *
 * Records N:M directed relations from this entity to other entities or
 * artefacts.  Entries are stored as a newline-separated flat string where
 * each entry is "target_id\trelation_type":
 *
 *   "REQ-SYS-005\tderived-from\nTC-SW-001\tverified-by"
 *
 * Relation types are free-form strings (e.g. "implements", "verifies",
 * "refines", "derived-from").  Any entity can carry this component.
 *
 * YAML key: "traceability" — sequence of {id|artefact, relation} mappings:
 *
 *   traceability:
 *     - id: REQ-SYS-005
 *       relation: derived-from
 *     - artefact: src/auth/login.c
 *       relation: implemented-in
 *
 * Integrates with TripletStore: each entry maps directly to a
 * (entity_id, relation_type, target_id) triple without duplication
 * (see entity_traceability_to_triplets() in yaml_simple.h).
 */
typedef struct {
    char entries[TRACE_STORE_LEN]; /**< "\n"-separated "target\trelation" pairs */
    int  count;                    /**< number of traceability links stored      */
} TraceabilityComponent;

/* -----------------------------------------------------------------------
 * Entity — the unified record that replaces the old Requirement struct.
 * Sparse components are zero-initialised when not applicable.
 * --------------------------------------------------------------------- */

/**
 * Unified entity record combining all ECS components.
 *
 * Mandatory components:
 *   - identity   — always populated (id, title, kind, …)
 *   - lifecycle  — populated for most entity kinds
 *   - text       — description / rationale
 *   - tags       — optional tag list
 *
 * Sparse / optional components (zero if absent):
 *   - user_story           — any entity can become a user story by carrying this
 *   - acceptance_criteria  — any entity can carry acceptance criteria
 *   - epic_membership      — any entity can belong to an epic
 *   - assumption           — any entity carrying an "assumption:" mapping
 *   - constraint           — any entity carrying a "constraint:" mapping
 *   - doc_body             — kind == ENTITY_KIND_DESIGN_NOTE / SECTION
 *   - traceability         — any entity carrying a "traceability:" sequence
 */
typedef struct {
    IdentityComponent          identity;
    LifecycleComponent         lifecycle;
    TextComponent              text;
    TagComponent               tags;

    /* Optional / sparse components */
    UserStoryComponent         user_story;
    AcceptanceCriteriaComponent acceptance_criteria;
    EpicMembershipComponent    epic_membership;
    AssumptionComponent        assumption;
    ConstraintComponent        constraint;
    DocumentBodyComponent      doc_body;
    TraceabilityComponent      traceability;
} Entity;

/** Dynamic array of Entity records (mirrors RequirementList). */
typedef struct {
    Entity *items;    /**< heap-allocated array of entities */
    int     count;    /**< number of valid entries          */
    int     capacity; /**< allocated capacity               */
} EntityList;

/* -----------------------------------------------------------------------
 * EntityList lifecycle API
 * --------------------------------------------------------------------- */

/** Initialise an empty EntityList. Must be called before first use. */
void entity_list_init(EntityList *list);

/**
 * Append a copy of *entity to list, growing the backing array if needed.
 *
 * Returns  0 on success.
 * Returns -1 on allocation failure (list is unmodified).
 */
int entity_list_add(EntityList *list, const Entity *entity);

/** Free the backing array and reset all fields to zero. */
void entity_list_free(EntityList *list);

/* -----------------------------------------------------------------------
 * Kind helper
 * --------------------------------------------------------------------- */

/**
 * Map a YAML "type" string to the corresponding EntityKind.
 *
 * Recognised mappings:
 *   "functional", "non-functional", "nonfunctional", ""  → ENTITY_KIND_REQUIREMENT
 *   "group"                                              → ENTITY_KIND_GROUP
 *   "story", "user-story"                                → ENTITY_KIND_STORY
 *   "design-note", "design_note", "design"               → ENTITY_KIND_DESIGN_NOTE
 *   "section"                                            → ENTITY_KIND_SECTION
 *   "assumption"                                         → ENTITY_KIND_ASSUMPTION
 *   "constraint"                                         → ENTITY_KIND_CONSTRAINT
 *   "test-case", "test_case", "test"                     → ENTITY_KIND_TEST_CASE
 *   "external", "directive", "standard", "regulation"    → ENTITY_KIND_EXTERNAL
 *   anything else                                        → ENTITY_KIND_UNKNOWN
 *
 * @param type_str  value of the YAML "type" key (may be NULL or empty)
 * @return EntityKind value
 */
EntityKind entity_kind_from_string(const char *type_str);

/**
 * Return a stable, human-readable label for an EntityKind value.
 *
 * @param kind  EntityKind to describe
 * @return pointer to a static string; never NULL
 */
const char *entity_kind_label(EntityKind kind);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VIBE_ENTITY_H */
