#ifndef VIBE_YAML_SIMPLE_H
#define VIBE_YAML_SIMPLE_H

#include "requirement.h"

/*
 * Parse a single YAML file and populate *out with any recognized
 * requirement fields found at the top level.
 *
 * Returns  0 on success (file has at least a top-level "id:" field).
 * Returns -1 if the file cannot be opened or contains no "id:" field.
 */
int yaml_parse_requirement(const char *path, Requirement *out);

#endif /* VIBE_YAML_SIMPLE_H */
