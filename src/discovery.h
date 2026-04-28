#ifndef VIBE_DISCOVERY_H
#define VIBE_DISCOVERY_H

#include "entity.h"
#include "config.h"

/*
 * Recursively walk root_dir and collect all YAML entities from YAML files
 * that contain at least a top-level "id:" field.
 * Hidden directories and files (names starting with '.') are skipped.
 * Directories whose base name appears in cfg->ignore_dirs are also skipped;
 * pass cfg == NULL to disable directory filtering.
 *
 * Returns the total number of entities found, or -1 when root_dir does not
 * exist or is not a directory.
 */
int discover_entities(const char *root_dir, EntityList *list,
                      const VibeConfig *cfg);

/*
 * Variant of discover_entities() with optional fail-fast behavior.
 *
 * When fail_fast is non-zero, discovery stops on the first YAML parse error
 * and returns -2.  When fail_fast is zero, malformed YAML files are reported
 * and skipped while discovery continues.
 */
int discover_entities_with_options(const char *root_dir, EntityList *list,
                                   const VibeConfig *cfg, int fail_fast);

#endif /* VIBE_DISCOVERY_H */
