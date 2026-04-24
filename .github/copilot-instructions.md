# GitHub Copilot Instructions — vibe-requirements

This file teaches GitHub Copilot the conventions, YAML schemas, ECS patterns,
and CLI usage of the **vibe-requirements** project so that it can generate
compliant YAML entities and support the project's one-file-per-chapter
authoring style.

---

## 1. Project overview

**vibe-requirements** stores all requirements, user stories, test cases,
assumptions, constraints, and design documents as plain YAML files inside a
version-controlled repository.  The companion CLI (`vibe-req`) provides
listing, filtering, traceability, coverage, and orphan-detection commands.

The underlying data model is **Entity-Component-System (ECS)** inspired: every
artefact is a stable *entity* (a unique ID) that carries a set of optional
*components* (fixed-size C structs).  This avoids a monolithic struct with many
optional fields and allows any component to be attached to any entity kind.

---

## 2. Recommended directory layout

```
project-root/
├── .vibe-req.yaml          # tool config (e.g. ignore_dirs)
├── requirements/
│   ├── sys/                # system-level requirements
│   │   └── REQ-SYS-001.yaml
│   ├── sw/                 # software requirements
│   │   ├── REQ-SW-001.yaml
│   │   └── chapter-02-authentication.yaml   # multi-entity file
│   └── hw/
├── tests/
│   └── TC-SW-001.yaml
├── design/
│   ├── chapter-01-overview.yaml
│   └── chapter-02-architecture.yaml
├── external/
│   └── EXT-MACH-DIR.yaml   # external normative sources
└── docs/
    └── SRS-CLIENT-001.yaml # document entities
```

### Multi-document (one-file-per-chapter) approach

A single YAML file may contain **multiple entities** separated by `---`.  Each
YAML document maps to exactly one entity.  Files without a top-level `id:`
field are silently ignored by the tool.

```yaml
---
id: REQ-SW-010
title: "Login rate limiting"
type: functional
status: draft
priority: must
description: |
  The system shall limit login attempts to five per minute per IP address.
---
id: REQ-SW-011
title: "Account lockout notification"
type: functional
status: draft
priority: should
description: |
  The system shall notify the account owner by email when their account is locked.
traceability:
  - id: REQ-SW-010
    relation: refines
```

Use the one-file-per-chapter style for design notes, grouped requirements, and
chapters of an SRS or SDD.  Use one file per entity for standalone requirements
and test cases that will be individually edited and linked.

---

## 3. Entity kinds and YAML `type` values

| `type` value(s) | Kind | Typical ID prefix |
|---|---|---|
| `requirement`, `functional`, `non-functional` | requirement | `REQ-SW-`, `REQ-SYS-` |
| `group` | group | `GRP-` |
| `story`, `user-story` | story | `STORY-` |
| `design-note`, `design` | design-note | `DESIGN-` |
| `section` | section | `SEC-` |
| `assumption` | assumption | `ASSUM-` |
| `constraint` | constraint | `CONSTR-` |
| `test-case`, `test` | test-case | `TC-` |
| `external`, `directive`, `standard`, `regulation` | external | `EXT-` |
| `document`, `srs`, `sdd` | document | `SRS-`, `SDD-` |

---

## 4. Core YAML schema

Every entity must have at least an `id:` field.  All other fields are optional
but strongly recommended for complete traceability.

### 4.1 Common fields (all entity kinds)

```yaml
id: REQ-SW-001            # required — unique identifier
title: "Short description"# recommended
type: functional          # drives EntityKind (see table above)
status: approved          # draft | approved | deprecated
priority: must            # must | should | could
owner: product-team       # person or team responsible
version: "1.0"            # document/entity version string
description: |            # detailed free-text description
  …
rationale: |              # justification (YAML "rationale:")
  …
tags:                     # optional tag list
  - security
  - authentication
```

### 4.2 Requirement

```yaml
id: REQ-SW-001
title: "User authentication"
type: functional          # or: non-functional, requirement
status: approved
priority: must
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
sources:
  - external: EU-2016-679:article:32   # GDPR Article 32
traceability:
  - id: REQ-SYS-005
    relation: derived-from
  - id: TC-SW-001
    relation: verified-by
  - artefact: src/auth/login.c
    relation: implemented-in
```

### 4.3 User story

```yaml
id: STORY-SW-001
title: "User can log in with email and password"
type: story
status: draft
priority: must
role: registered user          # "As a <role>"
goal: to log in using my email address and password
reason: I can access my personal account and settings
epic: EPIC-AUTH-001            # parent epic entity ID
acceptance_criteria:
  - A login form with email and password fields is displayed.
  - Submitting valid credentials redirects the user to the dashboard.
  - Submitting invalid credentials displays a clear error message.
traceability:
  - id: REQ-AUTH-001
    relation: implements
```

Legacy field names `as_a`, `i_want`, `so_that` are also accepted.

### 4.4 Assumption

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
  source: DOC-DEPLOY-001    # document or entity ID that validates this
```

### 4.5 Constraint

```yaml
id: CONSTR-SEC-001
title: "All external communication must use TLS 1.3 or higher"
type: constraint
status: approved
constraint:
  text: |
    Every network connection MUST be protected by TLS 1.3 or higher.
  kind: technical           # legal | technical | environmental
  source: SEC-POLICY-001    # document or regulation imposing the constraint
```

### 4.6 Test case

```yaml
id: TC-SW-001
title: "Verify login with valid credentials"
type: test-case
status: draft
priority: must
description: |
  Verify that a registered user can log in with a correct email/password.
preconditions:
  - A registered user account with username "testuser" exists.
  - The authentication endpoint is reachable.
steps:
  - step: 1
    action: "Submit a login request with username 'testuser' and the correct password."
    expected_output: "The system returns an authentication token and HTTP 200."
  - step: 2
    action: "Use the token to request a protected resource."
    expected_output: "The system returns the resource and HTTP 200."
expected_result: "The user gains access to the protected resource."
traceability:
  - id: REQ-SW-001
    relation: verifies
```

### 4.7 Design note / section

```yaml
id: DESIGN-CH-2
title: "File Format: YAML"
type: design-note           # or: section
status: approved
tags:
  - file-format
  - yaml
body: |
  A requirement file stores one or more YAML documents. Each document
  maps to a single entity identified by its `id:` field.
traceability:
  - id: REQ-002
    relation: elaborates
```

### 4.8 Group

```yaml
id: GRP-AUTH-001
title: "Authentication Requirements Group"
type: group
status: approved
description: |
  Collects all requirements related to authentication and session management.
```

Member requirements link back to the group:

```yaml
traceability:
  - id: GRP-AUTH-001
    relation: member-of
```

### 4.9 External normative source

```yaml
id: EXT-MACH-DIR
title: "EU Machinery Directive 2006/42/EC"
type: directive             # or: standard, regulation, external
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

Reference format for source citations: `<EXT-ID>:<section-type>:<section-number>`

- `section-type` is one of: `clause`, `article`, `annex`, `section`
- Example: `EXT-MACH-DIR:annex:I-1.1.2`, `EU-2016-679:article:32`

### 4.10 Document entity (SRS / SDD)

```yaml
id: SRS-CLIENT-001
title: "Software Requirements Specification — ClientCorp"
type: document              # also recognised: srs, sdd
doc_meta:
  doc_type: SRS
  version: "1.0"
  client: ClientCorp
  status: approved
```

Any entity can declare membership in one or more documents:

```yaml
documents:
  - SRS-CLIENT-001
  - SDD-SYS-001
```

### 4.11 Attachments

```yaml
attachments:
  - path: docs/spec.pdf
    description: "Original specification document"
  - path: images/diagram.png
    description: "Architecture overview diagram"
```

### 4.12 Sources

```yaml
sources:
  - external: EU-2016-679:article:32   # mapping item — value is stored
  - id: REQ-SYS-005                    # mapping item
  - EN-ISO-13849-2023:clause:4.5.2     # plain scalar — stored as-is
```

---

## 5. Traceability conventions

### 5.1 Traceability block

Use `traceability:` (preferred) or the alias `links:` — both are parsed
identically.  Each entry is a mapping with:
- `id` (for entity links) **or** `artefact` (for file/artefact links)
- `relation` — a free-form relation type string

```yaml
traceability:
  - id: REQ-SYS-005
    relation: derived-from
  - id: TC-SW-001
    relation: verified-by
  - artefact: src/auth/login.c
    relation: implemented-in
  - artefact: tests/auth/test_login.c
    relation: verified-by
```

### 5.2 Recommended relation types

| Relation | Meaning |
|---|---|
| `derived-from` / `derives-from` | This entity is derived from the target |
| `implements` / `implemented-by` | This entity implements the target |
| `verified-by` / `verifies` | This entity is verified by the target |
| `refines` | This entity is a refinement of the target |
| `member-of` | This entity belongs to a group or epic |
| `implemented-in` | Implemented in the named source artefact |
| `constrained-by` | This entity is constrained by the target |
| `elaborates` | This entity elaborates (details) the target |
| `part-of` | This entity is a sub-part of the target |
| `satisfies` / `satisfied-by` | Coverage-indicating relation |
| `tests` / `tested-by` | Coverage-indicating relation |

### 5.3 Traceability and coverage

The `coverage` command considers a requirement "covered" when it has at least
one link (outgoing or incoming) with a coverage-indicating relation:
`verifies`, `verified-by`, `implements`, `implemented-by`, `implemented-in`,
`implemented-by-test`, `tests`, `tested-by`, `satisfies`, `satisfied-by`.

### 5.4 Bidirectional links

For bidirectional relations, declare the link on **both** entities.  The
`--strict-links` flag on `vibe-req links` warns when a known bidirectional
relation is only declared in one direction.

---

## 6. ECS component reference

All components are optional unless stated otherwise.  Any component can be
attached to any entity kind.

| Component | YAML key(s) | Key fields |
|---|---|---|
| `IdentityComponent` | `id`, `title`, `type` | `id` (≤64 chars), `title` (≤256), `kind`, `file_path` |
| `LifecycleComponent` | `status`, `priority`, `owner`, `version` | status, priority, owner, version |
| `TextComponent` | `description`, `rationale` | description (≤2048), rationale (≤1024) |
| `TagComponent` | `tags` | newline-separated tag list (≤1024 bytes total) |
| `UserStoryComponent` | `role`/`as_a`, `goal`/`i_want`, `reason`/`so_that` | role (≤256), goal (≤512), reason (≤512) |
| `AcceptanceCriteriaComponent` | `acceptance_criteria` | criteria list (≤2048 bytes) |
| `EpicMembershipComponent` | `epic` | epic_id (≤64) |
| `AssumptionComponent` | `assumption` mapping | text (≤1024), status (≤32), source (≤256) |
| `ConstraintComponent` | `constraint` mapping | text (≤1024), kind (≤64), source (≤256) |
| `DocumentMetaComponent` | `doc_meta` mapping | title (≤256), doc_type (≤64), version (≤32), client (≤128), status (≤32) |
| `DocumentMembershipComponent` | `documents` sequence | doc_ids list (≤1024 bytes) |
| `DocumentBodyComponent` | `body` | free-form text (≤64 KB) |
| `TraceabilityComponent` | `traceability` or `links` sequence | entries: `target\trelation` pairs (≤4096 bytes) |
| `SourceComponent` | `sources` sequence | source refs (≤2048 bytes) |
| `TestProcedureComponent` | `preconditions`, `steps`, `expected_result` | preconditions (≤2048), steps (≤4096), expected_result (≤1024) |
| `ClauseCollectionComponent` | `clauses` sequence | `id\ttitle` pairs (≤8192 bytes) |
| `AttachmentComponent` | `attachments` sequence | `path\tdescription` pairs (≤2048 bytes) |

---

## 7. CLI command reference

All commands scan YAML files recursively from the given `directory` (default:
`.`).  Files without a top-level `id:` field are ignored.

### Build

```bash
cd src && make          # builds bin/vibe-req
make test               # builds and runs unit tests
```

### Common commands

```bash
# Scaffold a new entity YAML file
vibe-req new <type> <id> [directory]
# Examples:
vibe-req new requirement REQ-SW-042 requirements/sw/
vibe-req new test-case TC-SW-042 tests/
vibe-req new story STORY-SW-042
vibe-req new assumption ASSUM-SYS-002

# List all entities (ECS model, with optional filters)
vibe-req list [--kind <kind>] [--component <comp>] [--status <status>] [--priority <prio>] [directory]
vibe-req list --kind requirement --status approved
vibe-req list --kind test-case
vibe-req list --component traceability

# Show traceability chain for one entity (outgoing + incoming links)
vibe-req trace <id> [directory]
vibe-req trace REQ-SW-001

# Requirement coverage report (how many requirements are linked to tests/code)
vibe-req coverage [directory]

# Find requirements and test cases with no traceability links
vibe-req orphan [directory]

# Print all relations as a Subject → Relation → Object table
vibe-req links [--strict-links] [directory]

# Generate a Markdown or HTML report
vibe-req report [--format md|html] [--output <file>] [filters…] [directory]
vibe-req report --format html --output report.html
vibe-req report --kind requirement --status approved --format html --output approved.html

# Help
vibe-req --help
```

### Filter flags (for `list` and `report`)

| Flag | Values |
|---|---|
| `--kind` | `requirement`, `group`, `story`, `design-note`, `section`, `assumption`, `constraint`, `test-case`, `external`, `document` |
| `--component` | `user-story`, `acceptance-criteria`, `epic`, `assumption`, `constraint`, `doc-meta`, `doc-membership`, `doc-body`, `traceability`, `tags` |
| `--status` | `draft`, `approved`, `deprecated` (or any custom value) |
| `--priority` | `must`, `should`, `could` |

---

## 8. Authoring guidelines for AI-generated YAML

1. **Always include `id:` and `title:`.**  The `id:` field is the only mandatory
   field; without it the entity is ignored by the tool.

2. **Choose `type:` carefully.**  Use the canonical values from section 3 to
   ensure the entity is recognised with the correct kind.

3. **Use `traceability:` (not `links:`) for new files.**  Both keys are parsed
   identically, but `traceability:` is the preferred canonical key for new
   content.

4. **Prefer `derived-from` over `derives-from`** and `verified-by` over
   `verifies` for requirement→test links (mirror the direction from the
   requirement's perspective).

5. **Assumption and constraint data goes inside the nested mapping,** not at
   the top level:
   ```yaml
   assumption:
     text: "…"
     status: open
   ```
   Not `assumption_text: "…"`.

6. **Source references use the canonical format**
   `<EXT-ID>:<section-type>:<section-number>`.

7. **Multi-entity files use `---` as a separator.**  Do not duplicate the YAML
   document start marker at the very top of a file — it is implied.

8. **Use `vibe-req new <type> <id>` to scaffold a template** before filling it
   in.  The scaffold guarantees all mandatory fields are present.

9. **Body text (`body:`, `description:`, `rationale:`) should use the YAML
   literal block scalar (`|`)** so that newlines are preserved.

10. **Keep IDs stable.** Never change the `id:` of an existing entity — it is
    the key used by all traceability links.
