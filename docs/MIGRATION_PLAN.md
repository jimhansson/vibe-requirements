# Migration Plan — Dogfooding vibe-requirements (Issue #19)

This document tracks the plan for migrating the project to manage its own
requirements using the vibe-requirements tool itself. Each step corresponds
to a sub-issue of [issue #19](https://github.com/jimhansson/vibe-requirements/issues/19)
and will be implemented on its own branch, then merged into `feature/migration`.

---

## Step 1 — Move examples and add ignore-directories support

**Branch:** `feature/migration-step1-examples-and-ignore`

Move the current sample YAML files (`requirements/`) into
`examples/example1/` so the repository has a worked example separate from
the real requirements. At the same time, extend the tool to read an
`ignore_dirs` directive from `.vibe-req.yaml` so that the `examples/`
directory can be excluded when the tool runs on the real project.

Deliverables:
- `examples/example1/` — contains the current sample YAML files
- `.vibe-req.yaml` — project config with `ignore_dirs: [examples]`
- `src/discovery.c` — reads and respects `ignore_dirs` from config
- `src/config.[ch]` — new config parser (or extension of existing one)

---

## Step 2 — Migrate requirements from REQUIREMENTS.md to YAML

Each requirement group becomes its own sub-issue and branch. Multiple
requirements within the same group are stored in a single YAML file using
`---` document separators (multi-document stream).

### Step 2.1 — File-Based Storage requirements (§3.1, REQ-001–008)

**Branch:** `feature/migration-step2-file-based-storage`

Migrate the 8 requirements in section 3.1 to
`requirements/sys/file-based-storage.yaml`.

### Step 2.2 — Document Types requirements (§3.2, REQ-010–015)

**Branch:** `feature/migration-step2-document-types`

Migrate the 6 requirements in section 3.2 to
`requirements/sys/document-types.yaml`.

### Step 2.3 — Requirement Fields requirements (§3.3, REQ-020–029)

**Branch:** `feature/migration-step2-requirement-fields`

Migrate the 10 requirements in section 3.3 to
`requirements/sys/requirement-fields.yaml`.

### Step 2.4 — Traceability requirements (§3.4, REQ-030–035)

**Branch:** `feature/migration-step2-traceability`

Migrate the 6 requirements in section 3.4 to
`requirements/sys/traceability.yaml`.

### Step 2.5 — External Normative Sources requirements (§3.5, REQ-040–042)

**Branch:** `feature/migration-step2-external-sources`

Migrate the 3 requirements in section 3.5 to
`requirements/sys/external-sources.yaml`.

### Step 2.6 — CLI requirements (§3.6, REQ-050–059)

**Branch:** `feature/migration-step2-cli`

Migrate the 10 requirements in section 3.6 to
`requirements/sw/cli.yaml`.

### Step 2.7 — GUI requirements (§3.7, REQ-060–069)

**Branch:** `feature/migration-step2-gui`

Migrate the 10 requirements in section 3.7 to
`requirements/sw/gui.yaml`.

### Step 2.8 — Hardware Project Support requirements (§3.8, REQ-070–072)

**Branch:** `feature/migration-step2-hardware`

Migrate the 3 requirements in section 3.8 to
`requirements/sys/hardware-support.yaml`.

### Step 2.9 — Non-Functional Requirements (§4, NFR-001–006)

**Branch:** `feature/migration-step2-nfr`

Migrate the 6 non-functional requirements in section 4 to
`requirements/sys/non-functional.yaml`.

---

## Step 3 — Write test-description YAML for directory exclusion

**Branch:** `feature/migration-step3-test-description`

Write a test-case entity (YAML) that describes the expected behaviour of the
`ignore_dirs` feature added in Step 1, and link it to the corresponding
requirement (the one added in Step 1 that captures the ignore-directories
behaviour).

Deliverables:
- `requirements/tests/TC-IGNORE-DIRS-001.yaml` — test-case description

---

## Step 4 — Write the test

**Branch:** `feature/migration-step4-test-implementation`

Implement the automated test described by `TC-IGNORE-DIRS-001` in the
existing C test suite (`src/test_*.c`), verifying that directories listed in
`ignore_dirs` of `.vibe-req.yaml` are not scanned during auto-discovery.

Deliverables:
- `src/test_discovery_ignore.c` (or equivalent) — automated test
- Makefile updated to compile and run the new test

---

## Branch merge order

```
feature/migration-step1-examples-and-ignore  ─┐
feature/migration-step2-file-based-storage    ─┤
feature/migration-step2-document-types        ─┤
feature/migration-step2-requirement-fields    ─┤
feature/migration-step2-traceability          ─┤  → feature/migration
feature/migration-step2-external-sources      ─┤
feature/migration-step2-cli                   ─┤
feature/migration-step2-gui                   ─┤
feature/migration-step2-hardware              ─┤
feature/migration-step2-nfr                   ─┤
feature/migration-step3-test-description      ─┤
feature/migration-step4-test-implementation   ─┘
```
