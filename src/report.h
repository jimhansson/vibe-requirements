#ifndef VIBE_REPORT_H
#define VIBE_REPORT_H

#include <stdio.h>
#include "entity.h"
#include "triplet_store_c.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Output format for report_write(). */
typedef enum {
    REPORT_FORMAT_MARKDOWN = 0, /**< Markdown text (default)               */
    REPORT_FORMAT_HTML          /**< Self-contained HTML with inline CSS    */
} ReportFormat;

/**
 * Write a human-readable report of *list to *out.
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
 * @param fmt    output format (REPORT_FORMAT_MARKDOWN or REPORT_FORMAT_HTML)
 */
void report_write(FILE *out, const EntityList *list,
                  const TripletStore *store, ReportFormat fmt);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VIBE_REPORT_H */
