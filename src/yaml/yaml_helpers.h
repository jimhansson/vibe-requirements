#ifndef VIBE_YAML_HELPERS_H
#define VIBE_YAML_HELPERS_H

/*
 * @file yaml_helpers.h
 * @brief Internal buffer-handling and YAML-sequence helper declarations.
 *
 * These utilities are shared by the entity and link parser modules.  They
 * are NOT part of the public API — consumers should include yaml_simple.h.
 */

#include <stddef.h>
#include <yaml.h>

/* -------------------------------------------------------------------------
 * Buffer-size constants for common YAML field targets.
 * ---------------------------------------------------------------------- */

/** Maximum length (including NUL) of a link target (entity ID or artefact). */
#define LINK_TARGET_LEN   256

/** Maximum length (including NUL) of a relation name. */
#define LINK_RELATION_LEN 128

/** Maximum lengths for test-procedure step sub-fields. */
#define STEP_ACTION_LEN  512
#define STEP_OUTPUT_LEN  512

/** Maximum lengths for clause-collection sub-fields. */
#define CLAUSE_ID_LEN    128
#define CLAUSE_TITLE_LEN 256

/** Maximum lengths for attachment sub-fields. */
#define ATTACH_PATH_LEN  512
#define ATTACH_DESC_LEN  512

/* -------------------------------------------------------------------------
 * String / buffer helpers
 * ---------------------------------------------------------------------- */

/*
 * Copy at most (dst_size - 1) bytes from src into dst and NUL-terminate.
 * Truncates silently when src is longer than the buffer.
 */
void yaml_copy_field(char *dst, size_t dst_size, const char *src);

/*
 * Append text to a newline-separated flat buffer.
 * Returns 1 if the text was appended, 0 if there was not enough space.
 */
int yaml_append_to_flat(char *buf, size_t buf_size, int *count,
                        const char *text);

/*
 * Append a tab-separated "target\trelation" entry to a newline-separated
 * flat buffer.
 * Returns 1 if the entry was appended, 0 if there was not enough space.
 */
int yaml_append_pair_entry(char *buf, size_t buf_size, int *count,
                           const char *first, const char *second);

/*
 * Walk a YAML sequence node and collect scalar items into a flat buffer,
 * using yaml_append_to_flat().
 */
void yaml_collect_sequence(yaml_document_t *doc, yaml_node_t *seq,
                            char *buf, size_t buf_size, int *count);

#endif /* VIBE_YAML_HELPERS_H */
