# High-Level Design

## 1. Architecture Overview

The tool discovers requirement files automatically by scanning the entire repository for recognized file extensions (e.g., `.yaml`/`.yml` files containing a top-level `id` field). Projects may optionally add a `.vibe-req.yaml` configuration file at the repository root to provide glob patterns that restrict or extend discovery. The directory layout below is the **recommended convention** produced by `vibe-req init`; it is not enforced:

```
project-repo/
├── requirements/
│   ├── sys/          # System-level requirements (recommended)
│   ├── sw/           # Software requirements (recommended)
│   ├── hw/           # Hardware requirements (recommended)
│   └── safety/       # Safety / regulatory requirements (recommended)
├── stories/          # User stories (recommended)
├── design/           # System design documents (SDD) (recommended)
├── tests/            # Test cases (recommended)
├── external/         # Normative references (standards, directives) (recommended)
└── .vibe-req.yaml    # Optional configuration (glob patterns, ID prefix, schema version, …)

          ┌──────────────────────────────────────┐
          │          vibe-req  (binary)          │
          │                                      │
          │  ┌─────────┐  ┌──────────────────┐   │
          │  │   CLI   │  │   GUI (optional) │   │
          │  └────┬────┘  └────────┬─────────┘   │
          │       │                │             │
          │  ┌────▼────────────────▼──────────┐  │
          │  │         Core Library           │  │
          │  │  parser · validator · linker   │  │
          │  │  reporter · tracer             │  │
          │  └────────────────────────────────┘  │
          └──────────────┬───────────────────────┘
                         │ reads / writes
                    ┌────▼────┐
                    │  files  │  (YAML, …)
                    └─────────┘
```

## 2. File Format: YAML (Primary)

Related requirements: REQ-002, REQ-007, REQ-008.

A requirement file (e.g., `requirements/sw/REQ-SW-001.yaml`):

```yaml
id: REQ-SW-001
title: "User authentication"
type: functional
status: approved
priority: high
description: |
  The system shall authenticate users before granting access to any
  protected resource.
rationale: |
  Prevents unauthorized access in accordance with the project security policy.
verification: test
tags:
  - security
  - authentication
sources:
  - external: EU-2016-679:article:32   # GDPR Article 32
links:
  - id: REQ-SW-002
    relation: parent
  - id: REQ-SYS-005
    relation: derives-from
  - artefact: src/auth/login.c
    relation: implemented-in
  - artefact: tests/auth/test_login.c
    relation: verified-by
```

The canonical format for an external source reference in any requirement file is:

```
<EXT-ID>:<section-type>:<section-number>
```

Where `<section-type>` is one of `clause`, `article`, `annex`, or `section`, and `<EXT-ID>` is the `id` field of the corresponding external normative source document. Example: `EU-2016-679:article:32`, `EN-ISO-13849-2023:clause:4.5.2`, `EXT-MACH-DIR:annex:I-1.1.2`.

An external normative source file (`external/EU-Machinery-Dir-2006-42-EC.yaml`):

```yaml
id: EXT-MACH-DIR
title: "EU Machinery Directive 2006/42/EC"
type: directive
issuer: "European Parliament and of the Council"
year: 2006
clauses:
  - id: annex-I-1.1.2
    title: "Principles of safety integration"
    summary: |
      Machinery must be designed and constructed so that it is fitted for
      its function and can be operated, adjusted, and maintained without
      putting persons at risk.
```

A test case file (e.g., `tests/TC-SW-001.yaml`):

```yaml
id: TC-SW-001
title: "User authentication with valid credentials"
description: |
  Verify that the system grants access to a protected resource when
  a user provides valid credentials.
preconditions:
  - A registered user account with username "testuser" exists.
  - The protected resource endpoint is available.
steps:
  - step: 1
    action: "Submit a login request with username 'testuser' and the correct password."
    expected_output: "The system returns an authentication token and HTTP 200."
  - step: 2
    action: "Use the authentication token to request the protected resource."
    expected_output: "The system returns the resource content and HTTP 200."
expected_result: "The user gains access to the protected resource."
verification: test
status: approved
links:
  - id: REQ-SW-001
    relation: verifies
```

YAML files may also contain multiple entities in one physical file using YAML document separators:

```yaml
---
id: REQ-SW-001
title: "User authentication"
type: functional
status: approved
priority: high
description: |
  The system shall authenticate users before granting access to protected resources.
---
id: REQ-SW-002
title: "Session timeout"
type: non-functional
status: draft
priority: medium
description: |
  The system shall expire inactive sessions after a configurable timeout.
```

For multi-document YAML, each YAML document maps to exactly one entity.

## 3. Alternative Format Candidates

| Format | Pros | Cons |
|---|---|---|
| **YAML** | Widely known, good tooling, human-readable | Indentation-sensitive, complex edge cases |
| **TOML** | Simpler syntax, unambiguous, good for flat structures | Less natural for nested/long text blocks |
| **S-expressions** | Trivial to parse in Common Lisp, extensible | Unfamiliar to most engineers |
| **Markdown + front-matter** | Requirements as prose documents, renders on GitHub | Harder to parse structured fields reliably |
| **Custom DSL** | Full control over syntax | Maintenance burden, no existing tooling |

The initial implementation uses YAML. The parser layer shall be abstracted so that additional formats can be added by implementing a format-specific reader/writer module.

## 4. Core Library Modules

| Module | Responsibility |
|---|---|
| `parser` | Reads and deserializes requirement files from disk; format-pluggable |
| `validator` | Checks schema correctness, required fields, unique IDs, and link integrity |
| `linker` | Builds the full link graph from all files in the repository |
| `reporter` | Renders reports (Markdown, HTML, plain text) from the in-memory model |
| `tracer` | Traverses the link graph to produce traceability chains |
| `exporter` | Converts the model to other representations (CSV, ReqIF, …) |

## 5. In-Memory Graph Model (Triplet Store)

To support fast traceability queries and GUI editing, the core model should maintain an in-memory triplet store:

```
(subject, predicate, object)
```

Examples:

```
(REQ-SW-001, derives-from, REQ-SYS-005)
(REQ-SW-001, implemented-in, src/auth/login.c)
(TC-SW-001, verifies, REQ-SW-001)
(REQ-SW-001, cites, EU-2016-679:article:32)
```

Suggested internal structures:

- `triples: Vec<Triple>` (or equivalent)
- `by_subject: HashMap<EntityId, Vec<TripleId>>`
- `by_object: HashMap<EntityId, Vec<TripleId>>`
- `by_predicate: HashMap<Relation, Vec<TripleId>>`

This keeps write operations simple while enabling efficient traversal for `trace`, `lint`, and GUI graph views.

### 5.1 Link Mutation API (GUI + CLI)

Related requirements: REQ-066, REQ-068, REQ-069.

The core library should expose link mutation operations instead of letting GUI/CLI modify file models directly:

```
add_link(subject, relation, object) -> Result<LinkId, Error>
remove_link(link_id) -> Result<(), Error>
remove_links(filter) -> Result<usize, Error>
```

Write-through variants for immediate persistence to disk:

```
add_link_and_flush(subject, relation, object) -> Result<LinkId, Error>
remove_link_and_flush(link_id) -> Result<(), Error>
remove_links_and_flush(filter) -> Result<usize, Error>
```

Required behavior:

- Validate entity existence before insertion.
- Reject exact duplicate links unless relation explicitly allows multiplicity.
- Enforce relation constraints (`verifies` should typically originate from test-case entities, etc.).
- Preserve consistency between in-memory graph and per-file object model.
- In write-through mode, update YAML files atomically as part of the same operation.

### 5.2 Persistence Strategy

Related requirements: REQ-066, NFR-006.

For deterministic file output and clean diffs:

- Keep the graph as the working model during editing.
- Default mode: write-through (every accepted mutation is persisted immediately).
- Optional future mode: delayed/batched writes with explicit `flush()`.
- Sort links by `(relation, target)` before writing.
- Avoid rewriting untouched files.

### 5.3 Entity Origin Tracking (File Provenance)

Related requirements: REQ-007, REQ-067.

Every loaded entity must carry provenance metadata so the system knows exactly which file to patch when data changes.

Suggested metadata:

```
EntityOrigin {
  entity_id: EntityId,
  file_path: RepoRelativePath,
  document_index: u32,          // 0-based position in YAML stream
  doc_kind: requirement | test_case | external_source | story | design_doc,
  format: yaml,
  loaded_revision: u64
}
```

Indexes:

- `origin_by_entity: HashMap<EntityId, EntityOrigin>`
- `entities_by_file: HashMap<RepoRelativePath, Vec<EntityId>>`
- `entities_by_file_doc: HashMap<(RepoRelativePath, u32), EntityId>`
- `file_revision: HashMap<RepoRelativePath, u64>`

Rules:

- Each `EntityId` maps to exactly one owning YAML document in one file.
- Cross-file links are allowed; ownership of the link record is the subject entity's file.
- For multi-document files, ownership of a link record is the subject entity's YAML document.
- Rename/move operations must update origin indexes transactionally.

### 5.4 Repository API (Write-Through First)

Related requirements: REQ-066, REQ-067, REQ-068, REQ-069, REQ-007.

Expose a repository-level API that combines in-memory mutation and disk persistence in one call:

```
update_component_and_flush(entity_id, component_patch) -> Result<(), Error>
remove_component_and_flush(entity_id, component_type) -> Result<(), Error>
add_link_and_flush(subject, relation, object) -> Result<LinkId, Error>
remove_link_and_flush(link_id) -> Result<(), Error>
```

Execution model for each call:

1. Resolve origin file(s) for affected entities.
2. Validate schema + relation constraints against current graph.
3. Apply mutation in memory.
4. Serialize only affected entity document(s).
5. Atomically write to disk (temp file + rename).
6. Reindex provenance and graph indexes.

In multi-document YAML files, step 4 means replacing only the affected YAML document node in the in-memory file representation before writing the full file atomically.

Failure policy:

- If disk write fails, roll back the in-memory mutation.
- Return structured errors with file path and operation context.
- Never leave memory and disk diverged after a failed write-through call.

## 6. Entity-Component-Inspired Domain Model

An ECS-inspired model can separate common, regular fields from irregular, type-specific fields without deep inheritance trees.

### 6.1 Entity

`Entity` is a stable ID (`REQ-SW-001`, `TC-SW-001`, `EXT-MACH-DIR`, etc.).

### 6.2 Common Components (Regular Size)

Store predictable fields in dense component tables:

- `IdentityComponent` (`id`, `title`, `kind`)
- `LifecycleComponent` (`status`, `priority`, `owner`, `version`)
- `TextComponent` (`description`, `rationale`)
- `TagComponent` (`tags`)
- `SourceComponent` (`sources`)

These components cover most entities and are suitable for cache-friendly storage and batch queries.

### 6.3 Irregular Components (Variable Size)

Store sparse or large data in separate, optional components:

- `AssumptionComponent` (`text`, `status`, `source`) — any entity may carry this
- `ConstraintComponent` (`text`, `kind`, `source`) — any entity may carry this; `kind` is e.g. `legal`, `technical`, `environmental`
- `DocumentMetaComponent` (`title`, `doc_type`, `version`, `client`, `status`) — any entity representing a document (SRS, SDD, …) may carry this
- `DocumentMembershipComponent` (`doc_ids`, `count`) — attaches any entity to one or more document entities
- `TestProcedureComponent` (`preconditions`, `steps`, `expected_result`)
- `ClauseCollectionComponent` (external standard clauses/annexes/articles)
- `DocumentBodyComponent` (long free-form markdown/text blocks)
- `AttachmentComponent` (references to binary or generated artifacts)

Both `AssumptionComponent` and `ConstraintComponent` are pure ECS components: no specialised entity type is required to carry them.  Any entity (requirement, story, design note, …) may have one or both attached.  Links to documents or other requirements are handled via the shared relation component (`links:`).

`DocumentMetaComponent` and `DocumentMembershipComponent` implement the ECS document model: a document (SRS, SDD, etc.) is just an entity with `ENTITY_KIND_DOCUMENT`.  Its properties are expressed via `DocumentMetaComponent`; any other entity declares membership in one or more documents via `DocumentMembershipComponent`.  This naturally supports multiple SRS per client and N:M entity-to-document relationships without a rigid type hierarchy.

**YAML schema:**

```yaml
# A document entity
id: SRS-CLIENT-001
title: SRS for Client Project
type: document          # also: srs, sdd
doc_meta:
  doc_type: SRS
  version: 1.0
  client: ClientCorp
  status: approved

---
# Any entity can declare membership in one or more documents
id: REQ-SW-001
type: functional
documents:
  - SRS-CLIENT-001
  - SDD-SYS-001
```

> **Schema note:** The earlier `AssumptionComponent` included a `risk_if_false` field.  Under the new schema that information belongs either in the `text` of a dedicated risk entity or in a linked document referenced via `source`.  The previous flat YAML keys (`statement:`, `risk_if_false:`, `constraint_type:`) are replaced by nested `assumption:` and `constraint:` mapping nodes.

This separation keeps the hot path small while allowing rich per-entity data.

### 6.4 Traceability Component

Traceability is an ECS component attached to any entity — not a separate `TraceabilityLink` entity type.

```c
TraceabilityComponent {
    entries: flat string of "\n"-separated "target_id\trelation_type" pairs
    count:   number of outgoing links
}
```

**YAML key:** `traceability` — a sequence of `{id|artefact, relation}` mappings:

```yaml
traceability:
  - id: REQ-SYS-005
    relation: derived-from
  - id: TC-SW-001
    relation: verified-by
  - artefact: src/auth/login.c
    relation: implemented-in
```

**Relation types** are free-form strings (e.g. `"implements"`, `"verifies"`, `"refines"`, `"derived-from"`).

**N:M relationships** are natural because any entity can carry the component and each entry points to any target entity or artefact.

**Integration with TripletStore:** The component and the triplet store serve complementary roles that avoid duplication:

- `TraceabilityComponent` on an `Entity` is the per-entity view populated during YAML parsing.  It is a fixed-size, stack-allocatable struct (no heap).
- `TripletStore` is the global indexed view.  Each entry in `TraceabilityComponent` maps directly to a `(entity_id, relation_type, target_id)` triple.
- `entity_traceability_to_triplets(entity, store)` loads the component's entries into the store as triples; the store deduplicates automatically.
- Queries that need global lookup (e.g. "find everything verified by TC-SW-001") are served by `triplet_store_find_by_object()`; per-entity forward reads are served by the component directly.

### 6.5 Source Component

`SourceComponent` is an ECS component that records normative source references associated with any entity — external standards, regulations, requirement IDs, or other document citations.

```c
SourceComponent {
    sources: flat newline-separated string of source references
    count:   number of sources stored
}
```

**YAML key:** `sources` — a sequence of scalars or mappings:

```yaml
sources:
  - external: EU-2016-679:article:32   # GDPR Article 32
  - id: REQ-SYS-005
  - EN-ISO-13849-2023:clause:4.5.2     # plain scalar
```

For mapping items, the value of the first key-value pair found is extracted and stored, regardless of the key name (`external`, `id`, etc.).  Plain scalar items are stored as-is.

Any entity kind can carry this component.

### 6.6 Why ECS-Like Here

- Prevents monolithic requirement structs with many optional fields.
- Makes it easier to add new entity kinds without schema rewrites.
- Supports GUI features (property panels and graph views) by querying only needed components.
- Helps separate regular and irregular data for better memory behavior.

### 6.7 ECS Mutation API (Disk-Synchronized)

Related requirements: REQ-066, REQ-067, REQ-068.

Component mutation APIs should mirror triplet write-through guarantees:

```
set_component_and_flush(entity_id, component) -> Result<(), Error>
patch_component_and_flush(entity_id, patch) -> Result<(), Error>
clear_component_and_flush(entity_id, component_type) -> Result<(), Error>
```

Constraints:

- Component updates must validate against entity kind and schema version.
- Updates that affect generated links/sources must rebuild related indexes before write.
- GUI should call only these repository APIs, not direct component table setters.

## 7. CLI Command Design

```
vibe-req [command] [options] [directory]
```

`directory` defaults to `.` (the current working directory).  The tool
recursively scans `.yaml` / `.yml` files with a top-level `id:` field.

### Implemented commands

| Command | Description |
|---|---|
| *(default)* | List all entities using the legacy Requirement struct (id, title, type, status, priority) |
| `list` | List all entities (ECS model) with optional filters. Alias: `entities` |
| `trace <id>` | Show entity metadata, outgoing links, and incoming links for a given ID |
| `coverage` | Report how many requirements have traceability links to tests or code |
| `orphan` | List requirements and test cases with no traceability links in either direction |
| `links` | Print all relations as a Subject → Relation → Object table |

### Filter flags (for `list` / `entities`)

```
vibe-req list [--kind <kind>] [--component <comp>] [--status <status>] [--priority <prio>]
```

| Flag | Values |
|---|---|
| `--kind <kind>` | `requirement`, `group`, `story`, `design-note`, `section`, `assumption`, `constraint`, `test-case`, `external`, `document` |
| `--component <comp>` | `user-story`, `acceptance-criteria`, `epic`, `assumption`, `constraint`, `doc-meta`, `doc-membership`, `doc-body`, `traceability`, `tags` |
| `--status <status>` | e.g. `draft`, `approved`, `deprecated` |
| `--priority <prio>` | e.g. `must`, `should`, `could` |

### Other options

| Option | Description |
|---|---|
| `--strict-links` | Used with `links`: warn when a known bidirectional relation is only declared in one direction; exits non-zero if any warnings are found |
| `-h`, `--help` | Print usage and exit |

### Planned / future commands

| Command | Description |
|---|---|
| `init` | Initialize a new vibe-req project in the current directory |
| `new <type> <id>` | Create a new requirement / story / SDD / test-case / external-source file |
| `validate` | Validate all files: schema, IDs, links |
| `status` | Print counts by status and priority |
| `report` | Generate a Markdown / HTML requirements report |
| `export <format>` | Export to CSV or ReqIF |

## 8. Implementation Language Trade-offs

| Language | Distribution | Ecosystem | Known to author | Suitability |
|---|---|---|---|---|
| **C / C++** | Single binary | libyaml / RapidYAML; GTK / Qt for GUI | Yes | Good; GUI is straightforward with Qt |
| **Common Lisp** | Single binary (SBCL `--save-lisp-and-die`) | cl-yaml; McCLIM for GUI | Yes | Excellent for DSL; GUI ecosystem is limited |
| **Rust** | Single static binary | `serde_yaml`; `egui` / GTK for GUI | To explore | Excellent distribution story; memory safety |
| **Go** | Single static binary | `gopkg.in/yaml.v3`; Fyne / `gio` for GUI | To explore | Very easy cross-compilation; moderate GUI |

**Recommendation for evaluation:** Prototype the core parser and CLI in both Rust and Go, compare ergonomics and binary size, then decide. The GUI can be deferred to a later phase and use whichever GUI toolkit fits the chosen language.
