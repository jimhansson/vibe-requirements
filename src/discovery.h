#ifndef VIBE_DISCOVERY_H
#define VIBE_DISCOVERY_H

#include "requirement.h"

/*
 * Recursively walk root_dir and collect all YAML files that can be parsed
 * as requirements (i.e., they contain at least a top-level "id:" field).
 * Hidden directories and files (names starting with '.') are skipped.
 *
 * Returns the total number of requirements found, or -1 on a fatal error
 * (e.g., root_dir does not exist or is not a directory).
 */
int discover_requirements(const char *root_dir, RequirementList *list);

#endif /* VIBE_DISCOVERY_H */
