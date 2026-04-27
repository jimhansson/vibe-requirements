#ifndef VIBE_NEW_CMD_H
#define VIBE_NEW_CMD_H


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


#endif /* VIBE_NEW_CMD_H */
