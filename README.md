# vibe-requirements

[![CI](https://github.com/jimhansson/vibe-requirements/actions/workflows/ci.yml/badge.svg)](https://github.com/jimhansson/vibe-requirements/actions/workflows/ci.yml)

**vibe-requirements** is a "requirements as code" tool that stores all project
requirements, user stories, test cases, assumptions, constraints, and design
documents as plain YAML files inside a version-controlled repository.  A
companion CLI provides listing, filtering, traceability, coverage, and
orphan-detection commands.

---

## Table of Contents

- [Toolchain Requirements](#toolchain-requirements)
- [Quick Start](#quick-start)
- [CLI Reference](#cli-reference)
  - [Default: list requirements (legacy)](#default-list-requirements-legacy)
  - [list / entities](#list--entities)
  - [doc \<doc-id\>](#doc-doc-id)
  - [trace \<id\>](#trace-id)
  - [coverage](#coverage)
  - [orphan](#orphan)
  - [links](#links)
  - [Global options](#global-options)
- [YAML File Format](#yaml-file-format)
  - [Requirement](#requirement)
  - [User Story](#user-story)
  - [Assumption](#assumption)
  - [Constraint](#constraint)
  - [Test Case](#test-case)
  - [Document (SRS / SDD)](#document-srs--sdd)
  - [Document Schema](#document-schema)
  - [Traceability links](#traceability-links)
- [ECS Architecture](#ecs-architecture)
  - [Entity kinds](#entity-kinds)
  - [Component types](#component-types)
- [Documentation](#documentation)

---

## Toolchain Requirements

All sources are compiled as **C++23** (`-std=c++23`).

| Component | Minimum version | Notes |
|---|---|---|
| C++ compiler | GCC ≥ 13 / Clang ≥ 17 / MSVC 19.38+ | Full C++23 language support required |
| libyaml | any recent release | Runtime YAML parsing |
| Google Test / Google Mock | 1.12+ | Unit tests only |
| GNU Make | 4.x | Build system |

**Ubuntu / Debian** (the CI target):

```bash
sudo apt-get install g++ make libyaml-dev libgtest-dev libgmock-dev
```

GCC 13 ships in Ubuntu 24.04 LTS and Ubuntu 23.10. On older Ubuntu releases
install it via the `ubuntu-toolchain-r/test` PPA:

```bash
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install g++-13
export CXX=g++-13
```

**macOS** (Homebrew):

```bash
brew install gcc libyaml googletest
export CXX=g++-13   # or g++-14, whichever Homebrew installs
```

**Verify your compiler** supports the required C++23 features before building:

```bash
make check-compiler
```

---

## Quick Start

```bash
# Build
cd src && make

# List all entities in the current directory tree
./vibe-req list

# Show traceability chain for one entity
./vibe-req trace REQ-SW-001

# Check how many requirements are covered by tests / code
./vibe-req coverage

# Find requirements and test cases with no links at all
./vibe-req orphan

# Render one SRS/SDD document together with its member entities
./vibe-req doc SRS-CLIENT-001 --output srs.md

# Get help
./vibe-req --help
```

---

## CLI Reference

```
vibe-req [command] [options] [directory]
```

`directory` defaults to `.` (the current working directory).  The tool
recursively discovers all `.yaml` / `.yml` files that have a top-level `id:`
field; files without one are silently ignored.

### Default: list requirements (legacy)

Running `vibe-req` with no command prints a table of all entities that were
parsed using the legacy `Requirement` struct (id, title, type, status,
priority).

```
$ vibe-req
+------------+-----------------------------------------+--------------+----------+----------+
| ID         | Title                                   | Type         | Status   | Priority |
+------------+-----------------------------------------+--------------+----------+----------+
| REQ-AUTH-001 | The system shall support email …      | functional   | approved | must     |
| REQ-AUTH-002 | The system shall lock accounts …      | functional   | draft    | should   |
+------------+-----------------------------------------+--------------+----------+----------+

Total: 2 requirement(s)
```

### list / entities

`list` (alias `entities`) prints all discovered entities using the ECS model
and supports rich filtering.

```
vibe-req list [--kind <kind>] [--component <comp>] [--status <status>] [--priority <prio>] [directory]
```

**Filter flags:**

| Flag | Description | Example values |
|------|-------------|----------------|
| `--kind <kind>` | Show only entities of this kind | `requirement`, `story`, `test-case`, `assumption`, `constraint`, `design-note`, `section`, `group`, `external`, `document` |
| `--component <comp>` | Show only entities that carry the named ECS component | `user-story`, `acceptance-criteria`, `epic`, `assumption`, `constraint`, `doc-meta`, `doc-membership`, `doc-body`, `traceability`, `tags` |
| `--status <status>` | Show only entities with this lifecycle status | `draft`, `approved`, `deprecated` |
| `--priority <prio>` | Show only entities with this priority | `must`, `should`, `could` |

**Examples:**

```bash
# All entities
vibe-req list

# Only requirements
vibe-req list --kind requirement

# Only user stories
vibe-req list --kind story

# Only approved, must-priority requirements
vibe-req list --kind requirement --status approved --priority must

# All entities that carry a traceability component
vibe-req list --component traceability

# All entities with acceptance criteria
vibe-req list --component acceptance-criteria

# Scan a specific directory
vibe-req list requirements/
```

**Sample output:**

```
+---------------+------------------------------------+-------------+----------+----------+
| ID            | Title                              | Kind        | Status   | Priority |
+---------------+------------------------------------+-------------+----------+----------+
| ASSUM-SYS-001 | Network connectivity is available  | assumption  | draft    |          |
| CONSTR-SEC-001| All external communication must …  | constraint  | approved |          |
| REQ-AUTH-001  | The system shall support email … | requirement | approved | must     |
| STORY-SW-001  | User can log in with email …       | story       | draft    | must     |
+---------------+------------------------------------+-------------+----------+----------+

Total: 4 entities
```

### doc \<doc-id\>

`doc` renders a single document entity together with every entity that is
part of it. Membership is resolved through the same relation graph used for
traceability, so both `documents:` YAML membership and explicit `part-of` /
`contains` links are included.

```bash
vibe-req doc <doc-id> [--format md|html] [--output <file>] [directory]
```

**Examples:**

```bash
# Print a document to stdout as Markdown
vibe-req doc SRS-CLIENT-001

# Write an HTML version of the SDD to a file
vibe-req doc SDD-SYSTEM-001 --format html --output sdd.html

# Scan a specific directory
vibe-req doc SRS-CLIENT-001 requirements/
```

### trace \<id\>

Shows the full traceability chain for a single entity: entity metadata,
outgoing links (declared in the entity's own `traceability:` or `links:`
block), and incoming links (other entities that link to this one).

```
vibe-req trace <id> [directory]
```

**Example:**

```bash
$ vibe-req trace REQ-AUTH-001
Traceability chain for: REQ-AUTH-001
  Kind:   requirement
  Title:  The system shall support email and password authentication
  Status: approved

Outgoing links:
  -[member-of]-> GRP-AUTH-001

Incoming links:
  STORY-SW-001 -[implements]->
```

### coverage

Reports how many requirements have at least one traceability link to a test or
code implementation.  Coverage-indicating relations are:
`verifies`, `verified-by`, `implements`, `implemented-by`, `implemented-in`,
`implemented-by-test`, `tests`, `tested-by`, `satisfies`, `satisfied-by`.

```
vibe-req coverage [directory]
```

**Example:**

```bash
$ vibe-req coverage
Coverage Report
===============
Total requirements:    10
Linked requirements:   7 (70%)
Unlinked requirements: 3 (30%)

Unlinked requirements:
+---------------+------------------------------+--------+
| ID            | Title                        | Status |
+---------------+------------------------------+--------+
| REQ-AUTH-003  | Password reset flow          | draft  |
…
```

### orphan

Lists requirements and test cases that have **no** traceability links in
either direction (neither outgoing nor incoming).

```
vibe-req orphan [directory]
```

**Example:**

```bash
$ vibe-req orphan
Orphaned requirements and test cases (no traceability links):
+---------------+-------------+-------------------+--------+
| ID            | Kind        | Title             | Status |
+---------------+-------------+-------------------+--------+
| REQ-AUTH-003  | requirement | Password reset … | draft  |
+---------------+-------------+-------------------+--------+

Total: 1 orphan(s)
```

### links

Prints all relations parsed from the requirement files as a Subject → Relation → Object table.

```
vibe-req links [--strict-links] [directory]
```

`--strict-links` additionally warns (and exits non-zero) when a known
bidirectional relation is declared in only one direction.

**Example:**

```bash
$ vibe-req links
+---------------+-----------+---------------+----------+
| Subject       | Relation  | Object        | Source   |
+---------------+-----------+---------------+----------+
| STORY-SW-001  | implements| REQ-AUTH-001  | declared |
| REQ-AUTH-001  | member-of | GRP-AUTH-001  | declared |
+---------------+-----------+---------------+----------+

Total: 2 relation(s)
```

### Global options

| Option | Description |
|--------|-------------|
| `-h`, `--help` | Print the help message and exit |
| `--strict-links` | Warn when a known bidirectional relation is only declared in one direction (used with `links`) |
| `directory` | Root directory to scan (default: `.`) |

---

## YAML File Format

All entity files are YAML.  A file may contain multiple YAML documents
separated by `---`.  Only documents with a top-level `id:` field are
processed; all others are silently ignored.

### Requirement

```yaml
id: REQ-SW-001
title: "User authentication"
type: functional          # or: non-functional, requirement
status: approved          # draft | approved | deprecated
priority: must            # must | should | could
owner: product-team
version: "1.0"
description: |
  The system shall authenticate users before granting access to any
  protected resource.
rationale: |
  Prevents unauthorized access in accordance with the project security policy.
tags:
  - security
  - authentication
traceability:
  derived-from: REQ-SYS-005
  verified-by: TC-SW-001
  implemented-in: src/auth/login.c
```

> **Note:** Both `traceability:` and `links:` YAML keys are recognised;
> they are equivalent.

### User Story

```yaml
id: STORY-SW-001
title: "User can log in with email and password"
type: story
status: draft
priority: must
role: registered user
goal: to log in using my email address and password
reason: I can access my personal account and settings
epic: EPIC-AUTH-001
acceptance_criteria:
  - A login form with email and password fields is displayed.
  - Submitting valid credentials redirects the user to the dashboard.
  - Submitting invalid credentials displays a clear error message.
traceability:
  implements: REQ-AUTH-001
```

### Assumption

```yaml
id: ASSUM-SYS-001
title: "Network connectivity is available during normal operation"
type: assumption
status: draft
owner: systems-team
assumption:
  text: |
    The embedded device has a reliable network connection to the backend
    services during all normal operating modes.
  status: open              # open | confirmed | invalid
  source: DOC-DEPLOY-001
```

### Constraint

```yaml
id: CONSTR-SEC-001
title: "All external communication must use TLS 1.3 or higher"
type: constraint
status: approved
constraint:
  text: |
    Every network connection MUST be protected by TLS 1.3 or higher.
  kind: technical           # legal | technical | environmental
  source: SEC-POLICY-001
```

### Test Case

```yaml
id: TC-SW-001
title: "Verify login with valid credentials"
type: test-case
status: draft
priority: must
description: |
  Steps to verify that a registered user can log in successfully.
traceability:
  verifies: REQ-SW-001
```

### Document (SRS / SDD)

```yaml
id: SRS-CLIENT-001
title: "Software Requirements Specification — ClientCorp"
type: document          # also recognised: srs, sdd
doc_meta:
  doc_type: SRS
  version: "1.0"
  client: ClientCorp
  status: approved

---
# Any entity can declare membership in one or more documents
id: REQ-SW-001
type: functional
documents:
  - SRS-CLIENT-001
```

### Document Schema

A `document-schema` entity is a variant-aware blueprint that controls how a
document is assembled from its constituent section entities.  It is separate
from the `document` entity (which holds identity and metadata) — the schema
defines *how* the document is composed and rendered.

```yaml
id: SCHEMA-SRS-ACME-V1
title: "SRS Schema — Acme v1.0"
type: document-schema
status: approved
# Which document or variant this schema applies to
applies-to: SRS-ACME-001
# Customer / product variant identifiers
variant-profile:
  customer: acme
  product: v1.0          # alias: delivery
# Ordered list of section entity IDs to include
composition-profile:
  order:
    - SEC-INTRO
    - SEC-SCOPE
    - SEC-FUNCTIONAL-REQS
    - SEC-NON-FUNCTIONAL-REQS
    - SEC-TRACEABILITY
# Target output format
render-profile:
  format: markdown       # or: html
```

Section entities carry the narrative content via `body:`:

```yaml
id: SEC-INTRO
title: "Introduction"
type: section
status: approved
body: |
  This document specifies the software requirements for the Acme
  product line …
```

List and inspect document schemas with the CLI:

```bash
# List all document-schema entities
vibe-req list --kind document-schema

# Show entities that carry a composition-profile
vibe-req list --component composition-profile

# Show the traceability chain for a schema
vibe-req trace SCHEMA-SRS-ACME-V1
```

See [design/chapter-12-document-composition-system.yaml](design/chapter-12-document-composition-system.yaml)
for the full design documentation.

### Traceability links

Traceability entries may appear under `traceability:` **or** `links:` — both
are parsed identically.  The value is a relation-keyed mapping where each key
is a relation type and each value is either a single target ID (scalar) or a
list of target IDs (sequence):

```yaml
traceability:
  derived-from: REQ-SYS-005
  verified-by:
    - TC-SW-001
    - TC-SW-002
  implemented-in: src/auth/login.c
```

Relation types are free-form strings.  Common conventions:

| Relation | Meaning |
|----------|---------|
| `derives-from` / `derived-from` | This entity is derived from the target |
| `implements` / `implemented-by` | This entity implements the target |
| `verified-by` / `verifies` | This entity is verified by the target |
| `refines` | This entity is a refinement of the target |
| `member-of` | This entity belongs to a group or epic |
| `implemented-in` | This entity is implemented in the artefact |
| `constrained-by` | This entity is constrained by the target |
| `part-of` | This entity belongs to a document (inverse: `contains`) |
| `satisfies` / `satisfied-by` | Coverage-indicating relation |
| `tests` / `tested-by` | Coverage-indicating relation |

> **Document membership via `part-of`:** The `documents:` YAML key is the
> primary way to declare document membership, but the same information is
> also projected into the TripletStore as `(entity_id, "part-of", doc_id)`
> triples by `build_entity_relation_store()`.  This means document membership
> is queryable through the same graph API as all other links.  See the
> [document membership design note](#document-membership-via-part-of) below.

---

## ECS Architecture

`vibe-requirements` uses an Entity-Component-System (ECS) inspired model.
Rather than a single monolithic struct with many optional fields, every entity
is a stable ID that carries a set of optional **components**.  The entire
codebase is pure **C++23**: all component structs use `std::string` and
`std::vector` for their fields, and the entity list is a
`std::vector<Entity>` (`EntityList`).

```
Entity (stable ID: e.g. "REQ-SW-001")
├── IdentityComponent      — always present
├── LifecycleComponent     — present on most entities
├── TextComponent          — description + rationale
├── TagComponent           — optional tag list
├── UserStoryComponent     — role / goal / reason  (sparse)
├── AcceptanceCriteriaComponent — criteria list   (sparse)
├── EpicMembershipComponent  — parent epic ID      (sparse)
├── AssumptionComponent    — assumption text/status (sparse)
├── ConstraintComponent    — constraint text/kind   (sparse)
├── DocumentMetaComponent  — doc type/version/client (sparse)
├── DocumentMembershipComponent — parent doc IDs   (sparse)
├── DocumentBodyComponent  — free-form body text    (sparse)
├── TraceabilityComponent  — outgoing relation links (sparse)
├── SourceComponent        — normative source refs   (sparse)
├── AppliesToComponent     — variant/doc targets for a schema (sparse)
├── VariantProfileComponent — customer/product identifiers  (sparse)
├── CompositionProfileComponent — ordered section list      (sparse)
└── RenderProfileComponent — output format for rendering    (sparse)
```

Traceability links stored in `TraceabilityComponent` are also loaded into a
global **TripletStore** (a subject-predicate-object graph) to support
efficient reverse lookup (e.g. "which entities link *to* this one?").
`DocumentMembershipComponent` entries (`documents:` key) are also projected
into the TripletStore as `part-of` triples so that document membership is
queryable through the same indexed graph API as all other relations.

### Entity kinds

| YAML `type` value | Kind label | Description |
|---|---|---|
| `requirement`, `functional`, `non-functional` | `requirement` | A functional or non-functional requirement |
| `group` | `group` | A named collection of related requirements |
| `story`, `user-story` | `story` | A user story |
| `design-note`, `design` | `design-note` | A free-form design note (SDD content) |
| `section` | `section` | A non-normative document section |
| `assumption` | `assumption` | A recorded assumption |
| `constraint` | `constraint` | A technical, legal, or environmental constraint |
| `test-case`, `test` | `test-case` | An explicit test case |
| `external`, `directive`, `standard`, `regulation` | `external` | An external normative source |
| `document`, `srs`, `sdd` | `document` | A document entity (SRS, SDD, …) |
| `document-schema` | `document-schema` | A schema that controls how a document is assembled (variant, section order, render format) |

### Component types

| Component | YAML key(s) | Fields | Description |
|---|---|---|---|
| `IdentityComponent` | `id`, `title`, `type` | `id`, `title`, `kind`, `type_raw`, `file_path`, `doc_index` | Always present; core identity of every entity |
| `LifecycleComponent` | `status`, `priority`, `owner`, `version` | `status`, `priority`, `owner`, `version` | Workflow metadata |
| `TextComponent` | `description`, `rationale` | `description`, `rationale` | Free-text narrative |
| `TagComponent` | `tags` | `tags` (`std::vector<std::string>`) | Optional tag list |
| `UserStoryComponent` | `role`/`as_a`, `goal`/`i_want`, `reason`/`so_that` | `role`, `goal`, `reason` | User-story "as a / I want / so that" triad |
| `AcceptanceCriteriaComponent` | `acceptance_criteria` | `criteria` (`std::vector<std::string>`) | List of acceptance criteria |
| `EpicMembershipComponent` | `epic` | `epic_id` | Parent epic entity ID |
| `AssumptionComponent` | `assumption` (mapping) | `text`, `status`, `source` | Assumption text, validation status, and source reference |
| `ConstraintComponent` | `constraint` (mapping) | `text`, `kind`, `source` | Constraint wording, category, and source reference |
| `DocumentMetaComponent` | `doc_meta` (mapping) | `title`, `doc_type`, `version`, `client`, `status` | Document-level metadata |
| `DocumentMembershipComponent` | `documents` (sequence) | `doc_ids` (`std::vector<std::string>`) | Parent document entity IDs |
| `DocumentBodyComponent` | `body` | `body` | Free-form body text (design notes, sections) |
| `TraceabilityComponent` | `traceability` or `links` (mapping) | `entries` (`std::vector<std::pair<std::string,std::string>>`) | Outgoing directed relation links |
| `SourceComponent` | `sources` (sequence) | `sources` (`std::vector<std::string>`) | Normative source references (external standards, requirement IDs) |
| `AppliesToComponent` | `applies-to` (scalar or sequence) | `applies_to` (`std::vector<std::string>`) | Document IDs or variant identifiers that a `document-schema` applies to |
| `VariantProfileComponent` | `variant-profile` (mapping) | `customer`, `product` | Customer and product/delivery identifiers for a document variant |
| `CompositionProfileComponent` | `composition-profile` (mapping) | `order` (`std::vector<std::string>`) | Ordered list of section entity IDs defining the document structure |
| `RenderProfileComponent` | `render-profile` (mapping) | `format` | Output format for document rendering (`markdown`, `html`) |

Any component can be attached to any entity kind — for example, a
`requirement` entity can carry a `UserStoryComponent` if it also has `role:`,
`goal:`, and `reason:` fields.

### Document membership via `part-of`

Entities can declare document membership in two ways:

**1. Explicit `documents:` key (primary authoring mechanism)**

```yaml
id: REQ-SW-001
documents:
  - SRS-CLIENT-001
  - SDD-SYS-001
```

This populates `DocumentMembershipComponent` at parse time.  The `--component
doc-membership` filter and the `entity_has_component()` API both rely on this
component.

**2. TripletStore projection (query mechanism)**

`build_entity_relation_store()` automatically calls
`entity_doc_membership_to_triplets()` for every entity, which adds
`(entity_id, "part-of", doc_id)` triples to the global TripletStore.
`triplet_store_infer_inverses()` then adds the matching `(doc_id, "contains",
entity_id)` triple.

This means all standard TripletStore queries work for document membership:

```cpp
// C++ API (triplet_store.hpp)
// Which documents does REQ-SW-001 belong to?
auto docs = store.find_by_subject("REQ-SW-001");
// filter for predicate == "part-of"

// Which entities does SRS-CLIENT-001 contain?
auto members = store.find_by_subject("SRS-CLIENT-001");
// filter for predicate == "contains" (inferred)

// C API wrapper (triplet_store_c.h) — use if calling from C code or
// from modules that predate the C++ migration
CTripleList docs_c = triplet_store_find_by_subject(store, "REQ-SW-001");
// filter for predicate == "part-of"
triplet_store_list_free(&docs_c);
```

**Relation pair: `part-of` ↔ `contains`**

The `part-of` / `contains` pair is registered in the built-in relation-pair
table so `infer_inverses()` always creates the reverse direction automatically.

**Migration note for teams using `part-of` in `traceability:` blocks**

Teams that have already written explicit `traceability: {part-of: SRS-X}`
entries do not need to migrate.  Those entries continue to work and are loaded
into the TripletStore via the regular `entity_traceability_to_triplets()` path.
The `documents:` key is preferred for new content because it also populates the
`DocumentMembershipComponent` (enabling the `--component doc-membership` filter
and `entity_has_component()` predicate).

**Tradeoffs**

| Approach | Pros | Cons |
|---|---|---|
| `documents:` key (ECS component) | Self-contained per entity; fast `entity_has_component()` check; filter CLI support | Not visible in the TripletStore without the projection step |
| `traceability: {part-of: SRS-X}` | Unified with all other links; TripletStore-queryable natively | Does not populate `DocumentMembershipComponent`; `--component doc-membership` filter won't match |
| TripletStore projection (current approach) | Both APIs work; no migration needed; covers both authoring styles | Membership appears twice (component + store); duplication is harmless but may be surprising |

---

## Documentation

- [Requirements](docs/REQUIREMENTS.md) — functional and non-functional requirements, constraints, and roadmap
- [Design (wiki)](https://github.com/jimhansson/vibe-requirements/wiki) — high-level architecture, ECS domain model, and file format specification
- [CLI Reference](docs/CLI.md) — complete CLI command reference
- [Porting Guide](docs/PORTING.md) — migration instructions from pre-C++23 versions for contributors and packagers
- [Scripting](docs/SCRIPTING.md) — research on embedded scripting and programmability approaches
- [Open Questions](docs/OPEN_QUESTIONS.md) — unresolved design decisions
- [AI Integration](docs/AI_INTEGRATION.md) — research and recommendations on MCP server, AI-assisted authoring, and agent skills

### Design Notes

The `design/` directory contains numbered chapters that document the internal architecture as vibe-requirement YAML entities:

| Chapter | File | Topic |
|---|---|---|
| 1 | [chapter-01](design/chapter-01-architecture-overview.yaml) | Architecture Overview |
| 2 | [chapter-02](design/chapter-02-file-format-yaml.yaml) | File Format: YAML |
| 3 | [chapter-03](design/chapter-03-alternative-formats.yaml) | Alternative Formats |
| 4 | [chapter-04](design/chapter-04-core-library-modules.yaml) | Core Library Modules |
| 5 | [chapter-05](design/chapter-05-in-memory-graph-model.yaml) | In-Memory Graph Model |
| 6 | [chapter-06](design/chapter-06-entity-component-model.yaml) | Entity-Component Model |
| 7 | [chapter-07](design/chapter-07-cli-command-design.yaml) | CLI Command Design |
| 8 | [chapter-08](design/chapter-08-implementation-language.yaml) | Implementation Language |
| 9 | [chapter-09](design/chapter-09-memory-layout.yaml) | Memory Layout |
| 10 | [chapter-10](design/chapter-10-internal-state-coupling.yaml) | Internal State Coupling |
| 11 | [chapter-11](design/chapter-11-cpp23-migration.yaml) | C++23 Migration |
| 12 | [chapter-12](design/chapter-12-document-composition-system.yaml) | **Schema-Driven Document Composition** |
