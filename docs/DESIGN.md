# Design Document — vibe-requirements

This document describes the high-level architecture, the ECS domain model, the
in-memory graph model, and the file format specification for
**vibe-requirements**.  All source code is written in **C++23**.

---

## Table of Contents

- [1. Architecture Overview](#1-architecture-overview)
- [2. File Format: YAML](#2-file-format-yaml)
- [3. Alternative Formats](#3-alternative-formats)
- [4. Core Library Modules](#4-core-library-modules)
- [5. In-Memory Graph Model (TripletStore)](#5-in-memory-graph-model-tripletstore)
- [6. Entity-Component Model](#6-entity-component-model)
- [7. CLI Command Design](#7-cli-command-design)
- [8. Implementation Language: C++23](#8-implementation-language-c23)
- [9. Memory Layout and Ownership](#9-memory-layout-and-ownership)

---

## 1. Architecture Overview

`vibe-requirements` is structured as a single binary (`vibe-req`) that operates
on a directory tree of YAML files.  The pipeline is:

```
YAML files  →  entity_parser  →  EntityList  →  TripletStore  →  CLI commands
                (yaml/)           (entity.h)     (triplet_store.hpp)
```

1. **Discovery** (`discovery.cpp`): walks the directory tree and collects all
   `.yaml`/`.yml` paths, respecting ignore rules from `.vibe-req.yaml`.
2. **Parsing** (`yaml/entity_parser.cpp`): reads each YAML file and populates
   an `EntityList` (`std::vector<Entity>`).  Multi-document files (using `---`
   separators) produce multiple entities.
3. **Graph construction** (`list_cmd.cpp`): iterates the `EntityList` and
   inserts traceability triples into a `TripletStore`.  Inverse triples are
   inferred automatically.
4. **CLI commands** (`main.cpp`, `list_cmd.cpp`, `coverage.cpp`, `report.cpp`,
   `new_cmd.cpp`): execute the requested operation and print results to stdout.

---

## 2. File Format: YAML

All entity files are YAML.  A file may contain multiple YAML documents
separated by `---`.  Only documents with a top-level `id:` field are processed;
all others are silently ignored.

Multi-word YAML keys use `-` as the canonical word separator
(e.g. `acceptance-criteria`, `doc-meta`).  The parser accepts `_` variants for
backward compatibility and normalises them to `-` internally.

See the [README](../README.md#yaml-file-format) for detailed schema examples
for each entity kind.

---

## 3. Alternative Formats

The architecture does not prevent adding parsers for alternative formats
(TOML, S-expressions, Markdown front-matter).  The file format is isolated in
the `yaml/` sub-module.  A new format requires only a new parser that populates
the same `EntityList` and then passes it to the common graph-construction step.

---

## 4. Core Library Modules

| Source file | Responsibility |
|---|---|
| `entity.h` / `entity.cpp` | ECS component structs, `EntityList`, filter helpers |
| `yaml/entity_parser.cpp` | YAML → `EntityList` parser |
| `yaml/yaml_link_parser.cpp` | Parses `traceability:` / `links:` sequences |
| `triplet_store.hpp` / `.cpp` | C++ TripletStore class (`vibe::TripletStore`) |
| `triplet_store_c.h` / `_c.cpp` | C-language wrapper for TripletStore |
| `discovery.cpp` | Directory walker; respects `.vibe-req.yaml` ignore rules |
| `config.cpp` | Loads and exposes `.vibe-req.yaml` configuration |
| `list_cmd.cpp` | `list`, `trace`, `links` commands; `build_entity_relation_store()` |
| `coverage.cpp` | `coverage`, `orphan` commands |
| `report.cpp` | `report`, `doc` commands; Markdown and HTML renderers |
| `new_cmd.cpp` | `new` command; scaffolds YAML templates |
| `cli_args.cpp` | Argument parsing; `CliOptions` struct |
| `main.cpp` | Entry point; dispatches to command functions |
| `compat/cxx23_check.cpp` | Compile-time C++23 feature probe (used by `make check-compiler`) |

---

## 5. In-Memory Graph Model (TripletStore)

All inter-entity and entity-artefact relations are stored as
`(subject, predicate, object)` triples in a `vibe::TripletStore`.

### 5.1 Data structure

The store maintains three hash-map indexes (by subject, by predicate, by
object) backed by `std::unordered_map`.  This gives O(1) average-case lookup
by any component.

### 5.2 C++ API (`triplet_store.hpp`)

```cpp
#include "triplet_store.hpp"

vibe::TripletStore store;

// Add a declared triple
store.add("REQ-SW-001", "derived-from", "REQ-SYS-005");

// Infer inverse triples for all known relation pairs
store.infer_inverses();

// Query by subject
std::vector<const vibe::Triple *> links = store.find_by_subject("REQ-SW-001");
for (const auto *t : links) {
    // t->subject, t->predicate, t->object, t->inferred
}

// Query by object (incoming links)
auto incoming = store.find_by_object("REQ-SYS-005");
```

### 5.3 C wrapper API (`triplet_store_c.h`)

A thin C-language wrapper is provided for legacy call sites and for external
consumers that cannot use C++ headers directly.

```c
#include "triplet_store_c.h"

TripletStore *store = triplet_store_create();
triplet_store_add(store, "REQ-SW-001", "derived-from", "REQ-SYS-005");
triplet_store_infer_inverses(store);

CTripleList result = triplet_store_find_by_subject(store, "REQ-SW-001");
for (size_t i = 0; i < result.count; i++) {
    printf("%s -[%s]-> %s\n",
           result.triples[i].subject,
           result.triples[i].predicate,
           result.triples[i].object);
}
triplet_store_list_free(&result);
triplet_store_destroy(store);
```

> **Note:** New internal code should use the C++ `vibe::TripletStore` API
> directly.  The C wrapper exists for interoperability with external callers
> that cannot include C++ headers.

### 5.4 Inverse inference

The store has a built-in relation-pair table (e.g. `derives-from` ↔
`derived-to`, `implements` ↔ `implemented-by`).  Calling `infer_inverses()`
(or `triplet_store_infer_inverses()` via the C API) inserts synthetic inverse
triples for every user-declared triple whose relation has a registered inverse.
Synthetic triples carry `inferred == true`.

---

## 6. Entity-Component Model

### 6.1 Entity struct

Every parsed YAML document with an `id:` field maps to one `Entity`.  All
components are embedded directly in the `Entity` struct (value semantics,
no heap indirection for the struct itself).  C++ default construction
initialises all `std::string` and `std::vector` fields to empty.

```cpp
struct Entity {
    IdentityComponent           identity;
    LifecycleComponent          lifecycle;
    TextComponent               text;
    TagComponent                tags;
    UserStoryComponent          user_story;
    AcceptanceCriteriaComponent acceptance_criteria;
    EpicMembershipComponent     epic_membership;
    AssumptionComponent         assumption;
    ConstraintComponent         constraint;
    DocumentMetaComponent       doc_meta;
    DocumentMembershipComponent doc_membership;
    DocumentBodyComponent       doc_body;
    TraceabilityComponent       traceability;
    SourceComponent             sources;
    TestProcedureComponent      test_procedure;
    ClauseCollectionComponent   clause_collection;
    AttachmentComponent         attachment;
    AppliesToComponent          applies_to;
    VariantProfileComponent     variant_profile;
    CompositionProfileComponent composition_profile;
    RenderProfileComponent      render_profile;
};

using EntityList = std::vector<Entity>;
```

### 6.2 Checking for a component

Use `entity_has_component()` to test whether an entity carries a given
component.  The function takes the canonical `-`-separated component name:

```cpp
if (entity_has_component(&e, "traceability")) { /* … */ }
if (entity_has_component(&e, "user-story"))   { /* … */ }
if (entity_has_component(&e, "doc-meta"))     { /* … */ }
```

### 6.3 Filtering

`entity_apply_filter()` populates a destination `EntityList` with only the
entities from the source that match all non-null filter criteria:

```cpp
EntityList filtered;
entity_apply_filter(&all_entities, &filtered,
                    "requirement",  // kind
                    nullptr,        // component (any)
                    "approved",     // status
                    "must");        // priority
```

### 6.4 Component design rationale

All component fields use `std::string` instead of fixed-size `char[]` arrays.
`std::vector<std::string>` is used for multi-valued fields (tags, criteria,
doc membership, source references).  This removes all fixed-size string length
limits from the previous C implementation and eliminates the manual
`entity_free()` / `entity_copy()` lifetime management that the C version
required.

---

## 7. CLI Command Design

Each CLI command is implemented as a free function in its own translation unit:

| Command | Function | Source |
|---|---|---|
| (default) | `cmd_list_legacy()` | `main.cpp` |
| `list` / `entities` | `list_entities()` | `list_cmd.cpp` |
| `trace` | `cmd_trace_entity()` | `list_cmd.cpp` |
| `links` | `list_relations()` + `check_strict_links()` | `list_cmd.cpp` |
| `coverage` | `cmd_coverage()` | `coverage.cpp` |
| `orphan` | `cmd_orphan()` | `coverage.cpp` |
| `report` | `cmd_report()` | `report.cpp` |
| `doc` | `cmd_doc()` | `report.cpp` |
| `new` | `cmd_new()` | `new_cmd.cpp` |

Argument parsing is centralised in `cli_args.cpp` via `cli_parse_args()` which
fills a `CliOptions` struct.

---

## 8. Implementation Language: C++23

The entire codebase is compiled as **C++23** (`-std=c++23` with GCC or Clang).
There are no C (`.c`) source files.  The C-wrapper files
(`triplet_store_c.cpp`, `triplet_store_c.h`) use `extern "C"` linkage to
expose a C-compatible ABI, but are themselves compiled as C++.

### 8.1 Minimum compiler versions

| Compiler | Minimum version | Notes |
|---|---|---|
| GCC | 13 | Ships in Ubuntu 24.04 LTS |
| Clang | 17 | Ships in LLVM 17 release |
| MSVC | 19.38 (VS 2022 17.8) | `/std:c++latest` or `/std:c++23` required |

### 8.2 Key C++23 features used

| Feature | Usage |
|---|---|
| `if consteval` | `compat/cxx23_check.cpp` compiler probe |
| `static operator()` | `compat/cxx23_check.cpp` compiler probe |
| `auto(x)` decay-copy | `compat/cxx23_check.cpp` compiler probe |
| Multidimensional `operator[]` | `compat/cxx23_check.cpp` compiler probe |
| `std::string`, `std::vector` | All component structs and entity list |
| `std::optional` | TripletStore slot storage |
| `std::unordered_map` | TripletStore indexes |

### 8.3 Verifying C++23 support

Before building for the first time run:

```bash
cd src && make check-compiler
```

A successful run prints `Compiler check passed: <compiler> supports C++23.`
A failure means the configured compiler does not support C++23 — install or
configure a newer compiler (see the [README](../README.md#toolchain-requirements)).

---

## 9. Memory Layout and Ownership

### 9.1 EntityList ownership

`EntityList` (`std::vector<Entity>`) owns all its entities by value.  Copying
an `EntityList` performs a deep copy because all component fields are value
types (`std::string`, `std::vector`).  No manual `entity_free()` or
`entity_copy()` calls are required; RAII handles lifetime automatically.

### 9.2 TripletStore ownership

`vibe::TripletStore` owns all string data in its triples by value.  Pointers
returned by `find_by_subject()` / `find_by_object()` / `find_all()` point into
the store's internal storage and are **valid only until the next mutation**
(add / remove / compact).  Do not store these pointers beyond the current
read-only scope.

When using the C wrapper, `CTripleList` owns heap-allocated copies of all
string fields.  Every `CTripleList` result must be released with
`triplet_store_list_free()`.

### 9.3 No raw `new` / `delete`

Internal code does not use raw `new` or `delete`.  Dynamic allocation is
handled by `std::string`, `std::vector`, and `std::unordered_map`
constructors/destructors.  Exception: the C wrapper uses `strdup()` /
`free()` for the `CTriple` string copies, but these are fully encapsulated
inside `triplet_store_c.cpp` and released by `triplet_store_list_free()`.

---

## See also

- [README](../README.md) — quick start, CLI reference, and YAML format examples
- [CLI Reference](CLI.md) — complete CLI command reference
- [Porting Guide](PORTING.md) — migration instructions from pre-C++23 versions
- [Requirements](REQUIREMENTS.md) — functional and non-functional requirements
