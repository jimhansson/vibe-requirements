#ifndef VIBE_REPORT_H
#define VIBE_REPORT_H

#include <cstdio>
#include "entity.h"
#include "triplet_store.hpp"

/** Output format for report_write(). */
typedef enum {
    REPORT_FORMAT_MARKDOWN = 0,
    REPORT_FORMAT_HTML
} ReportFormat;

void report_write(FILE *out, const EntityList *list,
                  const vibe::TripletStore *store, ReportFormat fmt);

#endif /* VIBE_REPORT_H */
