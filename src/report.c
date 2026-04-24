#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "report.h"
#include "entity.h"
#include "triplet_store_c.h"

/* -----------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------- */

/* Ordered list of entity kinds used to group the report sections. */
static const EntityKind KIND_ORDER[] = {
    ENTITY_KIND_DOCUMENT,
    ENTITY_KIND_REQUIREMENT,
    ENTITY_KIND_GROUP,
    ENTITY_KIND_STORY,
    ENTITY_KIND_DESIGN_NOTE,
    ENTITY_KIND_SECTION,
    ENTITY_KIND_ASSUMPTION,
    ENTITY_KIND_CONSTRAINT,
    ENTITY_KIND_TEST_CASE,
    ENTITY_KIND_EXTERNAL,
    ENTITY_KIND_UNKNOWN,
};
#define KIND_ORDER_LEN ((int)(sizeof(KIND_ORDER) / sizeof(KIND_ORDER[0])))

/* Return 1 if any entity in *list has the given kind. */
static int list_has_kind(const EntityList *list, EntityKind kind)
{
    for (int i = 0; i < list->count; i++) {
        if (list->items[i].identity.kind == kind)
            return 1;
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * Markdown helpers
 * --------------------------------------------------------------------- */

/* Emit a newline-separated multi-value field (tags, sources, …) as a
 * Markdown bullet list.  `label` is the bold heading text.             */
static void md_bullet_list(FILE *out, const char *label, const char *flat,
                            int count)
{
    if (count == 0 || flat[0] == '\0')
        return;

    fprintf(out, "**%s:**\n\n", label);

    /* Iterate over newline-separated entries. */
    const char *p = flat;
    while (*p) {
        const char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        if (len > 0)
            fprintf(out, "- %.*s\n", (int)len, p);
        if (!nl)
            break;
        p = nl + 1;
    }
    fprintf(out, "\n");
}

/* Emit a single Markdown entity section (### heading + body). */
static void md_write_entity(FILE *out, const Entity *e,
                             const TripletStore *store)
{
    /* ---- Heading ---- */
    if (e->identity.title[0] != '\0')
        fprintf(out, "### %s — %s\n\n", e->identity.id, e->identity.title);
    else
        fprintf(out, "### %s\n\n", e->identity.id);

    /* ---- Meta line ---- */
    fprintf(out, "**Kind:** %s", entity_kind_label(e->identity.kind));
    if (e->lifecycle.status[0] != '\0')
        fprintf(out, " | **Status:** %s", e->lifecycle.status);
    if (e->lifecycle.priority[0] != '\0')
        fprintf(out, " | **Priority:** %s", e->lifecycle.priority);
    if (e->lifecycle.owner[0] != '\0')
        fprintf(out, " | **Owner:** %s", e->lifecycle.owner);
    fprintf(out, "\n\n");

    /* ---- Description / rationale ---- */
    if (e->text.description[0] != '\0')
        fprintf(out, "%s\n\n", e->text.description);
    if (e->text.rationale[0] != '\0')
        fprintf(out, "**Rationale:** %s\n\n", e->text.rationale);

    /* ---- Document body (design notes / sections) ---- */
    if (e->doc_body.body[0] != '\0')
        fprintf(out, "%s\n\n", e->doc_body.body);

    /* ---- Tags ---- */
    md_bullet_list(out, "Tags", e->tags.tags, e->tags.count);

    /* ---- Sources ---- */
    md_bullet_list(out, "Sources", e->sources.sources, e->sources.count);

    /* ---- Acceptance criteria ---- */
    md_bullet_list(out, "Acceptance Criteria",
                   e->acceptance_criteria.criteria,
                   e->acceptance_criteria.count);

    /* ---- User story ---- */
    if (e->user_story.role[0] != '\0' || e->user_story.goal[0] != '\0') {
        fprintf(out, "**User Story:**\n\n");
        if (e->user_story.role[0] != '\0')
            fprintf(out, "- As a: %s\n", e->user_story.role);
        if (e->user_story.goal[0] != '\0')
            fprintf(out, "- I want: %s\n", e->user_story.goal);
        if (e->user_story.reason[0] != '\0')
            fprintf(out, "- So that: %s\n", e->user_story.reason);
        fprintf(out, "\n");
    }

    /* ---- Traceability links ---- */
    if (store) {
        CTripleList out_links = triplet_store_find_by_subject(store,
                                                              e->identity.id);
        CTripleList in_links  = triplet_store_find_by_object(store,
                                                             e->identity.id);
        int has_out = 0, has_in = 0;
        for (size_t i = 0; i < out_links.count; i++)
            if (!out_links.triples[i].inferred) { has_out = 1; break; }
        for (size_t i = 0; i < in_links.count; i++)
            if (!in_links.triples[i].inferred) { has_in = 1; break; }

        if (has_out || has_in) {
            fprintf(out, "**Traceability:**\n\n");
            for (size_t i = 0; i < out_links.count; i++) {
                if (!out_links.triples[i].inferred)
                    fprintf(out, "- `[%s]` → %s\n",
                            out_links.triples[i].predicate,
                            out_links.triples[i].object);
            }
            for (size_t i = 0; i < in_links.count; i++) {
                if (!in_links.triples[i].inferred)
                    fprintf(out, "- `[%s]` ← %s\n",
                            in_links.triples[i].predicate,
                            in_links.triples[i].subject);
            }
            fprintf(out, "\n");
        }

        triplet_store_list_free(out_links);
        triplet_store_list_free(in_links);
    }

    fprintf(out, "---\n\n");
}

/* -----------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */

void report_write(FILE *out, const EntityList *list,
                  const TripletStore *store)
{
    fprintf(out, "# Requirements Report\n\n");

    /* ------------------------------------------------------------------ */
    /* Iterate over entity kinds in display order                          */
    /* ------------------------------------------------------------------ */
    for (int ki = 0; ki < KIND_ORDER_LEN; ki++) {
        EntityKind kind = KIND_ORDER[ki];

        if (!list_has_kind(list, kind))
            continue;

        /* Count entities of this kind. */
        int section_count = 0;
        for (int i = 0; i < list->count; i++) {
            if (list->items[i].identity.kind == kind)
                section_count++;
        }

        const char *label = entity_kind_label(kind);

        /* Section heading: capitalise first letter */
        char heading[64];
        strncpy(heading, label, sizeof(heading) - 1);
        heading[sizeof(heading) - 1] = '\0';
        heading[0] = (char)toupper((unsigned char)heading[0]);
        fprintf(out, "## %ss (%d)\n\n", heading, section_count);

        /* Entities in this kind section */
        for (int i = 0; i < list->count; i++) {
            const Entity *e = &list->items[i];
            if (e->identity.kind != kind)
                continue;

            md_write_entity(out, e, store);
        }
    }
}
