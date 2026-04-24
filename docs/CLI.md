# CLI Reference

`vibe-req` is the command-line interface for **vibe-requirements**.

```
vibe-req [command] [options] [directory]
```

`directory` defaults to `.` (the current working directory).  The tool
recursively discovers all `.yaml` / `.yml` files in the directory tree that
have a top-level `id:` field; files without one are silently ignored.

---

## Commands

### (default) — list requirements

Running `vibe-req` without a subcommand prints a table of all entities
parsed via the legacy Requirement struct (id, title, type, status, priority).

```bash
vibe-req [directory]
```

**Example:**

```
$ vibe-req requirements/
+---------------+------------------------------------------+-------------+----------+----------+
| ID            | Title                                    | Type        | Status   | Priority |
+---------------+------------------------------------------+-------------+----------+----------+
| REQ-AUTH-001  | The system shall support email/password  | functional  | approved | must     |
| REQ-AUTH-002  | The system shall lock accounts …         | functional  | draft    | should   |
+---------------+------------------------------------------+-------------+----------+----------+

Total: 2 requirement(s)
```

---

### list

List all entities using the ECS model with optional filters.  The alias
`entities` behaves identically.

```bash
vibe-req list [--kind <kind>] [--component <comp>] [--status <status>] [--priority <prio>] [directory]
vibe-req entities [<same flags>] [directory]
```

When no filter flags are given every entity is shown.

#### Filter flags

| Flag | Description |
|------|-------------|
| `--kind <kind>` | Show only entities of the given kind (see table below) |
| `--component <comp>` | Show only entities that carry the named ECS component (see table below) |
| `--status <status>` | Show only entities whose `status` field equals the given value |
| `--priority <prio>` | Show only entities whose `priority` field equals the given value |

**`--kind` values:**

| Value | Entity kind |
|-------|-------------|
| `requirement` | Functional / non-functional requirement |
| `group` | Requirement group / section heading |
| `story` | User story |
| `design-note` | SDD free-form design note |
| `section` | Non-normative document section |
| `assumption` | Recorded assumption |
| `constraint` | Technical, legal, or environmental constraint |
| `test-case` | Explicit test case |
| `external` | External normative source (standard, directive, regulation) |
| `document` | A document entity (SRS, SDD, …) |

**`--component` values:**

| Value | Meaning |
|-------|---------|
| `user-story` | Entity carries a `UserStoryComponent` (has `role:` / `goal:`) |
| `acceptance-criteria` | Entity carries an `AcceptanceCriteriaComponent` |
| `epic` | Entity belongs to an epic (carries `EpicMembershipComponent`) |
| `assumption` | Entity carries an `AssumptionComponent` |
| `constraint` | Entity carries a `ConstraintComponent` |
| `doc-meta` | Entity carries a `DocumentMetaComponent` |
| `doc-membership` | Entity carries a `DocumentMembershipComponent` |
| `doc-body` | Entity carries a `DocumentBodyComponent` |
| `traceability` | Entity carries a `TraceabilityComponent` (has at least one link) |
| `tags` | Entity has at least one tag |

#### Examples

```bash
# All entities
vibe-req list

# Only requirements
vibe-req list --kind requirement

# Only approved must-priority requirements
vibe-req list --kind requirement --status approved --priority must

# All entities that declare traceability links
vibe-req list --component traceability

# All entities with acceptance criteria
vibe-req list --component acceptance-criteria

# All draft user stories
vibe-req list --kind story --status draft

# Scan a specific directory
vibe-req list requirements/
```

**Sample output:**

```
+---------------+------------------------------------+-------------+----------+----------+
| ID            | Title                              | Kind        | Status   | Priority |
+---------------+------------------------------------+-------------+----------+----------+
| ASSUM-SYS-001 | Network connectivity is available  | assumption  | draft    |          |
| CONSTR-SEC-001 | All external communication …      | constraint  | approved |          |
| REQ-AUTH-001  | The system shall support email …   | requirement | approved | must     |
| STORY-SW-001  | User can log in with email …       | story       | draft    | must     |
+---------------+------------------------------------+-------------+----------+----------+

Total: 4 entities
```

---

### trace

Show the full traceability chain for a single entity.  Displays the entity's
kind, title, and status, followed by all **outgoing** links (declared by this
entity) and all **incoming** links (other entities that link to this one).
Outgoing links are shown recursively up to two hops deep.

```bash
vibe-req trace <id> [directory]
```

**Arguments:**

| Argument | Description |
|----------|-------------|
| `<id>` | The entity ID to inspect (required) |
| `directory` | Root directory to scan (default: `.`) |

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
  REQ-AUTH-002 -[refines]->
```

If the entity ID is not found in any scanned file a note is printed but the
link data from the TripletStore is still shown if any other entity references it.

```bash
# Trace an artefact that is not a YAML entity but appears as a link target
$ vibe-req trace src/auth/login.c
Traceability chain for: src/auth/login.c
  (entity not found in scanned files)

Outgoing links:
  (none)

Incoming links:
  REQ-AUTH-001 -[implemented-in]->
```

---

### coverage

Report how many requirements have at least one traceability link to a test or
code implementation.  A requirement is considered "covered" when it has at
least one link — outgoing or incoming — whose relation is in the set of
coverage-indicating relations:

`verifies`, `verified-by`, `implements`, `implemented-by`, `implemented-in`,
`implemented-by-test`, `tests`, `tested-by`, `satisfies`, `satisfied-by`

```bash
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
| REQ-AUTH-004  | Session timeout policy       | draft  |
| REQ-SEC-002   | Audit logging                | draft  |
+---------------+------------------------------+--------+
```

---

### orphan

List all requirements and test cases that have **no** traceability links in
either direction.  An entity is considered orphaned when neither:
- it declares any outgoing links (in its own `traceability:` / `links:` block), nor
- any other entity links to it.

Only entities of kind `requirement` and `test-case` are checked.

```bash
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
| TC-SW-002     | test-case   | Verify timeout …  | draft  |
+---------------+-------------+-------------------+--------+

Total: 2 orphan(s)
```

If no orphans are found:

```
No orphaned requirements or test cases found.
```

---

### report

Generate a human-readable Markdown or HTML report of all entities (or a
filtered subset).  Entities are grouped by kind.  Each entity section
includes its metadata (kind, status, priority, owner), description,
rationale, tags, sources, acceptance criteria, user-story fields, and all
traceability links (outgoing and incoming).

```bash
vibe-req report [--format md|html] [--output <file>] \
                [--kind <kind>] [--status <status>] [--priority <prio>] \
                [--component <comp>] [directory]
```

#### Options

| Option | Description |
|--------|-------------|
| `--format md` | Output Markdown (default) |
| `--format html` | Output a self-contained HTML document with inline CSS |
| `--output <file>` | Write the report to `<file>` instead of stdout |
| `--kind <kind>` | Include only entities of the given kind |
| `--status <status>` | Include only entities with the given lifecycle status |
| `--priority <prio>` | Include only entities with the given priority |
| `--component <comp>` | Include only entities that carry the named ECS component |

#### Examples

```bash
# Markdown report to stdout
vibe-req report

# Markdown report to a file
vibe-req report --output report.md

# HTML report to a file
vibe-req report --format html --output report.html

# Report only requirements
vibe-req report --kind requirement

# Report only approved requirements as HTML
vibe-req report --kind requirement --status approved --format html --output approved.html

# Report for a specific directory
vibe-req report requirements/
```

**Sample Markdown output (excerpt):**

```markdown
# Requirements Report

## Requirements (3)

### REQ-AUTH-001 — The system shall support email/password authentication

**Kind:** requirement | **Status:** approved | **Priority:** must

The system shall support email and password authentication for all users.

**Traceability:**

- `[member-of]` → GRP-AUTH-001
- `[implemented-by]` ← STORY-SW-001

---
```

---

### new

Scaffold a new entity YAML file.  Creates `<id>.yaml` in the given
directory (default: current directory) containing a minimal template for
the requested entity type.

```bash
vibe-req new <type> <id> [directory]
```

**Arguments:**

| Argument | Description |
|----------|-------------|
| `<type>` | Entity type (see table below) |
| `<id>` | Entity identifier; also used as the file name (`<id>.yaml`) |
| `directory` | Output directory (default: `.`) |

**`<type>` values:**

| Value | Entity kind |
|-------|-------------|
| `requirement`, `functional`, `non-functional` | Functional / non-functional requirement |
| `group` | Requirement group / section heading |
| `story`, `user-story` | User story |
| `design-note`, `design` | SDD free-form design note |
| `section` | Non-normative document section |
| `assumption` | Recorded assumption |
| `constraint` | Technical, legal, or environmental constraint |
| `test-case`, `test` | Explicit test case |
| `external`, `directive`, `standard`, `regulation` | External normative source |
| `document`, `srs`, `sdd` | Document entity |

The command fails with exit code `1` if:
- the file `<id>.yaml` already exists in the target directory, or
- the type string is not recognised.

**Examples:**

```bash
# Scaffold a new requirement in the current directory
vibe-req new requirement REQ-AUTH-003

# Scaffold a user story in a specific directory
vibe-req new story STORY-LOGIN-001 requirements/sw/

# Scaffold a test case
vibe-req new test-case TC-AUTH-001
```

---

### links

Print all relations parsed from the requirement files as a
Subject → Relation → Object table.

```bash
vibe-req links [--strict-links] [directory]
```

Each row shows the subject entity ID, the relation predicate, the object
entity ID (or artefact path), and whether the triple was **declared** in YAML
or **inferred** (automatically generated inverse).

**Example:**

```bash
$ vibe-req links
+---------------+--------------+---------------+----------+
| Subject       | Relation     | Object        | Source   |
+---------------+--------------+---------------+----------+
| REQ-AUTH-001  | member-of    | GRP-AUTH-001  | declared |
| REQ-AUTH-002  | member-of    | GRP-AUTH-001  | declared |
| REQ-AUTH-002  | refines      | REQ-AUTH-001  | declared |
| STORY-SW-001  | implements   | REQ-AUTH-001  | declared |
+---------------+--------------+---------------+----------+

Total: 4 relation(s)
```

#### --strict-links

When `--strict-links` is given the tool additionally checks every declared
triple whose relation has a known inverse.  If the inverse is not explicitly
declared in YAML a warning is printed to `stderr` and the process exits with a
non-zero code.

```bash
$ vibe-req links --strict-links
warning: strict-links: 'STORY-SW-001 -[implements]-> REQ-AUTH-001' — \
  inverse '[implemented-by]' not explicitly declared by 'REQ-AUTH-001'
1 strict-links warning(s) found.
```

This is useful in CI pipelines that require fully bidirectional link
declarations.

---

## Global options

| Option | Description |
|--------|-------------|
| `-h`, `--help` | Print the help message and exit |
| `directory` | Root directory to scan (default: `.`) |

---

## Configuration file

If a `.vibe-req.yaml` file is present at the root of the scanned directory the
tool loads it before discovery.  The configuration can restrict which
directories are scanned and how entity IDs are prefixed.

```yaml
# .vibe-req.yaml
ignore_dirs:
  - node_modules
  - .git
  - build
```

---

## Exit codes

| Code | Meaning |
|------|---------|
| `0` | Success |
| `1` | Error (cannot open directory, missing argument, `--strict-links` warnings found) |

---

## See also

- [README](../README.md) — quick start and YAML file format examples
- [Design](DESIGN.md) — architecture, ECS domain model, and component reference
