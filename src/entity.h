/**
 * @file entity.h
 * @brief ECS-inspired entity model for vibe-req.
 *
 * ## Overview
 *
 * Every artefact managed by vibe-req (requirement, user story, test case,
 * assumption, design note, …) is represented as an @c Entity — a single
 * struct that aggregates all optional ECS components.  Parsing one YAML
 * document produces exactly one @c Entity; the entity's @c identity.id
 * field is the stable, global primary key used in all traceability links.
 *
 * ## Memory Layout Rationale
 *
 * Components fall into two memory classes:
 *
 * **Fixed-size (inline) components** — stored directly inside the Entity
 * struct as char arrays.  These cover all fields that are present on the
 * vast majority of entities (identity, lifecycle, text, tags, traceability,
 * sources, user-story, acceptance-criteria, assumption, constraint, …).
 * Keeping them inline means the entire Entity can be stack-allocated or
 * placed in a contiguous heap array without any pointer chasing, which
 * makes batch operations (sort, filter, copy) cache-friendly.
 *
 * The array sizes are chosen conservatively:
 *   - Most IDs fit in 64 bytes; titles in 256 bytes.
 *   - The traceability store (@c TRACE_STORE_LEN = 4096) supports roughly
 *     40–80 links per entity before truncation.
 *   - The tag store (@c TAG_STORE_LEN = 1024) holds roughly 20–50 tags.
 *
 * **Heap-allocated (pointer) components** — used only for components that
 * are both rare and potentially large:
 *   - @c DocumentBodyComponent  (free-form body text, up to @c DOCBODY_LEN)
 *   - @c TestProcedureComponent (preconditions + steps + expected result)
 *   - @c ClauseCollectionComponent (external standard clause list)
 *   - @c AttachmentComponent   (attachment path/description pairs)
 *
 * A NULL pointer in any of these components means "absent" and costs
 * nothing.  Always call @c entity_free() to release heap storage, or use
 * @c entity_list_free() to release an entire @c EntityList at once.
 *
 * ## ECS Design Decisions
 *
 * The ECS-like approach was chosen over a monolithic struct or a deep
 * inheritance tree for three reasons:
 *
 *   1. **Extensibility without schema rewrites.**  Adding a new component
 *      type (e.g. @c RiskComponent) requires adding one new struct and one
 *      new field to @c Entity — no existing code paths change.
 *
 *   2. **Sparse data without waste.**  Optional components that are absent
 *      on most entities are represented as zero-initialised inline fields
 *      (pointer components are NULL).  There is no vtable or type-tag
 *      overhead per entity.
 *
 *   3. **Uniform API.**  @c entity_has_component(), @c entity_apply_filter(),
 *      and @c entity_copy() work generically across all component types
 *      without special-casing each kind.
 *
 * ## Integration Points
 *
 *   - **Parsing:**     @c yaml_simple.h exposes @c yaml_parse_entity() and
 *                      @c yaml_parse_entities() which populate Entity structs
 *                      from YAML files.
 *   - **Traceability:** @c entity_traceability_to_triplets() (declared in
 *                       yaml_simple.h) loads TraceabilityComponent entries
 *                       into a @c TripletStore for indexed graph queries.
 *   - **Reporting:**   @c report.h consumes @c EntityList to render Markdown
 *                      or HTML output.
 *   - **Discovery:**   @c discovery.h recursively collects all entities from
 *                      a directory tree into an @c EntityList.
 */

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
 * Most fields are fixed-size char arrays.  Large optional components
 * (DocumentBodyComponent, TestProcedureComponent, ClauseCollectionComponent,
 * AttachmentComponent) use heap-allocated char * pointers so that entities
 * that do not carry those components consume no extra memory.  NULL means
 * "absent"; call entity_free() to release any heap storage.
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

/**
 * Document body — present when kind == ENTITY_KIND_DESIGN_NOTE / SECTION.
 *
 * @note body is heap-allocated (via malloc/strdup).  NULL means absent.
 *       Call entity_free() to release.
 */
typedef struct {
    char *body; /**< free-form body text (YAML "body:"); heap-alloc'd, NULL if absent */
} DocumentBodyComponent;

/** Maximum byte size of the source reference store. */
#define SOURCE_STORE_LEN 2048

/**
 * Source component — normative source references on any entity.
 *
 * Records one or more source references (external standards, regulations,
 * requirement IDs, or document citations) associated with this entity.
 * Entries are stored as a newline-separated flat string.
 *
 * YAML key: "sources" — a sequence of scalars or mappings:
 *
 *   sources:
 *     - external: EU-2016-679:article:32
 *     - id: REQ-SYS-001
 *
 * For mapping items the value of the first key-value pair found is stored,
 * regardless of the key name (e.g. `external`, `id`, etc.).
 * Plain scalar items are stored as-is.
 *
 * Any entity can carry this component.
 */
typedef struct {
    char sources[SOURCE_STORE_LEN]; /**< newline-separated list of source references */
    int  count;                      /**< number of sources stored                    */
} SourceComponent;

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
 *
 * @note preconditions, steps, and expected_result are heap-allocated (NULL if
 *       absent).  Call entity_free() to release.
 */
typedef struct {
    char *preconditions;   /**< newline-separated pre-conditions; heap-alloc'd, NULL if absent */
    int   precondition_count; /**< number of pre-conditions stored                             */
    char *steps;           /**< newline-separated "action\texpected_output" pairs; heap-alloc'd */
    int   step_count;      /**< number of steps stored                                          */
    char *expected_result; /**< overall expected result; heap-alloc'd (strdup), NULL if absent  */
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
 * Note: the "summary" field is not stored in the component.
 *
 * @note clauses is heap-allocated (NULL if absent).  Call entity_free() to
 *       release.
 */
typedef struct {
    char *clauses; /**< newline-separated "id\ttitle" pairs; heap-alloc'd, NULL if absent */
    int   count;   /**< number of clauses stored                                           */
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
 *
 * @note attachments is heap-allocated (NULL if absent).  Call entity_free()
 *       to release.
 */
typedef struct {
    char *attachments; /**< newline-separated "path\tdescription" pairs; heap-alloc'd, NULL if absent */
    int   count;       /**< number of attachments stored                                               */
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
 * Large optional components (doc_body, test_procedure, clause_collection,
 * attachment) use heap-allocated char * pointers; NULL means absent.
 * Always call entity_free() to release heap resources, or let
 * entity_list_free() do it for all entities in a list.
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
 * Sparse / optional components (zero/NULL if absent):
 *   - user_story           — any entity can become a user story by carrying this
 *   - acceptance_criteria  — any entity can carry acceptance criteria
 *   - epic_membership      — any entity can belong to an epic
 *   - assumption           — any entity carrying an "assumption:" mapping
 *   - constraint           — any entity carrying a "constraint:" mapping
 *   - doc_meta             — any entity representing a document (SRS, SDD, …)
 *   - doc_membership       — any entity belonging to one or more documents
 *   - doc_body             — kind == ENTITY_KIND_DESIGN_NOTE / SECTION (heap)
 *   - traceability         — any entity carrying a "traceability:" sequence
 *   - sources              — any entity carrying a "sources:" sequence
 *   - test_procedure       — any entity carrying preconditions/steps/expected_result (heap)
 *   - clause_collection    — any entity carrying a "clauses:" sequence (heap)
 *   - attachment           — any entity carrying an "attachments:" sequence (heap)
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
    SourceComponent            sources;
    TestProcedureComponent     test_procedure;
    ClauseCollectionComponent  clause_collection;
    AttachmentComponent        attachment;
} Entity;

/** Dynamic array of Entity records. */
typedef struct {
    Entity *items;    /**< heap-allocated array of entities */
    int     count;    /**< number of valid entries          */
    int     capacity; /**< allocated capacity               */
} EntityList;

/* -----------------------------------------------------------------------
 * Entity lifecycle API
 * --------------------------------------------------------------------- */

/**
 * Release heap-allocated fields inside *entity.
 *
 * Frees the heap buffers for doc_body, test_procedure, clause_collection,
 * and attachment components and sets their pointers to NULL.  Does NOT free
 * the entity struct itself — the caller owns that memory.
 *
 * Safe to call on a zero-initialised (memset) entity.
 *
 * @param entity  pointer to the Entity whose heap fields should be freed
 */
void entity_free(Entity *entity);

/**
 * Deep-copy *src into *dst.
 *
 * Performs a shallow copy of all fixed-size fields and a deep copy (strdup)
 * of all heap-allocated fields.  *dst must not already contain live heap
 * pointers; zero-initialise it first (e.g. with memset) or call
 * entity_free() before passing it here.
 *
 * Returns  0 on success.
 * Returns -1 on allocation failure; *dst is freed and zeroed on failure.
 */
int entity_copy(Entity *dst, const Entity *src);

/* -----------------------------------------------------------------------
 * EntityList lifecycle API
 * --------------------------------------------------------------------- */

/** Initialise an empty EntityList. Must be called before first use. */
void entity_list_init(EntityList *list);

/**
 * Append a deep copy of *entity to list, growing the backing array if needed.
 *
 * Heap-allocated fields (doc_body, test_procedure, clause_collection,
 * attachment) are duplicated so that the caller may independently free or
 * reuse *entity after this call returns.
 *
 * Returns  0 on success.
 * Returns -1 on allocation failure (list is unmodified).
 */
int entity_list_add(EntityList *list, const Entity *entity);

/**
 * Free all entities in the list (including their heap fields) and the
 * backing array, then reset all fields to zero.
 */
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
 *   sources
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

/**
 * Comparator for qsort() — orders entities lexicographically by identity.id.
 *
 * @param a  pointer to first Entity (cast from void*)
 * @param b  pointer to second Entity (cast from void*)
 * @return   negative / zero / positive as strcmp semantics
 */
int entity_cmp_by_id(const void *a, const void *b);

/**
 * Build a filtered view of *src into *dst.
 *
 * Caller initialises dst before calling and must call entity_list_free() when
 * done.  Pass NULL (or empty string) for any filter argument to disable that
 * filter — the matching entities are always copied into dst.
 *
 * Filter arguments:
 * @param src              source entity list (read-only)
 * @param dst              destination list (already initialised by caller)
 * @param filter_kind      EntityKind label, e.g. "requirement", "test-case";
 *                         NULL or "" → accept all kinds
 * @param filter_comp      component name, e.g. "traceability", "assumption";
 *                         NULL or "" → accept all
 * @param filter_status    lifecycle status, e.g. "draft", "approved";
 *                         NULL or "" → accept all
 * @param filter_priority  lifecycle priority, e.g. "must", "should";
 *                         NULL or "" → accept all
 */
void entity_apply_filter(const EntityList *src, EntityList *dst,
                         const char *filter_kind,
                         const char *filter_comp,
                         const char *filter_status,
                         const char *filter_priority);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VIBE_ENTITY_H */
