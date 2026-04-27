#ifndef VIBE_REPORT_H
#define VIBE_REPORT_H

#ifdef __cplusplus
#include <cstdio>
#include "entity.h"
#include "triplet_store_c.h"
#else
#include <stdio.h>
#endif

/** Output format for report_write(). */
typedef enum {
    REPORT_FORMAT_MARKDOWN = 0,
    REPORT_FORMAT_HTML
} ReportFormat;

#ifdef __cplusplus
void report_write(FILE *out, const EntityList *list,
                  const TripletStore *store, ReportFormat fmt);
#endif

#endif /* VIBE_REPORT_H */
