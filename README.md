# vibe-requirements

[![CI](https://github.com/jimhansson/vibe-requirements/actions/workflows/ci.yml/badge.svg)](https://github.com/jimhansson/vibe-requirements/actions/workflows/ci.yml)

**vibe-requirements** is a "requirements as code" tool that stores all project
requirements, user stories, test cases, assumptions, constraints, and design
documents as plain YAML files inside a version-controlled repository.  A
companion CLI provides listing, filtering, traceability, coverage, and
orphan-detection commands.

---

## Table of Contents

- [Quick Start](#quick-start)
- [CLI Reference](#cli-reference)
  - [Default: list requirements (legacy)](#default-list-requirements-legacy)
  - [list / entities](#list--entities)
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
  - [Traceability links](#traceability-links)
- [ECS Architecture](#ecs-architecture)
  - [Entity kinds](#entity-kinds)
  - [Component types](#component-types)
- [Documentation](#documentation)

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
  - id: REQ-SYS-005
    relation: derived-from
  - id: TC-SW-001
    relation: verified-by
  - artefact: src/auth/login.c
    relation: implemented-in
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
  - id: REQ-AUTH-001
    relation: implements
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
  - id: REQ-SW-001
    relation: verifies
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

### Traceability links

Traceability entries may appear under `traceability:` **or** `links:` — both
are parsed identically.  Each entry is a mapping with either an `id` key (for
entity links) or an `artefact` key (for file/artefact links), plus a
`relation` key:

```yaml
traceability:
  - id: REQ-SYS-005
    relation: derived-from
  - id: TC-SW-001
    relation: verified-by
  - artefact: src/auth/login.c
    relation: implemented-in
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
is a stable ID that carries a set of optional **components**.  Components are
fixed-size C structs so the entire `Entity` can be stack-allocated.

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
└── SourceComponent        — normative source refs   (sparse)
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

### Component types

| Component | YAML key(s) | Fields | Description |
|---|---|---|---|
| `IdentityComponent` | `id`, `title`, `type` | `id`, `title`, `kind`, `type_raw`, `file_path`, `doc_index` | Always present; core identity of every entity |
| `LifecycleComponent` | `status`, `priority`, `owner`, `version` | `status`, `priority`, `owner`, `version` | Workflow metadata |
| `TextComponent` | `description`, `rationale` | `description`, `rationale` | Free-text narrative |
| `TagComponent` | `tags` | `tags` (newline-separated), `count` | Optional tag list |
| `UserStoryComponent` | `role`/`as_a`, `goal`/`i_want`, `reason`/`so_that` | `role`, `goal`, `reason` | User-story "as a / I want / so that" triad |
| `AcceptanceCriteriaComponent` | `acceptance_criteria` | `criteria` (newline-separated), `count` | List of acceptance criteria |
| `EpicMembershipComponent` | `epic` | `epic_id` | Parent epic entity ID |
| `AssumptionComponent` | `assumption` (mapping) | `text`, `status`, `source` | Assumption text, validation status, and source reference |
| `ConstraintComponent` | `constraint` (mapping) | `text`, `kind`, `source` | Constraint wording, category, and source reference |
| `DocumentMetaComponent` | `doc_meta` (mapping) | `title`, `doc_type`, `version`, `client`, `status` | Document-level metadata |
| `DocumentMembershipComponent` | `documents` (sequence) | `doc_ids` (newline-separated), `count` | Parent document entity IDs |
| `DocumentBodyComponent` | `body` | `body` | Free-form body text (design notes, sections) |
| `TraceabilityComponent` | `traceability` or `links` (sequence) | `entries` (newline-separated `target\trelation` pairs), `count` | Outgoing directed relation links |
| `SourceComponent` | `sources` (sequence) | `sources` (newline-separated), `count` | Normative source references (external standards, requirement IDs) |

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

```c
/* Which documents does REQ-SW-001 belong to? */
CTripleList docs = triplet_store_find_by_subject(store, "REQ-SW-001");
/* filter for predicate == "part-of" */

/* Which entities does SRS-CLIENT-001 contain? */
CTripleList members = triplet_store_find_by_subject(store, "SRS-CLIENT-001");
/* filter for predicate == "contains" (inferred) */
```

**Relation pair: `part-of` ↔ `contains`**

The `part-of` / `contains` pair is registered in the built-in relation-pair
table so `infer_inverses()` always creates the reverse direction automatically.

**Migration note for teams using `part-of` in `traceability:` blocks**

Teams that have already written explicit `traceability: [{id: SRS-X, relation: part-of}]`
entries do not need to migrate.  Those entries continue to work and are loaded
into the TripletStore via the regular `entity_traceability_to_triplets()` path.
The `documents:` key is preferred for new content because it also populates the
`DocumentMembershipComponent` (enabling the `--component doc-membership` filter
and `entity_has_component()` predicate).

**Tradeoffs**

| Approach | Pros | Cons |
|---|---|---|
| `documents:` key (ECS component) | Self-contained per entity; fast `entity_has_component()` check; filter CLI support | Not visible in the TripletStore without the projection step |
| `traceability: [{relation: part-of}]` | Unified with all other links; TripletStore-queryable natively | Does not populate `DocumentMembershipComponent`; `--component doc-membership` filter won't match |
| TripletStore projection (current approach) | Both APIs work; no migration needed; covers both authoring styles | Membership appears twice (component + store); duplication is harmless but may be surprising |

---

## Documentation

- [Requirements](docs/REQUIREMENTS.md) — functional and non-functional requirements, constraints, and roadmap
- [Design](docs/DESIGN.md) — high-level architecture, ECS domain model, and file format specification
- [CLI Reference](docs/CLI.md) — complete CLI command reference
- [Scripting](docs/SCRIPTING.md) — research on embedded scripting and programmability approaches
- [Open Questions](docs/OPEN_QUESTIONS.md) — unresolved design decisions
- [AI Integration](docs/AI_INTEGRATION.md) — research and recommendations on MCP server, AI-assisted authoring, and agent skills
