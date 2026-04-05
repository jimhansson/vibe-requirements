# Requirements and Design Document

## 1. Introduction

### 1.1 Purpose

This document describes the requirements and high-level design for **vibe-requirements**, a "requirements as code" tool. The goal is to manage all project requirements — software, hardware, safety-related, and standards-derived — as plain text files stored directly inside a version-controlled repository, with a companion command-line interface (CLI) and optional graphical user interface (GUI) for interacting with those files.

### 1.2 Concept: Requirements as Code

Requirements are stored as human-readable, version-controllable text files (e.g., YAML, TOML, or S-expressions). Because they live inside the repository alongside the source code and hardware design, every change to a requirement is tracked by the same VCS (e.g., Git) that tracks the rest of the project. Reviews, history, branching, and diffing work out of the box.

### 1.3 Scope

The tool covers:

- Capturing and editing requirements, user stories, system design documents (SDD), test cases, and related engineering documents.
- Linking requirements to each other, to code, to hardware design artefacts, and to external normative sources (standards, regulations, directives).
- Providing a CLI for day-to-day use in terminals and CI pipelines.
- Providing an optional GUI (native, not browser-based) for visual exploration and editing.
- Supporting projects that mix software and hardware (e.g., embedded systems, machines, electromechanical products).

### 1.4 Definitions

| Term | Definition |
|---|---|
| Requirement | A condition or capability that a system or component must satisfy |
| External requirement | A requirement derived from an external normative source such as a standard, regulation, or legal directive (e.g., EU Machinery Directive) |
| Traceability | The ability to follow a requirement forward (to design/code/tests) and backward (to its origin) |
| SDD | System/Software Design Document |
| User story | An informal, plain-language description of a feature from an end-user perspective |
| Test case | A structured document describing steps and expected results used to verify that a requirement is satisfied |
| Artefact | Any project file that can be linked to a requirement (source file, schematic, test case, document) |

---

## 2. Stakeholders

| Stakeholder | Role |
|---|---|
| Systems/requirements engineer | Defines, structures, and maintains requirements |
| Software developer | Implements requirements and links them to code |
| Hardware designer | Links requirements to schematics, PCB layouts, and BOM items |
| QA / verification engineer | Creates test cases and links them to requirements |
| Project manager | Tracks requirement status and project health |
| Safety / compliance engineer | Manages requirements derived from standards and legal directives |
| End user / customer | Source of high-level needs and user stories |

---

## 3. Functional Requirements

### 3.1 File-Based Storage

| ID | Requirement | Priority |
|---|---|---|
| REQ-001 | Requirements and related documents shall be stored as plain text files within the project repository | Must |
| REQ-002 | The tool shall support YAML as a primary file format for requirement files | Must |
| REQ-003 | The tool shall be designed so that alternative file formats (e.g., TOML, S-expressions, plain Markdown front-matter) can be added without redesigning the core | Should |
| REQ-004 | Requirement files shall be human-readable and human-editable without the tool (i.e., in any text editor) | Must |
| REQ-005 | The tool shall perform zero-config auto-discovery of all requirement files anywhere in the repository; an optional `.vibe-req.yaml` configuration file may specify glob patterns to restrict or extend the search scope | Must |
| REQ-006 | The tool should document and promote a recommended directory convention (e.g., `requirements/`, `stories/`, `design/`, `tests/`, `external/`) and the `init` command should scaffold this structure when requested | Should |

### 3.2 Document Types

| ID | Requirement | Priority |
|---|---|---|
| REQ-010 | The tool shall support a **Requirement** document type with at minimum: unique ID, title, description, type, status, priority, and links | Must |
| REQ-011 | The tool shall support a **User Story** document type with: ID, title, narrative ("As a … I want … so that …"), acceptance criteria, and links | Should |
| REQ-012 | The tool shall support a **System Design Document (SDD)** document type that can reference and be referenced by requirements | Should |
| REQ-013 | The tool shall support an **External Source** document type to represent normative references such as standards or legal directives, including clauses or articles that generate derived requirements | Should |
| REQ-014 | The tool shall support a **Hardware Artefact** link type so that requirements can be traced to schematics, PCB files, mechanical drawings, or BOM entries | Should |
| REQ-015 | The tool shall support a **Test Case** document type with at minimum: unique ID, title, description, preconditions, test steps, expected results, verification method (`inspection`, `analysis`, `demonstration`, `test`), status, and links to the requirements it verifies | Should |

### 3.3 Requirement Fields

| ID | Requirement | Priority |
|---|---|---|
| REQ-020 | Each requirement shall have a project-unique, human-readable identifier (e.g., `REQ-SYS-042`) | Must |
| REQ-021 | Each requirement shall have a **type**: `functional`, `non-functional`, `constraint`, `safety`, `interface` | Must |
| REQ-022 | Each requirement shall have a **status**: `draft`, `review`, `approved`, `implemented`, `verified`, `rejected` | Must |
| REQ-023 | Each requirement shall have a **priority**: `critical`, `high`, `medium`, `low` | Must |
| REQ-024 | Each requirement may declare one or more **sources**, which can be another requirement ID or an external normative reference (standard clause or directive article) | Must |
| REQ-025 | Each requirement may carry user-defined **tags** (labels) for free-form categorization | Should |
| REQ-026 | Requirements shall support a **rationale** field explaining why the requirement exists | Should |
| REQ-027 | Requirements shall support a **verification method** field: `inspection`, `analysis`, `demonstration`, `test` | Should |

### 3.4 Traceability

| ID | Requirement | Priority |
|---|---|---|
| REQ-030 | The tool shall allow a requirement to be linked to one or more other requirements (parent, child, derives-from, conflicts-with, implements) | Must |
| REQ-031 | The tool shall allow a requirement to be linked to test cases or verification records | Should |
| REQ-032 | The tool shall allow a requirement to be linked to source code files or specific Git commits | Should |
| REQ-033 | The tool shall allow a requirement to be linked to hardware design artefacts (schematics, drawings, BOM rows) | Should |
| REQ-034 | The tool shall allow a requirement to be linked to external normative sources (standard, directive, clause) | Must |
| REQ-035 | The CLI shall produce a traceability report showing the full link graph for a given requirement or for all requirements | Should |

### 3.5 External Normative Sources

| ID | Requirement | Priority |
|---|---|---|
| REQ-040 | The tool shall support defining an external normative source (e.g., EN ISO 13849, EU Machinery Directive 2006/42/EC) as a first-class document in the repository | Must |
| REQ-041 | Specific clauses or articles of a normative source shall be referenceable so that derived requirements can declare their origin precisely (e.g., `source: EN-ISO-13849-2023:clause:4.5.2`) | Must |
| REQ-042 | The CLI shall be able to report which requirements are derived from a given standard or directive and flag any that are not yet implemented or verified | Should |

### 3.6 Command-Line Interface (CLI)

| ID | Requirement | Priority |
|---|---|---|
| REQ-050 | The tool shall provide a CLI that runs on Linux, macOS, and Windows | Must |
| REQ-051 | The CLI shall provide commands to create, read, update, and delete requirements and related documents | Must |
| REQ-052 | The CLI shall provide a `validate` command that checks all requirement files for schema correctness and broken links | Must |
| REQ-053 | The CLI shall provide a `report` command that generates a human-readable summary (e.g., Markdown or HTML) of the requirement set | Should |
| REQ-054 | The CLI shall provide a `trace` command that outputs the traceability graph for one or all requirements | Should |
| REQ-055 | The CLI shall provide a `status` command showing counts of requirements per status and priority | Should |
| REQ-056 | The CLI shall exit with a non-zero code when validation fails, making it suitable for use in CI pipelines | Must |
| REQ-057 | The CLI shall ideally be distributed as a single self-contained executable with no runtime dependencies | Should |
| REQ-058 | The tool shall gracefully handle malformed or invalid input files: when a file cannot be parsed or fails schema validation, the tool shall report the error (including file path and, where possible, the line/column or field name) and continue processing all remaining files rather than aborting | Must |
| REQ-059 | The CLI shall provide a `--fail-fast` flag that causes processing to stop immediately on the first encountered parse or validation error; this flag is intended for use in CI pipelines where early termination is preferred | Should |

### 3.7 Graphical User Interface (GUI)

| ID | Requirement | Priority |
|---|---|---|
| REQ-060 | The tool shall provide an optional native GUI application (not browser-based) for browsing and editing requirements | Should |
| REQ-061 | The GUI shall read and write the same file formats used by the CLI with no intermediary database | Must |
| REQ-062 | The GUI shall provide a filterable, sortable list of all requirements and documents in the repository | Should |
| REQ-063 | The GUI shall provide a visual traceability graph view | Should |
| REQ-064 | The GUI shall provide an inline editor for requirement fields | Should |
| REQ-065 | The GUI shall not require installation of a web browser or JavaScript runtime | Must |

### 3.8 Hardware Project Support

| ID | Requirement | Priority |
|---|---|---|
| REQ-070 | The tool shall allow requirements to be categorized by subsystem, including hardware subsystems (e.g., `subsystem: power-supply`) | Should |
| REQ-071 | The tool shall support link types specific to hardware artefacts: `allocates-to-hardware`, `verified-by-hw-test`, `implemented-in-schematic` | Should |
| REQ-072 | The report generator shall be able to produce a requirements subset filtered by subsystem (hardware or software) | Should |

---

## 4. Non-Functional Requirements

| ID | Requirement | Priority |
|---|---|---|
| NFR-001 | The CLI shall start up and respond to commands in under 500 ms on typical hardware | Should |
| NFR-002 | The tool shall handle repositories with at least 10 000 requirement files without significant performance degradation | Should |
| NFR-003 | The tool shall not require network access for core operations (create, validate, report) | Must |
| NFR-004 | The file format schema shall be documented and stable; breaking changes shall require a major version increment | Must |
| NFR-005 | The tool shall be buildable from source on Linux with a documented build procedure | Must |

---

## 5. Constraints

- **No JavaScript / Node.js / browser runtime** — the tool (CLI and GUI) must not depend on a JavaScript runtime or a web browser engine.
- **Preferred implementation languages:** C, C++, Common Lisp, Rust, or Go. The choice should be justified against criteria such as single-binary distribution, ecosystem, and long-term maintainability.
- **Single-binary preferred:** The CLI and, where practical, the GUI should be distributable as a self-contained executable with no mandatory external runtime.
- **All project data lives in the repository** — no external database or server is required for core functionality.
- **VCS-agnostic** — although Git is the primary target, the tool should not hard-code Git-specific concepts in the file format.

---

## 6. High-Level Design

### 6.1 Architecture Overview

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
          │          vibe-req  (binary)           │
          │                                      │
          │  ┌─────────┐  ┌──────────────────┐   │
          │  │   CLI   │  │   GUI (optional)  │   │
          │  └────┬────┘  └────────┬─────────┘   │
          │       │                │              │
          │  ┌────▼────────────────▼──────────┐   │
          │  │         Core Library           │   │
          │  │  parser · validator · linker   │   │
          │  │  reporter · tracer             │   │
          │  └────────────────────────────────┘   │
          └──────────────┬───────────────────────┘
                         │ reads / writes
                    ┌────▼────┐
                    │  files  │  (YAML, TOML, …)
                    └─────────┘
```

### 6.2 File Format: YAML (Primary)

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

### 6.3 Alternative Format Candidates

| Format | Pros | Cons |
|---|---|---|
| **YAML** | Widely known, good tooling, human-readable | Indentation-sensitive, complex edge cases |
| **TOML** | Simpler syntax, unambiguous, good for flat structures | Less natural for nested/long text blocks |
| **S-expressions** | Trivial to parse in Common Lisp, extensible | Unfamiliar to most engineers |
| **Markdown + front-matter** | Requirements as prose documents, renders on GitHub | Harder to parse structured fields reliably |
| **Custom DSL** | Full control over syntax | Maintenance burden, no existing tooling |

The initial implementation uses YAML. The parser layer shall be abstracted so that additional formats can be added by implementing a format-specific reader/writer module.

### 6.4 Core Library Modules

| Module | Responsibility |
|---|---|
| `parser` | Reads and deserializes requirement files from disk; format-pluggable |
| `validator` | Checks schema correctness, required fields, unique IDs, and link integrity |
| `linker` | Builds the full link graph from all files in the repository |
| `reporter` | Renders reports (Markdown, HTML, plain text) from the in-memory model |
| `tracer` | Traverses the link graph to produce traceability chains |
| `exporter` | Converts the model to other representations (CSV, ReqIF, …) |

### 6.5 CLI Command Design

```
vibe-req <command> [options]

Commands:
  init              Initialize a new vibe-req project in the current directory
  new <type> <id>   Create a new requirement / story / SDD / test-case / external-source file
  validate          Validate all files: schema, IDs, links
  status            Print counts by status and priority
  trace <id>        Show full traceability chain for a requirement
  report            Generate a Markdown / HTML requirements report
  export <format>   Export to CSV or ReqIF
  lint              Check for orphaned requirements, missing verifications, etc.
```

### 6.6 Implementation Language Trade-offs

| Language | Distribution | Ecosystem | Known to author | Suitability |
|---|---|---|---|---|
| **C / C++** | Single binary | libyaml / RapidYAML; GTK / Qt for GUI | Yes | Good; GUI is straightforward with Qt |
| **Common Lisp** | Single binary (SBCL `--save-lisp-and-die`) | cl-yaml; McCLIM for GUI | Yes | Excellent for DSL; GUI ecosystem is limited |
| **Rust** | Single static binary | `serde_yaml`; `egui` / GTK for GUI | To explore | Excellent distribution story; memory safety |
| **Go** | Single static binary | `gopkg.in/yaml.v3`; Fyne / `gio` for GUI | To explore | Very easy cross-compilation; moderate GUI |

**Recommendation for evaluation:** Prototype the core parser and CLI in both Rust and Go, compare ergonomics and binary size, then decide. The GUI can be deferred to a later phase and use whichever GUI toolkit fits the chosen language.

---

## 7. Roadmap

| Phase | Milestone | Key Deliverables |
|---|---|---|
| 1 | Core CLI MVP | `init`, `new`, `validate`, `status`; YAML format; software requirements only |
| 2 | Traceability | `trace`, link graph, broken-link detection, external-source document type, test-case document type |
| 3 | Reporting | `report` (Markdown + HTML), `lint`, CI pipeline integration |
| 4 | Hardware & Safety | Hardware artefact links, safety requirement type, standards/directive support, subsystem filtering |
| 5 | GUI | Native GUI: list view, traceability graph, inline editor |
| 6 | Advanced | Alternative formats (TOML, S-expressions), `export` (CSV, ReqIF), user story and SDD document types |

---

## 8. Open Questions

1. **Primary implementation language** — After prototyping, which language (Rust, Go, C++, Common Lisp) offers the best balance of developer experience, binary portability, and GUI capability?
2. **Default file format** — Should YAML be the only format in Phase 1, or should TOML be supported in parallel from the start?
3. **ID scheme** — Should IDs be fully manual (engineer chooses `REQ-SW-001`) or partially auto-generated (tool assigns the next available number within a prefix)?
4. **ReqIF export** — ReqIF is the ISO standard interchange format for requirements. Is this a priority for the target projects?
5. **Multi-project monorepo** — Should one repository be able to host multiple independent projects, each with their own ID namespace and configuration?
6. **Hardware CAD integration** — Which CAD/EDA file formats should be targeted for hardware artefact links (e.g., KiCad, Altium, FreeCAD)?
