# Porting Guide — C++23 Migration

This guide describes how to update downstream integrations, packagers, and
contributor workflows after the **vibe-requirements** codebase was migrated
from a mixed C11/C++ codebase to a pure **C++23** codebase.

---

## Table of Contents

- [1. Who is this guide for?](#1-who-is-this-guide-for)
- [2. What changed](#2-what-changed)
- [3. Toolchain requirements](#3-toolchain-requirements)
- [4. Build system changes](#4-build-system-changes)
- [5. C++ idiom changes for contributors](#5-c-idiom-changes-for-contributors)
  - [5.1 Entity strings: char[] → std::string](#51-entity-strings-char--stdstring)
  - [5.2 Entity lists: EntityList struct → std::vector\<Entity\>](#52-entity-lists-entitylist-struct--stdvectorentity)
  - [5.3 Memory management: manual → RAII](#53-memory-management-manual--raii)
  - [5.4 TripletStore API: C++ only](#54-tripletstore-api-c-only)
- [6. Migration checklist for packagers](#6-migration-checklist-for-packagers)
- [7. Migration checklist for contributors](#7-migration-checklist-for-contributors)
- [8. Frequently asked questions](#8-frequently-asked-questions)

---

## 1. Who is this guide for?

- **Downstream packagers** (Debian/Ubuntu, Homebrew, Fedora, Nix, etc.) who
  build `vibe-req` from source and need to update their build recipes.
- **Contributors** who have local branches or patches based on the pre-C++23
  codebase and need to rebase or port their changes.
- **Integrators** who embed parts of the `vibe-req` library in their own
  projects and call the public C or C++ APIs.

---

## 2. What changed

| Area | Before (C11/C++17) | After (C++23) |
|---|---|---|
| Language standard | C11 (`-std=c11`) for `.c` files; C++17 for `.cpp` files | C++23 (`-std=c++23`) for all files |
| Source file extensions | `.c` and `.cpp` both present | `.cpp` only — no `.c` files remain |
| Entity string fields | `char id[64]`, `char title[256]`, etc. | `std::string id`, `std::string title`, etc. |
| Entity list type | `struct EntityList { Entity *items; size_t count; }` | `using EntityList = std::vector<Entity>` |
| Memory management | `entity_free()` / `entity_copy()` required | RAII — no manual calls needed |
| Make variables | `CC`, `CFLAGS` and `CXX`, `CXXFLAGS` | `CXX`, `CXXFLAGS` only |
| Linker | `gcc` for C-only objects; `g++` for C++ objects | `g++` always (C++ runtime always required) |
| Compiler check | None | `make check-compiler` target |

---

## 3. Toolchain requirements

| Component | Minimum version | How to obtain |
|---|---|---|
| C++ compiler | GCC ≥ 13 / Clang ≥ 17 / MSVC 19.38+ | See platform notes below |
| libyaml | any recent release | `apt-get install libyaml-dev` |
| Google Test / Google Mock | 1.12+ | `apt-get install libgtest-dev libgmock-dev` |
| GNU Make | 4.x | `apt-get install make` |

### Ubuntu / Debian

GCC 13 ships in Ubuntu 24.04 LTS and Ubuntu 23.10.  On older Ubuntu releases
install it from the `ubuntu-toolchain-r/test` PPA:

```bash
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install g++-13
export CXX=g++-13
```

### macOS (Homebrew)

```bash
brew install gcc libyaml googletest
export CXX=g++-13   # or g++-14, whichever Homebrew installed
```

### Fedora / RHEL

```bash
sudo dnf install gcc-c++ make libyaml-devel gtest-devel gmock-devel
```

GCC 13 ships in Fedora 38+.  On RHEL 9 use Developer Toolset or the GCC 13
SCL.

### Verify C++23 support

Before building, verify that the configured compiler passes the C++23 feature
probe:

```bash
cd src && make check-compiler
```

---

## 4. Build system changes

### Removed Make variables

The `CC` and `CFLAGS` variables no longer exist in `src/Makefile` because
there are no C source files.  If you overrode these in your build recipe,
remove them:

```diff
-CC      = gcc
-CFLAGS  = -std=c11 -Wall -O2
 CXX     = g++
 CXXFLAGS = -std=c++23 -Wall -Wextra -O2
```

### C++ linker always required

Because all translation units are C++, the final link step uses `g++`
(or `clang++`) regardless of which source files are being linked.  Packaging
recipes that passed `-lgcc` or relied on a C linker must be updated.

### New `check-compiler` target

```bash
make check-compiler   # verify C++23 before the main build
make                  # build vibe-req
make test             # build and run all unit tests
```

---

## 5. C++ idiom changes for contributors

### 5.1 Entity strings: `char[]` → `std::string`

**Before (C struct with fixed-size buffers):**

```c
/* Old API — do not use */
struct IdentityComponent {
    char id[64];
    char title[256];
    char type_raw[64];
    char file_path[512];
    int  doc_index;
    EntityKind kind;
};

/* Copying a string required strncpy + manual NUL-termination */
strncpy(e.identity.id, "REQ-SW-001", sizeof(e.identity.id) - 1);
```

**After (C++ with `std::string`):**

```cpp
// New API
struct IdentityComponent {
    std::string  id;
    std::string  title;
    std::string  type_raw;
    std::string  file_path;
    int          doc_index = 0;
    EntityKind   kind      = ENTITY_KIND_UNKNOWN;
};

// String assignment — no length limit, no manual NUL-termination
e.identity.id = "REQ-SW-001";
```

The same pattern applies to all other string fields across all component
structs.  There are no longer any `char[]` length limits on field values.

### 5.2 Entity lists: `EntityList` struct → `std::vector<Entity>`

**Before (manual heap array):**

```c
/* Old API — do not use */
EntityList list;
entity_list_init(&list);
entity_list_add(&list, &new_entity);   // deep-copies entity
// …
entity_list_free(&list);               // frees all entities + array
```

**After (`std::vector<Entity>` with value semantics):**

```cpp
// New API
EntityList list;                        // EntityList = std::vector<Entity>
list.push_back(new_entity);             // copy-constructs; RAII manages lifetime
// …
// No explicit free required — destructor handles everything
```

Iteration uses standard C++ range-based for:

```cpp
for (const Entity &e : list) {
    printf("%s\n", e.identity.id.c_str());
}
```

### 5.3 Memory management: manual → RAII

The old C implementation required:

```c
entity_free(&e);          // free heap-allocated char * fields
entity_copy(&dst, &src);  // deep-copy char * fields
entity_list_free(&list);  // free entire list
```

These functions **no longer exist**.  RAII via `std::string` and
`std::vector` destructors handles all cleanup automatically.  Simply let
`Entity` and `EntityList` values go out of scope.

### 5.4 TripletStore API: C++ only

Use the C++ API (`triplet_store.hpp` / `vibe::TripletStore`) in all code.
The legacy C wrapper (`triplet_store_c.h`) has been removed.

**C++ API:**

```cpp
#include "triplet_store.hpp"

vibe::TripletStore store;
store.add("REQ-SW-001", "derived-from", "REQ-SYS-005");
store.infer_inverses();

auto links = store.find_by_subject("REQ-SW-001");
for (const auto *t : links) {
    printf("%s -[%s]-> %s\n",
           t->subject.c_str(),
           t->predicate.c_str(),
           t->object.c_str());
}
// No explicit cleanup — RAII handles it
```

---

## 6. Migration checklist for packagers

- [ ] Update build dependencies: require **GCC ≥ 13**, **Clang ≥ 17**, or
      **MSVC 19.38+** in your package metadata.
- [ ] Remove any `CC` / `CFLAGS` overrides in your build recipe (no C files
      remain).
- [ ] Ensure the link step uses `g++` / `clang++` (not `gcc`).
- [ ] Run `make check-compiler` as an optional pre-build sanity check.
- [ ] Remove `libc` / `libstdc++` version pins that were based on C11 ABI
      assumptions — the binary now links the C++23 standard library.
- [ ] Update CI images to use Ubuntu 24.04 LTS (ships GCC 13 by default) or
      install GCC 13 via the `ubuntu-toolchain-r/test` PPA on older releases.

---

## 7. Migration checklist for contributors

- [ ] Ensure your local compiler is GCC ≥ 13 or Clang ≥ 17 (`make check-compiler`).
- [ ] Replace any `char id[N]` / `char title[N]` field accesses with
      `std::string` access.
- [ ] Replace `entity_list_init()` / `entity_list_add()` / `entity_list_free()`
      with `std::vector<Entity>` operations.
- [ ] Remove calls to `entity_free()` and `entity_copy()` (no longer exist).
- [ ] Prefer the C++ TripletStore API (`triplet_store.hpp`) in all internal code.
- [ ] Use RAII: do not manually `free()` strings or arrays that were previously
      heap-allocated by the old C helpers.
- [ ] Update any patch or branch that adds a new component: add a
      `std::string` or `std::vector<std::string>` field to the component
      struct — no `char[]` buffer sizing arithmetic needed.
- [ ] When passing `std::string` to legacy `const char *` parameters, use
      `.c_str()`.

---

## 8. Frequently asked questions

**Q: Can I still call the tool's API from a C project?**

No.  The `triplet_store_c.h` C wrapper has been removed.  All internal code
uses the C++ API (`triplet_store.hpp` / `vibe::TripletStore`) directly.
Embedding the tool's internals in a C project is not supported.

**Q: Are there ABI compatibility guarantees?**

No.  The library is an implementation detail of the `vibe-req` binary and
does not promise ABI stability between releases.

**Q: What happened to `entity_free()` and `entity_copy()`?**

They were removed as part of the C→C++23 migration.  RAII via `std::string`
and `std::vector` destructors replaces them entirely.  If your code calls
these functions it needs to be updated to remove those calls.

**Q: What happened to `yaml_helpers.c` / `yaml_helpers.o`?**

`yaml_helpers.c` was removed as dead code during the migration.  Key
normalisation (converting `_` YAML key separators to `-`) is now handled
directly inside `yaml/entity_parser.cpp`.

**Q: I get a compiler error `'entity_free' was not declared in this scope`.**

This function was removed.  Delete the call site; the `Entity` destructor
handles cleanup automatically.

**Q: I get `error: ISO C++23 requires …` or similar.**

Your compiler does not support C++23.  Install a newer compiler and verify
with `make check-compiler`.  See [Section 3](#3-toolchain-requirements) for
platform-specific installation instructions.
