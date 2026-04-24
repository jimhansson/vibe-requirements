#include <stdio.h>
#include <string.h>

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

/* Write HTML-escaped text: & → &amp;  < → &lt;  > → &gt;  " → &quot; */
static void html_escape(FILE *out, const char *s)
{
    for (; *s; s++) {
        switch (*s) {
        case '&':  fputs("&amp;",  out); break;
        case '<':  fputs("&lt;",   out); break;
        case '>':  fputs("&gt;",   out); break;
        case '"':  fputs("&quot;", out); break;
        default:   fputc(*s, out); break;
        }
    }
}

/* Emit a newline-separated multi-value field as an HTML <ul> list. */
static void html_bullet_list(FILE *out, const char *label, const char *flat,
                              int count)
{
    if (count == 0 || flat[0] == '\0')
        return;

    fprintf(out, "<p><strong>");
    html_escape(out, label);
    fprintf(out, ":</strong></p>\n<ul>\n");

    const char *p = flat;
    while (*p) {
        const char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        if (len > 0) {
            fprintf(out, "  <li>");
            /* Use fwrite to handle the non-NUL-terminated slice, then
             * escape character-by-character would require a copy; instead
             * write slice via a temporary buffer on the stack.           */
            char buf[512];
            size_t copy = len < sizeof(buf) - 1 ? len : sizeof(buf) - 1;
            memcpy(buf, p, copy);
            buf[copy] = '\0';
            html_escape(out, buf);
            fprintf(out, "</li>\n");
        }
        if (!nl)
            break;
        p = nl + 1;
    }
    fprintf(out, "</ul>\n");
}

/* Emit a single HTML entity section (<section> + <h3> + body). */
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

    /* ---- Meta badges ---- */
    fprintf(out, "<p class=\"meta\">");
    fprintf(out, "<span class=\"kind\">");
    html_escape(out, entity_kind_label(e->identity.kind));
    fprintf(out, "</span>");
    if (e->lifecycle.status[0] != '\0') {
        fprintf(out, " | <span class=\"status\">");
        html_escape(out, e->lifecycle.status);
        fprintf(out, "</span>");
    }
    if (e->lifecycle.priority[0] != '\0') {
        fprintf(out, " | <span class=\"priority\">");
        html_escape(out, e->lifecycle.priority);
        fprintf(out, "</span>");
    }
    if (e->lifecycle.owner[0] != '\0') {
        fprintf(out, " | <span class=\"owner\">");
        html_escape(out, e->lifecycle.owner);
        fprintf(out, "</span>");
    }
    fprintf(out, "</p>\n");

    /* ---- Description / rationale ---- */
    if (e->text.description[0] != '\0') {
        fprintf(out, "<p class=\"description\">");
        html_escape(out, e->text.description);
        fprintf(out, "</p>\n");
    }
    if (e->text.rationale[0] != '\0') {
        fprintf(out, "<p class=\"rationale\"><strong>Rationale:</strong> ");
        html_escape(out, e->text.rationale);
        fprintf(out, "</p>\n");
    }

    /* ---- Document body ---- */
    if (e->doc_body.body[0] != '\0') {
        fprintf(out, "<pre class=\"body\">");
        html_escape(out, e->doc_body.body);
        fprintf(out, "</pre>\n");
    }

    /* ---- Tags ---- */
    html_bullet_list(out, "Tags", e->tags.tags, e->tags.count);

    /* ---- Sources ---- */
    html_bullet_list(out, "Sources", e->sources.sources, e->sources.count);

    /* ---- Acceptance criteria ---- */
    html_bullet_list(out, "Acceptance Criteria",
                     e->acceptance_criteria.criteria,
                     e->acceptance_criteria.count);

    /* ---- User story ---- */
    if (e->user_story.role[0] != '\0' || e->user_story.goal[0] != '\0') {
        fprintf(out, "<p><strong>User Story:</strong></p>\n<ul>\n");
        if (e->user_story.role[0] != '\0') {
            fprintf(out, "  <li>As a: ");
            html_escape(out, e->user_story.role);
            fprintf(out, "</li>\n");
        }
        if (e->user_story.goal[0] != '\0') {
            fprintf(out, "  <li>I want: ");
            html_escape(out, e->user_story.goal);
            fprintf(out, "</li>\n");
        }
        if (e->user_story.reason[0] != '\0') {
            fprintf(out, "  <li>So that: ");
            html_escape(out, e->user_story.reason);
            fprintf(out, "</li>\n");
        }
        fprintf(out, "</ul>\n");
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
            fprintf(out, "<p><strong>Traceability:</strong></p>\n<ul>\n");
            for (size_t i = 0; i < out_links.count; i++) {
                if (!out_links.triples[i].inferred) {
                    fprintf(out, "  <li><code>[");
                    html_escape(out, out_links.triples[i].predicate);
                    fprintf(out, "]</code> → <a href=\"#");
                    html_escape(out, out_links.triples[i].object);
                    fprintf(out, "\">");
                    html_escape(out, out_links.triples[i].object);
                    fprintf(out, "</a></li>\n");
                }
            }
            for (size_t i = 0; i < in_links.count; i++) {
                if (!in_links.triples[i].inferred) {
                    fprintf(out, "  <li><code>[");
                    html_escape(out, in_links.triples[i].predicate);
                    fprintf(out, "]</code> ← <a href=\"#");
                    html_escape(out, in_links.triples[i].subject);
                    fprintf(out, "\">");
                    html_escape(out, in_links.triples[i].subject);
                    fprintf(out, "</a></li>\n");
                }
            }
            fprintf(out, "</ul>\n");
        }

        triplet_store_list_free(out_links);
        triplet_store_list_free(in_links);
    }

    fprintf(out, "</section>\n\n");
}

/* -----------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */

void report_write(FILE *out, const EntityList *list,
                  const TripletStore *store, ReportFormat format)
{
    /* ------------------------------------------------------------------ */
    /* HTML: emit header                                                   */
    /* ------------------------------------------------------------------ */
    if (format == REPORT_FORMAT_HTML) {
        fprintf(out,
            "<!DOCTYPE html>\n"
            "<html lang=\"en\">\n"
            "<head>\n"
            "<meta charset=\"UTF-8\">\n"
            "<meta name=\"viewport\" content=\"width=device-width,"
            " initial-scale=1\">\n"
            "<title>Requirements Report</title>\n"
            "<style>\n"
            "  body { font-family: sans-serif; max-width: 900px;"
            " margin: 2em auto; padding: 0 1em; }\n"
            "  h1, h2 { border-bottom: 1px solid #ccc; padding-bottom:"
            " 0.2em; }\n"
            "  h3 { margin-top: 1.5em; }\n"
            "  section.entity { border-left: 3px solid #ccc;"
            " padding-left: 1em; margin-bottom: 1.5em; }\n"
            "  .meta { color: #555; font-size: 0.9em; }\n"
            "  .kind { background: #eee; border-radius: 3px;"
            " padding: 0.1em 0.4em; }\n"
            "  .status { background: #d4edda; border-radius: 3px;"
            " padding: 0.1em 0.4em; }\n"
            "  .priority { background: #fff3cd; border-radius: 3px;"
            " padding: 0.1em 0.4em; }\n"
            "  pre.body { background: #f8f8f8; padding: 0.8em;"
            " border-radius: 4px; overflow-x: auto; }\n"
            "  code { background: #f0f0f0; padding: 0.1em 0.3em;"
            " border-radius: 3px; }\n"
            "</style>\n"
            "</head>\n"
            "<body>\n"
            "<h1>Requirements Report</h1>\n\n");
    } else {
        /* Markdown header */
        fprintf(out, "# Requirements Report\n\n");
    }

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

        /* Section heading */
        if (format == REPORT_FORMAT_HTML) {
            fprintf(out, "<h2>");
            /* Capitalise first letter */
            if (label[0] != '\0') {
                char c = (char)(label[0] >= 'a' && label[0] <= 'z'
                                ? label[0] - 32 : label[0]);
                fputc(c, out);
                html_escape(out, label + 1);
            }
            fprintf(out, "s (%d)</h2>\n\n", section_count);
        } else {
            /* Markdown: capitalise first letter */
            char heading[64];
            strncpy(heading, label, sizeof(heading) - 1);
            heading[sizeof(heading) - 1] = '\0';
            if (heading[0] >= 'a' && heading[0] <= 'z')
                heading[0] = (char)(heading[0] - 32);
            fprintf(out, "## %ss (%d)\n\n", heading, section_count);
        }

        /* Entities in this kind section */
        for (int i = 0; i < list->count; i++) {
            const Entity *e = &list->items[i];
            if (e->identity.kind != kind)
                continue;

            if (format == REPORT_FORMAT_HTML)
                html_write_entity(out, e, store);
            else
                md_write_entity(out, e, store);
        }
    }

    /* ------------------------------------------------------------------ */
    /* HTML: close document                                                */
    /* ------------------------------------------------------------------ */
    if (format == REPORT_FORMAT_HTML) {
        fprintf(out, "</body>\n</html>\n");
    }
}
