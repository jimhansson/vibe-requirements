# Open Questions

1. **Primary implementation language** — After prototyping, which language (Rust, Go, C++, Common Lisp) offers the best balance of developer experience, binary portability, and GUI capability?
2. **Default file format** — Should YAML be the only format in Phase 1, or should TOML be supported in parallel from the start?
3. **ID scheme** — Should IDs be fully manual (engineer chooses `REQ-SW-001`) or partially auto-generated (tool assigns the next available number within a prefix)?
4. **ReqIF export** — ReqIF is the ISO standard interchange format for requirements. Is this a priority for the target projects?
5. **Multi-project monorepo** — Should one repository be able to host multiple independent projects, each with their own ID namespace and configuration?
6. **Hardware CAD integration** — Which CAD/EDA file formats should be targeted for hardware artefact links (e.g., KiCad, Altium, FreeCAD)?
