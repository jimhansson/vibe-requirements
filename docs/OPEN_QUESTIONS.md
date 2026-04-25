# Open Questions

- **Primary implementation language** — After prototyping, which language (Rust, Go, C++, Common Lisp) offers the best balance of developer experience, binary portability, and GUI capability?

  - **Binary Interface** — Should a core of programs be in a seperated library that can be used by both programs and maybe even have bindings for many languages
  
- **ID scheme** — Should IDs be fully manual (engineer chooses `REQ-SW-001`) or partially auto-generated (tool assigns the next available number within a prefix)?

- **ReqIF export** — ReqIF is the ISO standard interchange format for requirements. Is this a priority for the target projects?

- **Multi-project monorepo** — Should one repository be able to host multiple independent projects, each with their own ID namespace and configuration?

  - what about allowing multiple projects that are related (or not)
    where requirements can be linked between

- **Bidirectional links — explicit vs. inferred** (issue #49) — Should users declare links in both directions, or should the tool infer the reverse automatically?

  **Background**

  The current model stores directed triples `(subject, predicate, object)`. The triplet store already has both `find_by_subject` and `find_by_object` indexes, so in-memory navigation is bidirectional regardless of how links are declared. The question is about the authoring experience in YAML files and what validation to apply.

  **Three main approaches**

  | Approach | Description |
  |---|---|
  | **A — Explicit bidirectional** | User must write a link entry in *both* the subject file and the object file. Tool validates that the inverse exists. |
  | **B — Inferred inverse** | User writes the link once; the tool auto-generates the inverse triple at load time using a predefined relation-pair registry. |
  | **C — Unidirectional (current)** | User writes the link once, on whichever side is most natural. The tool can query it in both directions via the object index. No inverse triple is generated or required. |

  **Pros and cons**

  *Option A — Explicit bidirectional*
  - ✅ Both files are self-contained; a diff on either file shows the relationship.
  - ✅ Ownership is unambiguous: each team member or work-item "acknowledges" the link.
  - ✅ No relation-pair registry needed.
  - ❌ Double maintenance burden; easy to update one side and forget the other.
  - ❌ Merge conflicts more likely (two files change for one logical link).
  - ❌ In regulated projects the inconsistency (A→B exists but B→A missing) can produce spurious audit findings.

  *Option B — Inferred inverse*
  - ✅ Single source of truth in the YAML files.
  - ✅ Always consistent; no risk of one-sided links.
  - ✅ Works well with standard relation pairs already used in practice: `verifies` ↔ `verified-by`, `derives-from` ↔ `derived-to`, `parent` ↔ `child`, `implements` ↔ `implemented-by`, `conflicts-with` ↔ `conflicts-with` (symmetric).
  - ❌ Requires maintaining a **relation-pair registry** (built-in table + optional project-level extension).
  - ❌ Inferred triples in the store can feel "magic" if the relation pair is wrong or missing.
  - ❌ Custom or free-form relation names have no known inverse; the tool must fall back to Option C for those.

  *Option C — Unidirectional (current)*
  - ✅ Simplest mental model; no registry, no extra YAML.
  - ✅ Closest to how semantic-web / RDF stores are used.
  - ✅ Already implemented and working.
  - ❌ The natural authoring side varies by relation type, so teams may be inconsistent.
  - ❌ Impact analysis requires querying by object explicitly; GUI and reports must make that step transparent.

  **How mainstream tools handle this**

  | Tool | Approach |
  |---|---|
  | IBM DOORS / DOORS Next | Directed links declared once; link module shows both ends; no inverse required in files. |
  | Polarion ALM | Directed links; built-in "back-links" view computed from object index; no explicit inverse. |
  | Jama Connect | Directed traceability; navigation in both directions is a UI feature, not a file feature. |
  | ReqIF (ISO 26531) | Links are directed; consumers may display them bidirectionally; no mandatory inverse. |
  | Capella / MBSE tools | Explicit directed relations; inverse displayed in property view. |

  **Recommendation**

  Start with **Option B** (inferred inverse) as the default, with **Option C** as the silent fallback for unknown relation names:

  1. Ship a small built-in **relation-pair table** covering the common relation names (see below).
  2. At load time, for each stored triple `(A, rel, B)` where `rel` has a known inverse `inv(rel)`, automatically add `(B, inv(rel), A)` as a *synthetic* (not persisted) triple. Mark synthetic triples so reporters and validators can distinguish them from user-declared ones.
  3. For unknown or custom relation names, store only the user-declared direction (Option C fallback).
  4. Add an optional `--strict-links` validation mode that warns when a user has explicitly declared only one direction of a relation that is in the pair table — this is useful for regulated environments that require explicit acknowledgement on both sides.

  **Proposed built-in relation-pair table**

  | Forward relation | Inverse relation |
  |---|---|
  | `derives-from` | `derived-to` |
  | `parent` | `child` |
  | `verifies` | `verified-by` |
  | `implements` | `implemented-by` |
  | `implemented-in` | `implemented-by-artefact` |
  | `conflicts-with` | `conflicts-with` (symmetric) |
  | `refines` | `refined-by` |
  | `traces-to` | `traced-from` |
  | `part-of` | `contains` |
  | `satisfies` | `satisfied-by` |
  | `tests` | `tested-by` |

  The table should be extensible via `.vibe-req.yaml` so projects can add domain-specific pairs.

- **Document membership as a graph edge** — Should `documents:` membership be modelled exclusively as an ECS component, or also projected into the TripletStore as `part-of` relations?

  **Background**

  Entities can declare their parent documents via the `documents:` YAML key, which populates `DocumentMembershipComponent`.  This component enables the `--component doc-membership` filter and the `entity_has_component()` API.  However, it is *not* visible to the TripletStore, so membership-based queries (e.g. "which entities are in SRS-CLIENT-001?") cannot be expressed through the unified graph API.

  **Decision (implemented)**

  Keep `DocumentMembershipComponent` for backward compatibility and CLI filter support.  Additionally, call `entity_doc_membership_to_triplets()` inside `build_entity_relation_store()` to project every `documents:` entry as a `(entity_id, "part-of", doc_id)` triple.  `infer_inverses()` then generates the matching `(doc_id, "contains", entity_id)` triple automatically.

  This approach unifies document membership with the general traceability graph without requiring any YAML migration.

  **Tradeoffs**

  | Concern | Notes |
  |---|---|
  | Duplication | Each membership appears in both `DocumentMembershipComponent` and the TripletStore. Harmless — the two representations serve different callers. |
  | `--component doc-membership` filter | Continues to work via the ECS component (unchanged). |
  | TripletStore queries | Now work for document membership via `part-of`/`contains` predicates. |
  | `--strict-links` validation | Users who write explicit `traceability: [{relation: part-of}]` entries will be checked for the missing inverse `contains` declaration, just like any other known relation pair. |
  | Migration | Not required. Both authoring styles (`documents:` key and `traceability: [{relation: part-of}]`) are fully supported and produce TripletStore entries. |

- **Hardware CAD integration** — Which CAD/EDA file formats should be targeted for hardware artefact links (e.g., KiCad, Altium, FreeCAD)?

- **Scripting language choice** — Should the embedded scripting layer use Lua (low overhead, procedural, easy to embed) or GNU Guile (expressive Scheme, author-familiar, larger footprint)? See [SCRIPTING.md](SCRIPTING.md) for full analysis. The recommendation is to start with Lua and optionally add Guile later; the final decision depends on the implementation language chosen for the tool itself.
  
  - **Script sandboxing** — Should user scripts have read-only access to the requirement model only (safe, sandboxed), or should they have unrestricted OS access (powerful but potentially unsafe)?

  - **Optional scripting build flag** — Should the scripting layer be compiled in by default, or be an optional feature to keep the binary smaller for users who do not need it?
