# vibe-requirements — Agent System Prompt Fragment

Include this fragment in the system prompt of any LLM agent or coding assistant
that needs to create or modify vibe-requirements YAML files.  It provides a
concise but complete description of the tool's concepts, file format, and CLI.

---

## What is vibe-requirements?

**vibe-requirements** is a "requirements as code" tool.  All requirements, user
stories, test cases, assumptions, constraints, design notes, and external
normative sources are stored as plain YAML files inside a version-controlled
repository.  The companion CLI (`vibe-req`) provides listing, filtering,
traceability, coverage, and orphan-detection commands.

The data model is **Entity-Component-System (ECS)** inspired.  Every artefact
is a stable *entity* identified by a unique ID (e.g. `REQ-SW-001`).  Entities
carry optional *components* — fixed-size data blobs such as lifecycle metadata,
traceability links, acceptance criteria, and assumption text.  Any component
can be attached to any entity kind.

---

## YAML file format

A YAML file is discovered and parsed when it contains at least a top-level
`id:` field.  Files without `id:` are silently ignored.

A single file may contain **multiple entities** separated by `---` (YAML
multi-document format).

### Minimal entity

```yaml
id: REQ-SW-001
title: "User authentication"
type: functional
```

### Full requirement

```yaml
id: REQ-SW-001
title: "User authentication"
type: functional          # functional | non-functional | requirement
status: approved          # draft | approved | deprecated
priority: must            # must | should | could
owner: product-team
version: "1.0"
description: |
  The system shall authenticate users before granting access to any
  protected resource.
rationale: |
  Prevents unauthorized access.
tags:
  - security
sources:
  - external: EU-2016-679:article:32
traceability:
  - id: REQ-SYS-005
    relation: derived-from
  - id: TC-SW-001
    relation: verified-by
  - artefact: src/auth/login.c
    relation: implemented-in
```

### User story

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
  - Submitting valid credentials redirects to the dashboard.
  - Invalid credentials show a clear error message.
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
assumption:
  text: |
    The device has a reliable network connection during all normal
    operating modes.
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

### Test case

```yaml
id: TC-SW-001
title: "Verify login with valid credentials"
type: test-case
status: draft
priority: must
preconditions:
  - A registered user account exists.
  - The authentication endpoint is reachable.
steps:
  - step: 1
    action: "Submit login with correct credentials."
    expected_output: "System returns HTTP 200 and an auth token."
expected_result: "User gains access to the protected resource."
traceability:
  - id: REQ-SW-001
    relation: verifies
```

### Design note / section

```yaml
id: DESIGN-CH-2
title: "File Format: YAML"
type: design-note           # or: section
status: approved
body: |
  Each YAML document maps to a single entity identified by its `id:` field.
traceability:
  - id: REQ-002
    relation: elaborates
```

### Document entity (SRS / SDD)

```yaml
id: SRS-CLIENT-001
title: "Software Requirements Specification — ClientCorp"
type: document              # also: srs, sdd
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
```

### External normative source

```yaml
id: EXT-MACH-DIR
title: "EU Machinery Directive 2006/42/EC"
type: directive             # or: standard, regulation, external
clauses:
  - id: annex-I-1.1.2
    title: "Principles of safety integration"
    summary: |
      Machinery must be designed so it can be operated without putting
      persons at risk.
```

Source citation format: `<EXT-ID>:<section-type>:<section-number>`
where `section-type` ∈ {`clause`, `article`, `annex`, `section`}.

---

## Entity kinds

| `type` value | Kind | Typical ID prefix |
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

## Traceability conventions

Use `traceability:` (preferred) or its alias `links:`.  Each entry needs
either an `id` key (entity link) or an `artefact` key (file link), plus
`relation`.

```yaml
traceability:
  - id: REQ-SYS-005
    relation: derived-from
  - artefact: src/auth/login.c
    relation: implemented-in
```

Common relation types: `derived-from`, `implements`, `implemented-by`,
`verified-by`, `verifies`, `refines`, `member-of`, `elaborates`, `part-of`,
`constrained-by`, `implemented-in`, `satisfies`, `tests`.

Coverage-indicating relations (used by `vibe-req coverage`): `verifies`,
`verified-by`, `implements`, `implemented-by`, `implemented-in`,
`implemented-by-test`, `tests`, `tested-by`, `satisfies`, `satisfied-by`.

---

## CLI commands

```bash
# Build the tool
cd src && make

# Scaffold a new entity YAML file (recommended starting point)
vibe-req new <type> <id> [directory]
# e.g.:
vibe-req new requirement REQ-SW-042 requirements/sw/
vibe-req new test-case TC-SW-042 tests/
vibe-req new story STORY-SW-042

# List all entities (ECS model)
vibe-req list [--kind <kind>] [--status <status>] [--priority <prio>] [--component <comp>] [dir]

# Show traceability chain for one entity
vibe-req trace <id> [directory]

# Requirement coverage report
vibe-req coverage [directory]

# Find entities with no traceability links
vibe-req orphan [directory]

# Print all relations as a table
vibe-req links [--strict-links] [directory]

# Generate Markdown or HTML report
vibe-req report [--format md|html] [--output <file>] [filters…] [directory]

# Help
vibe-req --help
```

### `list` / `report` filter flags

| Flag | Example values |
|---|---|
| `--kind` | `requirement`, `story`, `test-case`, `assumption`, `constraint`, `design-note`, `section`, `group`, `external`, `document` |
| `--component` | `user-story`, `acceptance-criteria`, `epic`, `assumption`, `constraint`, `doc-meta`, `doc-membership`, `doc-body`, `traceability`, `tags` |
| `--status` | `draft`, `approved`, `deprecated` |
| `--priority` | `must`, `should`, `could` |

---

## Key authoring rules

1. `id:` is the only mandatory field.  All other fields are optional but
   recommended for traceability.
2. Use `vibe-req new <type> <id>` to scaffold a template before filling it in.
3. Use `traceability:` (not `links:`) in new files.
4. Assumption and constraint detail goes inside the nested mapping
   (`assumption:` / `constraint:`), not at the top level.
5. Use the literal block scalar (`|`) for multi-line text fields
   (`description:`, `rationale:`, `body:`, `text:`).
6. Source citations follow `<EXT-ID>:<section-type>:<section-number>`.
7. Never change the `id:` of an existing entity — all traceability links
   reference it by that ID.
8. Multi-entity files use `---` as a separator between documents.
9. Entity IDs should be stable, uppercase, and follow the project's
   naming conventions (e.g. `REQ-SW-001`, `TC-SW-001`).
