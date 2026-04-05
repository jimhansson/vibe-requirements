# Requirements Document

## 1. Introduction

### 1.1 Purpose

This document describes the requirements for **vibe-requirements**, a "requirements as code" tool. The goal is to manage all project requirements — software, hardware, safety-related, and standards-derived — as plain text files stored directly inside a version-controlled repository, with a companion command-line interface (CLI) and optional graphical user interface (GUI) for interacting with those files.

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

### 3.9 Scripting and Programmability

| ID | Requirement | Priority |
|---|---|---|
| REQ-080 | The tool shall provide an embedded scripting layer that allows users to extend its behaviour without modifying the tool's source code | Should |
| REQ-081 | The scripting layer shall expose the in-memory requirement model (all requirements, test cases, and external sources) as read-only objects that scripts can inspect and query | Should |
| REQ-082 | The scripting layer shall allow users to define custom validation rules; violations shall be reported as errors or warnings alongside the built-in validation output | Should |
| REQ-083 | The scripting layer shall allow users to define custom lint rules that inspect the full requirement link graph | Should |
| REQ-084 | The scripting layer shall allow users to define custom report generators that produce output in any format the script chooses | Could |
| REQ-085 | Scripts shall be loadable by listing them in `.vibe-req.yaml` and/or by convention-based auto-discovery (e.g., all files in a designated `scripts/` folder) | Should |
| REQ-086 | The embedded scripting interpreter shall be distributed under a license compatible with the project's GNU GPL v3 license | Must |
| REQ-087 | The scripting layer shall not require users to install a separate runtime; the interpreter shall be compiled into the tool binary | Should |
| REQ-088 | Errors in user scripts (syntax errors, runtime exceptions) shall be reported clearly with the script file name and, where possible, the line number; they shall not silently suppress the built-in tool output | Must |

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

## 6. Roadmap

| Phase | Milestone | Key Deliverables |
|---|---|---|
| 1 | Core CLI MVP | `init`, `new`, `validate`, `status`; YAML format; software requirements only |
| 2 | Traceability | `trace`, link graph, broken-link detection, external-source document type, test-case document type |
| 3 | Reporting | `report` (Markdown + HTML), `lint`, CI pipeline integration |
| 4 | Hardware & Safety | Hardware artefact links, safety requirement type, standards/directive support, subsystem filtering |
| 5 | GUI | Native GUI: list view, traceability graph, inline editor |
| 6 | Advanced | Alternative formats (TOML, S-expressions), `export` (CSV, ReqIF), user story and SDD document types |
| 7 | Scripting | Embedded scripting layer (Lua and/or Guile): custom validators, lint rules, and report generators |
