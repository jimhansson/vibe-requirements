#ifndef VIBE_YAML_SIMPLE_H
#define VIBE_YAML_SIMPLE_H

#include "requirement.h"
#include "triplet_store_c.h"

/*
 * Parse a single YAML file and populate *out with any recognized
 * requirement fields found at the top level.
 *
 * Returns  0 on success (file has at least a top-level "id:" field).
 * Returns -1 if the file cannot be opened or contains no "id:" field.
 *
 * NOTE: Only the first YAML document in the file is examined.
 *       Use yaml_parse_requirements() to handle multi-document files.
 */
int yaml_parse_requirement(const char *path, Requirement *out);

/*
 * Parse all YAML documents in a file (multi-document streams are
 * supported via "---" separators) and append every document that
 * contains at least a top-level "id:" field to *list.
 *
 * Returns the number of requirements appended (>= 0).
 * Returns -1 if the file cannot be opened.
 */
int yaml_parse_requirements(const char *path, RequirementList *list);

/*
 * Parse the top-level "links" sequence of a single YAML requirement file
 * and insert each link as a (subject_id, relation, target) triple into
 * store.
 *
 * Each YAML link entry is expected to have the form:
 *   - id: <target-entity-id>   (or artefact: <path>)
 *     relation: <relation-name>
 *
 * Returns the number of triples successfully added (>= 0).
 * Returns -1 if the file cannot be opened or parsed.
 */
int yaml_parse_links(const char *path, const char *subject_id,
                     TripletStore *store);

#endif /* VIBE_YAML_SIMPLE_H */
