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
    ENTITY_KIND_DOCUMENT,        /**< a document entity (SRS, SDD, …)          */
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

/**
 * Document-meta component — can be attached to any entity that represents
 * a document (SRS, SDD, test-plan, …).
 *
 * Schema: {title, doc_type, version, client, status}
 *   title    — human-readable document title (YAML "doc_meta.title")
 *   doc_type — document category, e.g. "SRS", "SDD", "test-plan"
 *   version  — document version string, e.g. "1.0", "2.3-draft"
 *   client   — client / customer for whom the document is produced
 *   status   — document lifecycle status, e.g. "draft", "approved"
 *
 * YAML key: "doc_meta" — a mapping node:
 *
 *   doc_meta:
 *     doc_type: SRS
 *     version: 1.0
 *     client: ClientCorp
 *     status: approved
 */
typedef struct {
    char title[256];    /**< document title (YAML "doc_meta.title")       */
    char doc_type[64];  /**< document category (YAML "doc_meta.doc_type") */
    char version[32];   /**< document version (YAML "doc_meta.version")   */
    char client[128];   /**< client / customer (YAML "doc_meta.client")   */
    char status[32];    /**< document status (YAML "doc_meta.status")     */
} DocumentMetaComponent;

/** Maximum byte size of the document-membership store. */
#define DOC_MEMBER_STORE_LEN 1024

/**
 * Document-membership component — attaches any entity to one or more
 * document entities.
 *
 * Any entity (requirement, assumption, user story, …) can belong to any
 * number of documents without a rigid type hierarchy.  The list of parent
 * document entity-ids is stored as a newline-separated flat string.
 *
 * YAML key: "documents" — a sequence of entity IDs:
 *
 *   documents:
 *     - SRS-CLIENT-001
 *     - SDD-SYSTEM-001
 */
typedef struct {
    char doc_ids[DOC_MEMBER_STORE_LEN]; /**< newline-separated document entity IDs */
    int  count;                          /**< number of document memberships         */
} DocumentMembershipComponent;

/** Maximum byte size of the document body store. */
#define DOCBODY_LEN 65536

/** Document body — present when kind == ENTITY_KIND_DESIGN_NOTE / SECTION. */
typedef struct {
    char body[DOCBODY_LEN]; /**< free-form body text (YAML "body:") */
} DocumentBodyComponent;

/** Maximum byte size of the test-procedure preconditions store. */
#define TEST_PROC_PRECOND_LEN 2048
/** Maximum byte size of the test-procedure steps store. */
#define TEST_PROC_STEPS_LEN   4096
/** Maximum byte size of the test-procedure expected-result field. */
#define TEST_PROC_RESULT_LEN  1024

/**
 * Test-procedure component — present on test-case entities.
 *
 * Captures the structured procedure of a test case.
 *
 * Schema:
 *   preconditions (sequence)   — list of pre-conditions (newline-separated)
 *   steps (sequence of mappings) — each step has "action" and "expected_output"
 *                                  stored as newline-separated "action\texpected_output" pairs
 *   expected_result (scalar)   — overall expected result of the test
 *
 * YAML keys:
 *   preconditions:
 *     - A registered user account exists.
 *   steps:
 *     - step: 1
 *       action: "Submit login request."
 *       expected_output: "System returns HTTP 200."
 *   expected_result: "The user gains access."
 */
typedef struct {
    char preconditions[TEST_PROC_PRECOND_LEN]; /**< newline-separated pre-conditions       */
    int  precondition_count;                    /**< number of pre-conditions stored        */
    char steps[TEST_PROC_STEPS_LEN];            /**< newline-separated "action\texpected_output" pairs */
    int  step_count;                            /**< number of steps stored                 */
    char expected_result[TEST_PROC_RESULT_LEN]; /**< overall expected result (YAML "expected_result:") */
} TestProcedureComponent;

/** Maximum byte size of the clause-collection store. */
#define CLAUSE_STORE_LEN 8192

/**
 * Clause-collection component — present on external-source entities.
 *
 * Holds the list of clauses, articles, or annexes defined by an external
 * normative document (standard, directive, regulation).  Each clause is
 * stored as a "id\ttitle" pair in a newline-separated flat string.
 *
 * YAML key: "clauses" — a sequence of {id, title, [summary]} mappings:
 *
 *   clauses:
 *     - id: annex-I-1.1.2
 *       title: "Principles of safety integration"
 *       summary: |
 *         Machinery must be designed …
 *
 * Note: the "summary" field is not stored in the fixed-size struct; it is
 * available in the source YAML for human reading but is omitted from the
 * in-memory component to keep the Entity stack-allocatable.
 */
typedef struct {
    char clauses[CLAUSE_STORE_LEN]; /**< newline-separated "id\ttitle" pairs */
    int  count;                      /**< number of clauses stored            */
} ClauseCollectionComponent;

/** Maximum byte size of the attachment store. */
#define ATTACH_STORE_LEN 2048

/**
 * Attachment component — references to binary or generated artefacts.
 *
 * Any entity may carry one or more attachment references (e.g. specification
 * PDFs, generated diagrams, test reports).  Each attachment is stored as a
 * "path\tdescription" pair in a newline-separated flat string.
 *
 * YAML key: "attachments" — a sequence of {path, [description]} mappings:
 *
 *   attachments:
 *     - path: docs/spec.pdf
 *       description: "Original specification document"
 *     - path: images/diagram.png
 *       description: "Architecture overview diagram"
 */
typedef struct {
    char attachments[ATTACH_STORE_LEN]; /**< newline-separated "path\tdescription" pairs */
    int  count;                          /**< number of attachments stored                */
} AttachmentComponent;

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
 *   - doc_meta             — any entity representing a document (SRS, SDD, …)
 *   - doc_membership       — any entity belonging to one or more documents
 *   - doc_body             — kind == ENTITY_KIND_DESIGN_NOTE / SECTION
 *   - traceability         — any entity carrying a "traceability:" sequence
 *   - test_procedure       — any entity carrying preconditions/steps/expected_result
 *   - clause_collection    — any entity carrying a "clauses:" sequence
 *   - attachment           — any entity carrying an "attachments:" sequence
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
    DocumentMetaComponent      doc_meta;
    DocumentMembershipComponent doc_membership;
    DocumentBodyComponent      doc_body;
    TraceabilityComponent      traceability;
    TestProcedureComponent     test_procedure;
    ClauseCollectionComponent  clause_collection;
    AttachmentComponent        attachment;
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
 *   "requirement", "functional", "non-functional", "nonfunctional", "" → ENTITY_KIND_REQUIREMENT
 *   "group"                                              → ENTITY_KIND_GROUP
 *   "story", "user-story"                                → ENTITY_KIND_STORY
 *   "design-note", "design_note", "design"               → ENTITY_KIND_DESIGN_NOTE
 *   "section"                                            → ENTITY_KIND_SECTION
 *   "assumption"                                         → ENTITY_KIND_ASSUMPTION
 *   "constraint"                                         → ENTITY_KIND_CONSTRAINT
 *   "test-case", "test_case", "test"                     → ENTITY_KIND_TEST_CASE
 *   "external", "directive", "standard", "regulation"    → ENTITY_KIND_EXTERNAL
 *   "document", "srs", "sdd"                             → ENTITY_KIND_DOCUMENT
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

/**
 * Test whether a named ECS component is present (non-empty / non-zero) on an
 * entity.
 *
 * Recognised component names:
 *   user-story, user_story
 *   acceptance-criteria, acceptance_criteria
 *   epic, epic-membership, epic_membership
 *   assumption
 *   constraint
 *   doc-meta, doc_meta
 *   doc-membership, documents
 *   doc-body, body
 *   traceability
 *   tags
 *   test-procedure, test_procedure
 *   clause-collection, clause_collection, clauses
 *   attachment, attachments
 *
 * @param entity  pointer to the Entity to inspect (must not be NULL)
 * @param comp    component name string (case-sensitive); NULL or "" → always 1
 * @return 1 if the component is present, 0 if absent or name is unrecognised
 */
int entity_has_component(const Entity *entity, const char *comp);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VIBE_ENTITY_H */
