#ifndef VIBE_DISCOVERY_H
#define VIBE_DISCOVERY_H

#include "entity.h"
#include "config.h"

/*
 * Recursively walk root_dir and collect all YAML files that can be parsed
 * as entities (i.e., they contain at least a top-level "id:" field).
 * Hidden directories and files (names starting with '.') are skipped.
 * Directories whose base name appears in cfg->ignore_dirs are also skipped;
 * pass cfg == NULL to disable directory filtering.
 *
 * Returns the total number of entities found, or -1 on a fatal error
 * (e.g., root_dir does not exist or is not a directory).
 */
int discover_entities(const char *root_dir, EntityList *list,
                      const VibeConfig *cfg);

#endif /* VIBE_DISCOVERY_H */
