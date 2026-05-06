#ifndef VIBE_NEW_CMD_H
#define VIBE_NEW_CMD_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Scaffold a new entity YAML file.
 *
 * Creates a file named `<id>.yaml` in `dir` (default: current directory)
 * containing a minimal template for the given entity type.
 *
 * @param type_str  Entity type string (e.g. "requirement", "story", …)
 * @param id        Entity identifier (used as filename and `id:` field)
 * @param dir       Output directory; if NULL or empty, uses "."
 *
 * @return  0 on success
 *         -1 if the file already exists
 *         -2 if the output file cannot be opened for writing
 *         -3 if the type string is unrecognised
 */
int new_cmd_scaffold(const char *type_str, const char *id, const char *dir);

/**
 * Suggest the next unused numeric ID for the given type and prefix.
 *
 * Scans the YAML files under @p dir and finds the largest numeric suffix
 * used by entities of the given kind whose IDs start with @p prefix.
 * The next ID is returned in @p out (e.g., "REQ-042").
 *
 * @param type_str  Entity type string (e.g. "requirement", "story", …)
 * @param prefix    ID prefix to match (e.g. "REQ-"); if NULL or empty,
 *                  a default prefix is selected based on @p type_str
 * @param dir       Root directory to scan; if NULL or empty, uses "."
 * @param out       Output buffer to receive the suggested ID
 * @param out_len   Size of the output buffer
 *
 * @return  0 on success
 *         -1 if the directory cannot be scanned
 *         -2 if the output buffer is too small
 *         -3 if the type string is unrecognised
 *         -4 if no prefix could be determined
 */
int new_cmd_next_id(const char *type_str, const char *prefix, const char *dir,
                    char *out, size_t out_len);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VIBE_NEW_CMD_H */
