/**
 * @file validate.h
 * @brief Validation command for vibe-req.
 *
 * Checks the loaded entity list and relation store for consistency problems:
 *   - duplicate entity IDs
 *   - traceability links pointing to unknown entity IDs
 */

#ifndef VIBE_VALIDATE_H
#define VIBE_VALIDATE_H

#include "entity.h"
#include "triplet_store.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Run all validation checks on the entity list and relation store.
 *
 * Every problem found is printed to stderr with an "error:" prefix.
 * A summary line is printed to stdout (clean) or stderr (problems found).
 *
 * @param elist  The full entity list.
 * @param store  The relation triplet store built from elist.
 * @return       Total number of problems found (0 = clean).
 */
int cmd_validate(const EntityList *elist, const vibe::TripletStore *store);

#ifdef __cplusplus
}
#endif

#endif /* VIBE_VALIDATE_H */
