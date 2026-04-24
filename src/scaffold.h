#ifndef VIBE_SCAFFOLD_H
#define VIBE_SCAFFOLD_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Write a YAML scaffold template for the given entity type and ID to *out.
 *
 * The template is appropriate for the requested entity kind.  All placeholder
 * fields contain clearly visible TODO markers so the user knows what to fill
 * in.
 *
 * Recognised type strings (same aliases as entity_kind_from_string):
 *   requirement, functional, non-functional, nonfunctional
 *   group
 *   story, user-story
 *   design-note, design_note, design
 *   section
 *   assumption
 *   constraint
 *   test-case, test_case, test
 *   external, directive, standard, regulation
 *   document, srs, sdd
 *
 * @param out   destination stream (must be open for writing)
 * @param type  entity type string (e.g. "requirement", "story")
 * @param id    entity ID to embed in the scaffold (e.g. "REQ-AUTH-001")
 * @return  0 on success, -1 if type is unrecognised
 */
int scaffold_write(FILE *out, const char *type, const char *id);

/**
 * Create a new YAML scaffold file for the given entity type and ID.
 *
 * If out_path is NULL the file is written to "./<id>.yaml" in the current
 * working directory.  If out_path is not NULL the file is written to that
 * path.
 *
 * Fails (returns 1) when:
 *   - type is unrecognised
 *   - the target file already exists (to prevent accidental overwrites)
 *   - the file cannot be opened for writing
 *
 * On success prints a one-line confirmation to stdout and returns 0.
 *
 * @param type      entity type string
 * @param id        entity ID
 * @param out_path  output file path, or NULL to derive from id
 * @return  0 on success, 1 on error
 */
int cmd_new(const char *type, const char *id, const char *out_path);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VIBE_SCAFFOLD_H */
