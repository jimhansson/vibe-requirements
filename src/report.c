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
 * HTML helpers
 * --------------------------------------------------------------------- */

/* Write `text` to `out` with HTML special characters escaped. */
static void html_escape(FILE *out, const char *text)
{
    for (const char *p = text; *p; p++) {
        switch (*p) {
            case '&':  fputs("&amp;",  out); break;
            case '<':  fputs("&lt;",   out); break;
            case '>':  fputs("&gt;",   out); break;
            case '"':  fputs("&quot;", out); break;
            case '\'': fputs("&#39;",  out); break;
            default:   fputc(*p, out);       break;
        }
    }
}

/* Emit a newline-separated multi-value field as an HTML unordered list. */
static void html_bullet_list(FILE *out, const char *label, const char *flat,
                              int count)
{
    if (count == 0 || flat[0] == '\0')
        return;

    fprintf(out, "<p><strong>%s:</strong></p>\n<ul>\n", label);

    const char *p = flat;
    while (*p) {
        const char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        if (len > 0) {
            char entry[512];
            size_t copy_len = len < sizeof(entry) - 1 ? len : sizeof(entry) - 1;
            memcpy(entry, p, copy_len);
            entry[copy_len] = '\0';
            fprintf(out, "<li>");
            html_escape(out, entry);
            fputs("</li>\n", out);
        }
        if (!nl)
            break;
        p = nl + 1;
    }
    fputs("</ul>\n", out);
}

/* Emit a single HTML entity section. */
static void html_write_entity(FILE *out, const Entity *e,
                               const TripletStore *store)
{
    fprintf(out, "<section class=\"entity\" id=\"");
    html_escape(out, e->identity.id);
    fprintf(out, "\">\n");

    /* ---- Heading ---- */
    fprintf(out, "<h3>");
    html_escape(out, e->identity.id);
    if (e->identity.title[0] != '\0') {
        fprintf(out, " — ");
        html_escape(out, e->identity.title);
    }
    fprintf(out, "</h3>\n");

    /* ---- Meta ---- */
    fprintf(out, "<p><strong>Kind:</strong> %s",
            entity_kind_label(e->identity.kind));
    if (e->lifecycle.status[0] != '\0') {
        fprintf(out, " | <strong>Status:</strong> ");
        html_escape(out, e->lifecycle.status);
    }
    if (e->lifecycle.priority[0] != '\0') {
        fprintf(out, " | <strong>Priority:</strong> ");
        html_escape(out, e->lifecycle.priority);
    }
    if (e->lifecycle.owner[0] != '\0') {
        fprintf(out, " | <strong>Owner:</strong> ");
        html_escape(out, e->lifecycle.owner);
    }
    fprintf(out, "</p>\n");

    /* ---- Description / rationale ---- */
    if (e->text.description[0] != '\0') {
        fprintf(out, "<p>");
        html_escape(out, e->text.description);
        fprintf(out, "</p>\n");
    }
    if (e->text.rationale[0] != '\0') {
        fprintf(out, "<p><strong>Rationale:</strong> ");
        html_escape(out, e->text.rationale);
        fprintf(out, "</p>\n");
    }

    /* ---- Document body ---- */
    if (e->doc_body.body[0] != '\0') {
        fprintf(out, "<pre>");
        html_escape(out, e->doc_body.body);
        fprintf(out, "</pre>\n");
    }

    /* ---- Tags / Sources / Acceptance criteria ---- */
    html_bullet_list(out, "Tags",   e->tags.tags,   e->tags.count);
    html_bullet_list(out, "Sources", e->sources.sources, e->sources.count);
    html_bullet_list(out, "Acceptance Criteria",
                     e->acceptance_criteria.criteria,
                     e->acceptance_criteria.count);

    /* ---- User story ---- */
    if (e->user_story.role[0] != '\0' || e->user_story.goal[0] != '\0') {
        fprintf(out, "<p><strong>User Story:</strong></p>\n<ul>\n");
        if (e->user_story.role[0] != '\0') {
            fprintf(out, "<li>As a: ");
            html_escape(out, e->user_story.role);
            fprintf(out, "</li>\n");
        }
        if (e->user_story.goal[0] != '\0') {
            fprintf(out, "<li>I want: ");
            html_escape(out, e->user_story.goal);
            fprintf(out, "</li>\n");
        }
        if (e->user_story.reason[0] != '\0') {
            fprintf(out, "<li>So that: ");
            html_escape(out, e->user_story.reason);
            fprintf(out, "</li>\n");
        }
        fprintf(out, "</ul>\n");
    }

    /* ---- Traceability ---- */
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
            fprintf(out, "<p><strong>Traceability:</strong></p>\n<ul>\n");
            for (size_t i = 0; i < out_links.count; i++) {
                if (!out_links.triples[i].inferred) {
                    fprintf(out, "<li>[%s] → <a href=\"#%s\">%s</a></li>\n",
                            out_links.triples[i].predicate,
                            out_links.triples[i].object,
                            out_links.triples[i].object);
                }
            }
            for (size_t i = 0; i < in_links.count; i++) {
                if (!in_links.triples[i].inferred) {
                    fprintf(out, "<li>[%s] ← <a href=\"#%s\">%s</a></li>\n",
                            in_links.triples[i].predicate,
                            in_links.triples[i].subject,
                            in_links.triples[i].subject);
                }
            }
            fprintf(out, "</ul>\n");
        }

        triplet_store_list_free(out_links);
        triplet_store_list_free(in_links);
    }

    fprintf(out, "</section>\n\n");
}

/* Inline CSS for the HTML report. */
static const char HTML_CSS[] =
    "body{font-family:sans-serif;max-width:960px;margin:0 auto;padding:1rem}"
    "section.entity{border:1px solid #ddd;border-radius:4px;"
    "padding:1rem;margin-bottom:1rem}"
    "h1,h2,h3{color:#333}"
    "h2{border-bottom:2px solid #ddd;padding-bottom:.25rem}"
    "a{color:#0366d6}";

/* -----------------------------------------------------------------------
 * Markdown report
 * --------------------------------------------------------------------- */

static void report_write_md(FILE *out, const EntityList *list,
                             const TripletStore *store)
{
    fprintf(out, "# Requirements Report\n\n");

    for (int ki = 0; ki < KIND_ORDER_LEN; ki++) {
        EntityKind kind = KIND_ORDER[ki];

        if (!list_has_kind(list, kind))
            continue;

        int section_count = 0;
        for (int i = 0; i < list->count; i++) {
            if (list->items[i].identity.kind == kind)
                section_count++;
        }

        const char *label = entity_kind_label(kind);
        char heading[64];
        strncpy(heading, label, sizeof(heading) - 1);
        heading[sizeof(heading) - 1] = '\0';
        heading[0] = (char)toupper((unsigned char)heading[0]);
        fprintf(out, "## %ss (%d)\n\n", heading, section_count);

        for (int i = 0; i < list->count; i++) {
            const Entity *e = &list->items[i];
            if (e->identity.kind != kind)
                continue;
            md_write_entity(out, e, store);
        }
    }
}

/* -----------------------------------------------------------------------
 * HTML report
 * --------------------------------------------------------------------- */

static void report_write_html(FILE *out, const EntityList *list,
                               const TripletStore *store)
{
    fprintf(out, "<!DOCTYPE html>\n"
                 "<html lang=\"en\">\n"
                 "<head>\n"
                 "<meta charset=\"UTF-8\">\n"
                 "<meta name=\"viewport\" content=\"width=device-width,"
                 " initial-scale=1\">\n"
                 "<title>Requirements Report</title>\n"
                 "<style>%s</style>\n"
                 "</head>\n"
                 "<body>\n"
                 "<h1>Requirements Report</h1>\n\n",
            HTML_CSS);

    for (int ki = 0; ki < KIND_ORDER_LEN; ki++) {
        EntityKind kind = KIND_ORDER[ki];

        if (!list_has_kind(list, kind))
            continue;

        int section_count = 0;
        for (int i = 0; i < list->count; i++) {
            if (list->items[i].identity.kind == kind)
                section_count++;
        }

        const char *label = entity_kind_label(kind);
        char heading[64];
        strncpy(heading, label, sizeof(heading) - 1);
        heading[sizeof(heading) - 1] = '\0';
        heading[0] = (char)toupper((unsigned char)heading[0]);

        fprintf(out, "<h2>%ss (%d)</h2>\n\n", heading, section_count);

        for (int i = 0; i < list->count; i++) {
            const Entity *e = &list->items[i];
            if (e->identity.kind != kind)
                continue;
            html_write_entity(out, e, store);
        }
    }

    fprintf(out, "</body>\n</html>\n");
}

/* -----------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */

void report_write(FILE *out, const EntityList *list,
                  const TripletStore *store, ReportFormat fmt)
{
    if (fmt == REPORT_FORMAT_HTML)
        report_write_html(out, list, store);
    else
        report_write_md(out, list, store);
}
