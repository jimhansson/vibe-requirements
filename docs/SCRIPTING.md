# Scripting and Programmability — Research Document

## 1. Motivation

**vibe-requirements** targets a wide range of engineering projects — embedded systems, safety-critical products, standards-compliance workflows — each with its own domain-specific rules, naming conventions, and reporting needs. A hard-coded feature set can never cover every combination of requirements standard, project convention, and output format that real teams need.

The goal of an embedded scripting layer is to let users extend the tool without forking it or waiting for upstream development:

- Write **custom validators** that encode project- or standard-specific rules (e.g., "every `safety` requirement must have a `verification: test` field").
- Write **custom report generators** that produce output formats not built into the tool.
- Write **custom lint rules** that go beyond the built-in orphan and broken-link checks.
- Register **lifecycle hooks** that run before or after `validate`, `report`, or `export` commands.
- Programmatically **query and transform** the in-memory requirement graph.

---

## 2. What Should Be Scriptable

The following areas are candidates for exposure to the scripting layer. This list is a starting point and should be refined during implementation.

| Area | Description | Priority |
|---|---|---|
| **Document validation** | User-defined predicates that run after built-in schema validation; can emit warnings or errors | High |
| **Custom lint rules** | Rules that inspect the full requirement graph (e.g., every approved requirement must have at least one test case) | High |
| **Report templates** | User-defined templates or generators that produce custom Markdown, HTML, or plain-text reports | Medium |
| **Export hooks** | Pre/post hooks around the `export` command to transform or filter data | Medium |
| **ID generation** | Override the default ID-generation strategy with a project-specific function | Low |
| **Link resolution** | Custom resolvers for non-standard artefact references (e.g., Jira ticket IDs, Confluence pages) | Low |

### 2.1 Scripting Entry Points

From the user's perspective, scripts should be loadable in two ways:

1. **Configuration file** — the `.vibe-req.yaml` file can list script files to load at startup.
2. **Convention-based auto-discovery** — the tool automatically loads any scripts found in a designated folder (e.g., `scripts/` or `.vibe-req-scripts/`) inside the repository.

---

## 3. Language Options and Analysis

### 3.1 GNU Guile (Scheme)

**Website:** https://www.gnu.org/software/guile/  
**License:** GNU LGPL v3+

Guile is the official GNU extension language and is specifically designed to be embedded in host applications. It implements the R5RS/R6RS/R7RS Scheme standards with many GNU extensions.

**Advantages:**
- Designed from the ground up as an embeddable extension language; the embedding API is stable and well-documented.
- LGPL v3+ license is compatible with the project's GPL v3 license.
- Full Scheme: hygienic macros, first-class continuations, proper tail recursion — excellent for writing rules and transformations.
- Rich standard library covering strings, lists, I/O, regular expressions, and POSIX interfaces.
- The author has familiarity with GNU Scheme, reducing the learning curve.
- Long history (since 1993); used in GNU Lilypond, GnuCash, GNU Make, and other GNU projects.

**Disadvantages:**
- Adds a non-trivial dependency: Guile is a substantial library (~30 MB shared library or larger static build).
- Static linking of Guile into the final binary is possible but complex; LGPL requires either dynamic linking or providing object files for re-linking.
- Scheme syntax is unfamiliar to most engineers; adoption may require documentation and examples.
- Windows support exists but is less mature than on Linux/macOS.

**Embedding complexity:** Medium. The C API (`scm_boot_guile`, `scm_c_define_gsubr`, etc.) is well-established. Rust or Go bindings would require a C FFI wrapper layer.

---

### 3.2 Lua

**Website:** https://www.lua.org/  
**License:** MIT

Lua is the most widely used embedded scripting language in the industry. It is designed specifically for embedding in host applications.

**Advantages:**
- MIT license — fully compatible with GPL v3.
- Extremely small footprint: the Lua 5.4 source is under 350 KB; the interpreter adds roughly 200–300 KB to a binary.
- Simple, regular syntax — easier for engineers without a Lisp background to learn quickly.
- Excellent embedding API with a well-understood stack-based C interface.
- Very fast for an interpreted language.
- Widely documented; large community.

**Disadvantages:**
- The language is intentionally minimal; complex metaprogramming requires discipline.
- Lacks a rich standard library; file I/O, regex, and JSON support require extra modules (included in LuaJIT or available via LuaRocks).
- Not a Lisp — macros and code-as-data require a different approach.

**Embedding complexity:** Low. Lua's C API is the reference for simplicity.  Rust bindings exist (`mlua`; MIT/Apache-2.0), Go bindings exist (`gopher-lua`; MIT).

---

### 3.3 GNU Guile versus Lua — Side-by-Side

| Criterion | GNU Guile | Lua |
|---|---|---|
| License | LGPL v3+ | MIT |
| GPL v3 compatibility | Yes | Yes |
| Binary size impact | Large (~30 MB) | Very small (~300 KB) |
| Embedding complexity | Medium | Low |
| Syntax familiarity | Low (Scheme/Lisp) | High (procedural) |
| Metaprogramming | Excellent (macros, continuations) | Limited |
| Windows support | Fair | Excellent |
| Rust FFI | Via C wrapper | `mlua` crate (MIT) |
| Go FFI | Via C wrapper | `gopher-lua` (MIT) |

---

### 3.4 Other Candidates

#### Tcl
- **License:** BSD-like (Tcl license) — compatible with GPL v3.
- Very mature, safe-interpreter sandboxing is excellent for untrusted scripts.
- Syntax is unusual; adoption would be low.
- Embedding is well-supported via `libtcl`.

#### Python (embedded via libpython)
- **License:** PSF License — compatible with GPL v3.
- Large standard library, extremely high familiarity.
- Major drawback: conflicts with the project constraint of **no runtime dependency** and **single binary distribution**; embedding CPython is complex and produces a large binary.
- PyPy's embedding story is even more complex.
- Not recommended given the project constraints.

#### JavaScript (QuickJS)
- **Website:** https://bellard.org/quickjs/
- **License:** MIT
- QuickJS is a small, embeddable JavaScript engine that is self-contained and easy to compile statically.
- Very small footprint (~700 KB static library).
- JavaScript is the most widely known scripting language; adoption barrier is low.
- Conflict: the project explicitly disallows JavaScript/Node.js/browser runtimes. Even though QuickJS is a standalone engine with no browser dependency, using JavaScript as the extension language would contradict the project's stated constraint and philosophy.
- **Not recommended** due to this philosophical conflict.

#### Custom S-expression DSL
- If the implementation language is Common Lisp or Scheme, the host language itself can serve as the extension language (no separate interpreter needed).
- If the implementation is in Rust or Go, a small S-expression evaluator could be written from scratch.
- Maximum control, minimal binary size, but high implementation cost and limited standard library.

---

## 4. License Compatibility Summary

The project is released under **GNU GPL v3**.

| Language | License | GPL v3 Compatible | Notes |
|---|---|---|---|
| GNU Guile | LGPL v3+ | Yes | LGPL ≥ v3 is compatible with GPL v3 |
| Lua | MIT | Yes | MIT is compatible with GPL v3 |
| Tcl | BSD (Tcl license) | Yes | BSD is compatible with GPL v3 |
| Python (CPython) | PSF | Yes | But binary distribution is impractical |
| QuickJS | MIT | Yes | But violates project JS constraint |
| Custom DSL | — | N/A | No third-party license involved |

All practical candidates are license-compatible with GPL v3.

---

## 5. What Should Be Exposed to Scripts

The scripting API should expose the **in-memory requirement model** after all files have been parsed and linked. A minimal first version of the API should provide:

| API Function / Object | Description |
|---|---|
| `all_requirements()` | Returns a list of all requirement objects |
| `requirement(id)` | Returns the requirement object for the given ID, or nil/null |
| `requirement.id` | The unique ID string |
| `requirement.type` | The type field value |
| `requirement.status` | The status field value |
| `requirement.links` | List of link objects (each has `id` and `relation`) |
| `requirement.tags` | List of tag strings |
| `emit_error(id, message)` | Report a validation error attributed to a specific requirement |
| `emit_warning(id, message)` | Report a validation warning |
| `all_test_cases()` | Returns a list of all test case objects |
| `all_external_sources()` | Returns a list of all external normative source objects |

Scripts are called at defined hook points and receive the model as input. They cannot modify the on-disk files (read-only access to the model is sufficient for validation and reporting).

---

## 6. Recommendation

### Short-term (MVP scripting)

**Adopt Lua** as the embedded scripting language for the initial scripting layer.

Reasons:
1. MIT license — unambiguously compatible with GPL v3.
2. Smallest binary size impact of all practical candidates (~300 KB).
3. Low embedding complexity — Lua's C API is the de facto standard for embedding; mature Rust (`mlua`) and Go (`gopher-lua`) bindings exist.
4. Procedural syntax lowers the barrier for engineers who are not familiar with Lisp.
5. Sufficient expressiveness for the target use cases (validation rules, lint rules, report templates).

The primary use cases (custom validators and lint rules) do not require the advanced metaprogramming features that make Guile compelling. Lua provides a pragmatic, low-overhead path to a first working implementation.

### Long-term (advanced programmability)

Once the tool has an implementation language and a working Lua layer, consider:

- **Adding GNU Guile support** as an alternative scripting backend for users who prefer Scheme. Because Guile is specifically designed for this use case and the author is familiar with it, it remains a strong candidate for a second backend.
- **Exposing a stable scripting API** (documented and versioned) so that scripts are not broken by tool upgrades.
- Investigating whether Guile's ability to use the host language as a DSL (macros, quasiquotation) can enable more expressive requirement validation rules than procedural Lua.

### Decision Point

The choice between Lua (pragmatic, low overhead) and Guile (expressive, author-familiar) should be revisited once the implementation language is chosen:

- If the tool is written in **Rust**: prefer Lua via `mlua`; Guile requires a C FFI wrapper.
- If the tool is written in **Go**: prefer Lua via `gopher-lua`; Guile requires cgo and a C wrapper.
- If the tool is written in **C or C++**: both Lua and Guile are straightforward to embed via their native C APIs; Guile becomes a stronger candidate.
- If the tool is written in **Common Lisp (SBCL)**: no separate interpreter is needed — user scripts are simply Lisp code evaluated by the host runtime.

---

## 7. Open Questions from This Research

1. Should the scripting layer be enabled in all builds, or be an optional compile-time feature to keep the binary smaller for users who do not need it?
2. Should scripts be sandboxed (no filesystem or network access other than read-only access to the requirement model), or should they be trusted code with full OS access?
3. Should a standard library of reusable validation functions (e.g., "every safety requirement has a test case") be shipped with the tool?
4. Should the scripting API be designed against a language-neutral interface description (similar to an IDL) so that multiple backends (Lua, Guile, Tcl) can be supported without duplicating the glue code?
