### Cleanup task 1: Reduce duplicated control flow in `src/main.cpp` (centralize TripletStore lifecycle and output handling)

**Why**
`src/main.cpp` has multiple command branches that repeat the same patterns:
- build/destroy `TripletStore *store`
- open/close an optional output file
- similar error handling / early returns

This repetition increases code size and makes future changes error-prone.

**What to do**
- Introduce small helper(s), e.g.
  - `with_relation_store(elist, lambda)` that creates/destroys the store and handles the null-store error once.
  - `with_output_file(path, lambda)` that opens/closes a `FILE*` and handles fopen error once.
- Refactor the following branches to use the helper(s):
  - `--report`
  - `doc <doc-id>`
  - `--trace`, `--coverage`, `--orphan`
  - `--links`, `--strict-links`
- Keep behavior identical (exit codes, stderr messages).

**Acceptance criteria**
- No behavior change (manual smoke test of all commands).
- `src/main.cpp` shrinks and becomes more linear/clear.
- No resource leaks (store and FILE handles always released).

---

### Cleanup task 2: ~~Remove duplicated API docs and constants between C and C++ triplet store interfaces~~ (Completed — C API wrapper removed)

The legacy C wrapper (`src/triplet_store_c.h` and `src/triplet_store_c.cpp`)
has been deleted.  All internal code now uses `vibe::TripletStore` (C++ API)
directly.  The duplicated constants, docs, and `CTripleList` type are gone.

---

### Cleanup task 3: Consolidate command filtering logic to a single helper (avoid repeating filter-condition checks)

**Why**
`src/main.cpp` repeats the same filter-condition checks:
```cpp
if (opts.filter_kind || opts.filter_comp || opts.filter_status || opts.filter_priority) { ... }
```
The repeated condition is noisy and makes it easy to forget a filter field if options expand.

**What to do**
- Add a helper function, e.g. `bool cli_has_entity_filter(const CliOptions& opts);` (in `cli_args.{h,cpp}`), or a method on `CliOptions`.
- Update all call sites in `main.cpp` to use it.
- (Optional) consider a small struct representing the filter to pass around, to improve clarity.

**Acceptance criteria**
- Filter condition appears in one place only.
- No behavior changes to filtering.
- Code reads more clearly around the list/report/doc commands.
