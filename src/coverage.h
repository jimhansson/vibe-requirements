/**
 * @file coverage.h
 * @brief Coverage and orphan query helpers for the vibe-req CLI.
 *
 * Provides functions for determining whether a requirement is "covered" by
 * at least one traceability link to a test or code implementation, and for
 * identifying entities that carry no traceability links at all.
 *
 * The two command-level renderers (cmd_coverage, cmd_orphan) call the lower-
 * level predicates (entity_is_covered, entity_has_any_link) and print tabular
 * results to stdout.  The predicates are also independently testable.
 */

#ifndef VIBE_COVERAGE_H
#define VIBE_COVERAGE_H

#include "entity.h"
#include "triplet_store_c.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return 1 if @p pred is a coverage-indicating relation type.
 *
 * Coverage relations are those that connect a requirement to a test case or
 * code implementation artifact.  The full set is:
 *   verifies, verified-by, implements, implemented-by, implemented-in,
 *   implemented-by-test, tests, tested-by, satisfies, satisfied-by.
 *
 * @param pred  relation predicate string (must not be NULL)
 * @return      1 if pred is a coverage relation, 0 otherwise
 */
int is_coverage_predicate(const char *pred);

/**
 * Return 1 if the entity identified by @p id has at least one declared
 * (non-inferred) traceability link in @p store that uses a coverage-indicating
 * predicate — either outgoing from @p id or incoming to @p id.
 *
 * @param store  populated TripletStore (must not be NULL)
 * @param id     entity identifier to check (must not be NULL)
 * @return       1 if covered, 0 otherwise
 */
int entity_is_covered(const TripletStore *store, const char *id);

/**
 * Return 1 if the entity identified by @p id has at least one declared
 * (non-inferred) traceability link in @p store in either direction.
 *
 * This is the orphan predicate: an entity is an orphan when this returns 0.
 *
 * @param store  populated TripletStore (must not be NULL)
 * @param id     entity identifier to check (must not be NULL)
 * @return       1 if at least one link exists, 0 if the entity is an orphan
 */
int entity_has_any_link(const TripletStore *store, const char *id);

/**
 * Print a requirement coverage report to stdout.
 *
 * Reports the total number of requirements, how many are linked to tests or
 * code (covered), and lists unlinked requirements in a table.
 *
 * @param elist  full entity list to inspect
 * @param store  populated TripletStore containing all declared links
 */
void cmd_coverage(const EntityList *elist, const TripletStore *store);

/**
 * Print a list of orphaned requirements and test cases to stdout.
 *
 * An entity is considered orphaned when it has no traceability links in
 * either direction.  Only entities of kind ENTITY_KIND_REQUIREMENT and
 * ENTITY_KIND_TEST_CASE are considered.
 *
 * @param elist  full entity list to inspect
 * @param store  populated TripletStore containing all declared links
 */
void cmd_orphan(const EntityList *elist, const TripletStore *store);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VIBE_COVERAGE_H */
