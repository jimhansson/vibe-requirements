#ifndef VIBE_REPORT_H
#define VIBE_REPORT_H

#include <stdio.h>
#include "entity.h"
#include "triplet_store_c.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Write a human-readable Markdown report of *list to *out.
 *
 * Entities are grouped by kind.  Each entity section includes:
 *   - ID and title heading
 *   - Kind, status, priority, and owner metadata
 *   - Description and rationale
 *   - Tags
 *   - Traceability links (outgoing and incoming, from *store)
 *
 * When store is NULL traceability sections are omitted.
 *
 * @param out    destination stream (must be open for writing)
 * @param list   entity list to render (may be a filtered subset)
 * @param store  relation store for traceability links (may be NULL)
 */
void report_write(FILE *out, const EntityList *list,
                  const TripletStore *store);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VIBE_REPORT_H */
