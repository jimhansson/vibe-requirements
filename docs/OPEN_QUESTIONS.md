# Open Questions

1. **Primary implementation language** — After prototyping, which language (Rust, Go, C++, Common Lisp) offers the best balance of developer experience, binary portability, and GUI capability?
2. **Default file format** — Should YAML be the only format in Phase 1, or should TOML be supported in parallel from the start?
3. **ID scheme** — Should IDs be fully manual (engineer chooses `REQ-SW-001`) or partially auto-generated (tool assigns the next available number within a prefix)?
4. **ReqIF export** — ReqIF is the ISO standard interchange format for requirements. Is this a priority for the target projects?
5. **Multi-project monorepo** — Should one repository be able to host multiple independent projects, each with their own ID namespace and configuration?
6. **Hardware CAD integration** — Which CAD/EDA file formats should be targeted for hardware artefact links (e.g., KiCad, Altium, FreeCAD)?
7. **Scripting language choice** — Should the embedded scripting layer use Lua (low overhead, procedural, easy to embed) or GNU Guile (expressive Scheme, author-familiar, larger footprint)? See [SCRIPTING.md](SCRIPTING.md) for full analysis. The recommendation is to start with Lua and optionally add Guile later; the final decision depends on the implementation language chosen for the tool itself.
8. **Script sandboxing** — Should user scripts have read-only access to the requirement model only (safe, sandboxed), or should they have unrestricted OS access (powerful but potentially unsafe)?
9. **Optional scripting build flag** — Should the scripting layer be compiled in by default, or be an optional feature to keep the binary smaller for users who do not need it?
