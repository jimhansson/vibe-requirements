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

## 2. File Format: YAML (Primary)

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

## 5. CLI Command Design

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

## 6. Implementation Language Trade-offs

| Language | Distribution | Ecosystem | Known to author | Suitability |
|---|---|---|---|---|
| **C / C++** | Single binary | libyaml / RapidYAML; GTK / Qt for GUI | Yes | Good; GUI is straightforward with Qt |
| **Common Lisp** | Single binary (SBCL `--save-lisp-and-die`) | cl-yaml; McCLIM for GUI | Yes | Excellent for DSL; GUI ecosystem is limited |
| **Rust** | Single static binary | `serde_yaml`; `egui` / GTK for GUI | To explore | Excellent distribution story; memory safety |
| **Go** | Single static binary | `gopkg.in/yaml.v3`; Fyne / `gio` for GUI | To explore | Very easy cross-compilation; moderate GUI |

**Recommendation for evaluation:** Prototype the core parser and CLI in both Rust and Go, compare ergonomics and binary size, then decide. The GUI can be deferred to a later phase and use whichever GUI toolkit fits the chosen language.
