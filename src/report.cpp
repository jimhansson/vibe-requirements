/**
 * @file report.cpp
 * @brief Report generation (C++ edition).
 */

#include <cstdio>
#include <cstring>
#include <cctype>
#include <vector>
#include <string>

#include "report.h"
#include "entity.h"

static const EntityKind KIND_ORDER[] = {
    ENTITY_KIND_DOCUMENT,
    ENTITY_KIND_DOCUMENT_SCHEMA,
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

static int list_has_kind(const EntityList *list, EntityKind kind)
{
    for (const auto &e : *list) {
        if (e.identity.kind == kind)
            return 1;
    }
    return 0;
}

static void md_bullet_list(FILE *out, const char *label,
                            const std::vector<std::string> &items)
{
    if (items.empty())
        return;

    fprintf(out, "**%s:**\n\n", label);
    for (const auto &item : items) {
        if (!item.empty())
            fprintf(out, "- %s\n", item.c_str());
    }
    fprintf(out, "\n");
}

static void md_write_entity(FILE *out, const Entity *e,
                             const vibe::TripletStore *store)
{
    if (!e->identity.title.empty())
        fprintf(out, "### %s — %s\n\n", e->identity.id.c_str(),
                e->identity.title.c_str());
    else
        fprintf(out, "### %s\n\n", e->identity.id.c_str());

    fprintf(out, "**Kind:** %s", entity_kind_label(e->identity.kind));
    if (!e->lifecycle.status.empty())
        fprintf(out, " | **Status:** %s", e->lifecycle.status.c_str());
    if (!e->lifecycle.priority.empty())
        fprintf(out, " | **Priority:** %s", e->lifecycle.priority.c_str());
    if (!e->lifecycle.owner.empty())
        fprintf(out, " | **Owner:** %s", e->lifecycle.owner.c_str());
    fprintf(out, "\n\n");

    if (!e->text.description.empty())
        fprintf(out, "%s\n\n", e->text.description.c_str());
    if (!e->text.rationale.empty())
        fprintf(out, "**Rationale:** %s\n\n", e->text.rationale.c_str());

    if (!e->doc_body.body.empty())
        fprintf(out, "%s\n\n", e->doc_body.body.c_str());

    md_bullet_list(out, "Tags",    e->tags.tags);
    md_bullet_list(out, "Sources", e->sources.sources);
    md_bullet_list(out, "Acceptance Criteria",
                   e->acceptance_criteria.criteria);

    if (!e->user_story.role.empty() || !e->user_story.goal.empty()) {
        fprintf(out, "**User Story:**\n\n");
        if (!e->user_story.role.empty())
            fprintf(out, "- As a: %s\n", e->user_story.role.c_str());
        if (!e->user_story.goal.empty())
            fprintf(out, "- I want: %s\n", e->user_story.goal.c_str());
        if (!e->user_story.reason.empty())
            fprintf(out, "- So that: %s\n", e->user_story.reason.c_str());
        fprintf(out, "\n");
    }

    if (store) {
        auto out_links = store->find_by_subject(e->identity.id);
        auto in_links  = store->find_by_object(e->identity.id);
        bool has_out = false, has_in = false;
        for (const auto *t : out_links) if (!t->inferred) { has_out = true; break; }
        for (const auto *t : in_links)  if (!t->inferred) { has_in  = true; break; }

        if (has_out || has_in) {
            fprintf(out, "**Traceability:**\n\n");
            for (const auto *t : out_links) {
                if (!t->inferred)
                    fprintf(out, "- `[%s]` → %s\n",
                            t->predicate.c_str(), t->object.c_str());
            }
            for (const auto *t : in_links) {
                if (!t->inferred)
                    fprintf(out, "- `[%s]` ← %s\n",
                            t->predicate.c_str(), t->subject.c_str());
            }
            fprintf(out, "\n");
        }
    }

    fprintf(out, "---\n\n");
}

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

static void html_bullet_list(FILE *out, const char *label,
                              const std::vector<std::string> &items)
{
    if (items.empty())
        return;

    fprintf(out, "<p><strong>%s:</strong></p>\n<ul>\n", label);
    for (const auto &item : items) {
        if (!item.empty()) {
            fprintf(out, "<li>");
            html_escape(out, item.c_str());
            fputs("</li>\n", out);
        }
    }
    fputs("</ul>\n", out);
}

static void html_write_entity(FILE *out, const Entity *e,
                               const vibe::TripletStore *store)
{
    fprintf(out, "<section class=\"entity\" id=\"");
    html_escape(out, e->identity.id.c_str());
    fprintf(out, "\">\n");

    fprintf(out, "<h3>");
    html_escape(out, e->identity.id.c_str());
    if (!e->identity.title.empty()) {
        fprintf(out, " — ");
        html_escape(out, e->identity.title.c_str());
    }
    fprintf(out, "</h3>\n");

    fprintf(out, "<p><strong>Kind:</strong> %s",
            entity_kind_label(e->identity.kind));
    if (!e->lifecycle.status.empty()) {
        fprintf(out, " | <strong>Status:</strong> ");
        html_escape(out, e->lifecycle.status.c_str());
    }
    if (!e->lifecycle.priority.empty()) {
        fprintf(out, " | <strong>Priority:</strong> ");
        html_escape(out, e->lifecycle.priority.c_str());
    }
    if (!e->lifecycle.owner.empty()) {
        fprintf(out, " | <strong>Owner:</strong> ");
        html_escape(out, e->lifecycle.owner.c_str());
    }
    fprintf(out, "</p>\n");

    if (!e->text.description.empty()) {
        fprintf(out, "<p>");
        html_escape(out, e->text.description.c_str());
        fprintf(out, "</p>\n");
    }
    if (!e->text.rationale.empty()) {
        fprintf(out, "<p><strong>Rationale:</strong> ");
        html_escape(out, e->text.rationale.c_str());
        fprintf(out, "</p>\n");
    }

    if (!e->doc_body.body.empty()) {
        fprintf(out, "<pre>");
        html_escape(out, e->doc_body.body.c_str());
        fprintf(out, "</pre>\n");
    }

    html_bullet_list(out, "Tags",    e->tags.tags);
    html_bullet_list(out, "Sources", e->sources.sources);
    html_bullet_list(out, "Acceptance Criteria",
                     e->acceptance_criteria.criteria);

    if (!e->user_story.role.empty() || !e->user_story.goal.empty()) {
        fprintf(out, "<p><strong>User Story:</strong></p>\n<ul>\n");
        if (!e->user_story.role.empty()) {
            fprintf(out, "<li>As a: ");
            html_escape(out, e->user_story.role.c_str());
            fprintf(out, "</li>\n");
        }
        if (!e->user_story.goal.empty()) {
            fprintf(out, "<li>I want: ");
            html_escape(out, e->user_story.goal.c_str());
            fprintf(out, "</li>\n");
        }
        if (!e->user_story.reason.empty()) {
            fprintf(out, "<li>So that: ");
            html_escape(out, e->user_story.reason.c_str());
            fprintf(out, "</li>\n");
        }
        fprintf(out, "</ul>\n");
    }

    if (store) {
        auto out_links = store->find_by_subject(e->identity.id);
        auto in_links  = store->find_by_object(e->identity.id);
        bool has_out = false, has_in = false;
        for (const auto *t : out_links) if (!t->inferred) { has_out = true; break; }
        for (const auto *t : in_links)  if (!t->inferred) { has_in  = true; break; }

        if (has_out || has_in) {
            fprintf(out, "<p><strong>Traceability:</strong></p>\n<ul>\n");
            for (const auto *t : out_links) {
                if (!t->inferred) {
                    fprintf(out, "<li>[%s] → <a href=\"#%s\">%s</a></li>\n",
                            t->predicate.c_str(),
                            t->object.c_str(),
                            t->object.c_str());
                }
            }
            for (const auto *t : in_links) {
                if (!t->inferred) {
                    fprintf(out, "<li>[%s] ← <a href=\"#%s\">%s</a></li>\n",
                            t->predicate.c_str(),
                            t->subject.c_str(),
                            t->subject.c_str());
                }
            }
            fprintf(out, "</ul>\n");
        }
    }

    fprintf(out, "</section>\n\n");
}

static const char HTML_CSS[] =
    "body{font-family:sans-serif;max-width:960px;margin:0 auto;padding:1rem}"
    "section.entity{border:1px solid #ddd;border-radius:4px;"
    "padding:1rem;margin-bottom:1rem}"
    "h1,h2,h3{color:#333}"
    "h2{border-bottom:2px solid #ddd;padding-bottom:.25rem}"
    "a{color:#0366d6}";

static void report_write_md(FILE *out, const EntityList *list,
                             const vibe::TripletStore *store)
{
    fprintf(out, "# Requirements Report\n\n");

    for (int ki = 0; ki < KIND_ORDER_LEN; ki++) {
        EntityKind kind = KIND_ORDER[ki];

        if (!list_has_kind(list, kind))
            continue;

        int section_count = 0;
        for (const auto &e : *list) {
            if (e.identity.kind == kind)
                section_count++;
        }

        const char *label = entity_kind_label(kind);
        char heading[64];
        strncpy(heading, label, sizeof(heading) - 1);
        heading[sizeof(heading) - 1] = '\0';
        heading[0] = (char)toupper((unsigned char)heading[0]);
        fprintf(out, "## %ss (%d)\n\n", heading, section_count);

        for (const auto &e : *list) {
            if (e.identity.kind != kind)
                continue;
            md_write_entity(out, &e, store);
        }
    }
}

static void report_write_html(FILE *out, const EntityList *list,
                               const vibe::TripletStore *store)
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
        for (const auto &e : *list) {
            if (e.identity.kind == kind)
                section_count++;
        }

        const char *label = entity_kind_label(kind);
        char heading[64];
        strncpy(heading, label, sizeof(heading) - 1);
        heading[sizeof(heading) - 1] = '\0';
        heading[0] = (char)toupper((unsigned char)heading[0]);

        fprintf(out, "<h2>%ss (%d)</h2>\n\n", heading, section_count);

        for (const auto &e : *list) {
            if (e.identity.kind != kind)
                continue;
            html_write_entity(out, &e, store);
        }
    }

    fprintf(out, "</body>\n</html>\n");
}

/* -------------------------------------------------------------------------
 * JSON output
 * ---------------------------------------------------------------------- */

static void json_escape_string(FILE *out, const char *str)
{
    fputc('"', out);
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '"':  fputs("\\\"", out); break;
            case '\\': fputs("\\\\", out); break;
            case '\n': fputs("\\n",  out); break;
            case '\r': fputs("\\r",  out); break;
            case '\t': fputs("\\t",  out); break;
            default:
                if ((unsigned char)*p < 0x20)
                    fprintf(out, "\\u%04x", (unsigned char)*p);
                else
                    fputc(*p, out);
                break;
        }
    }
    fputc('"', out);
}

static void json_write_string_field(FILE *out, const char *key, const char *val)
{
    fprintf(out, "    ");
    json_escape_string(out, key);
    fprintf(out, ": ");
    json_escape_string(out, val);
}

static void json_write_string_array(FILE *out, const char *key,
                                    const std::vector<std::string> &items)
{
    if (items.empty())
        return;
    fprintf(out, "    ");
    json_escape_string(out, key);
    fprintf(out, ": [");
    for (size_t i = 0; i < items.size(); i++) {
        if (i > 0)
            fputs(", ", out);
        json_escape_string(out, items[i].c_str());
    }
    fputc(']', out);
}

static void json_write_entity(FILE *out, const Entity *e,
                               const vibe::TripletStore *store, bool last)
{
    fputs("  {\n", out);

    /* Mandatory identity fields */
    json_write_string_field(out, "id", e->identity.id.c_str());
    fprintf(out, ",\n");
    json_write_string_field(out, "kind", entity_kind_label(e->identity.kind));

    /* Optional identity fields */
    if (!e->identity.title.empty()) {
        fprintf(out, ",\n");
        json_write_string_field(out, "title", e->identity.title.c_str());
    }
    if (!e->identity.type_raw.empty()) {
        fprintf(out, ",\n");
        json_write_string_field(out, "type", e->identity.type_raw.c_str());
    }
    if (!e->identity.file_path.empty()) {
        fprintf(out, ",\n");
        json_write_string_field(out, "file", e->identity.file_path.c_str());
    }

    /* Lifecycle */
    if (!e->lifecycle.status.empty()) {
        fprintf(out, ",\n");
        json_write_string_field(out, "status", e->lifecycle.status.c_str());
    }
    if (!e->lifecycle.priority.empty()) {
        fprintf(out, ",\n");
        json_write_string_field(out, "priority", e->lifecycle.priority.c_str());
    }
    if (!e->lifecycle.owner.empty()) {
        fprintf(out, ",\n");
        json_write_string_field(out, "owner", e->lifecycle.owner.c_str());
    }
    if (!e->lifecycle.version.empty()) {
        fprintf(out, ",\n");
        json_write_string_field(out, "version", e->lifecycle.version.c_str());
    }

    /* Text */
    if (!e->text.description.empty()) {
        fprintf(out, ",\n");
        json_write_string_field(out, "description", e->text.description.c_str());
    }
    if (!e->text.rationale.empty()) {
        fprintf(out, ",\n");
        json_write_string_field(out, "rationale", e->text.rationale.c_str());
    }
    if (!e->doc_body.body.empty()) {
        fprintf(out, ",\n");
        json_write_string_field(out, "body", e->doc_body.body.c_str());
    }

    /* Arrays */
    if (!e->tags.tags.empty()) {
        fprintf(out, ",\n");
        json_write_string_array(out, "tags", e->tags.tags);
    }
    if (!e->sources.sources.empty()) {
        fprintf(out, ",\n");
        json_write_string_array(out, "sources", e->sources.sources);
    }
    if (!e->acceptance_criteria.criteria.empty()) {
        fprintf(out, ",\n");
        json_write_string_array(out, "acceptance_criteria",
                                e->acceptance_criteria.criteria);
    }
    if (!e->doc_membership.doc_ids.empty()) {
        fprintf(out, ",\n");
        json_write_string_array(out, "documents", e->doc_membership.doc_ids);
    }
    if (!e->applies_to.applies_to.empty()) {
        fprintf(out, ",\n");
        json_write_string_array(out, "applies_to", e->applies_to.applies_to);
    }

    /* User story */
    if (!e->user_story.role.empty() || !e->user_story.goal.empty() ||
        !e->user_story.reason.empty()) {
        fprintf(out, ",\n    \"user_story\": {");
        bool first = true;
        if (!e->user_story.role.empty()) {
            json_escape_string(out, "role");
            fputs(": ", out);
            json_escape_string(out, e->user_story.role.c_str());
            first = false;
        }
        if (!e->user_story.goal.empty()) {
            if (!first) fputs(", ", out);
            json_escape_string(out, "goal");
            fputs(": ", out);
            json_escape_string(out, e->user_story.goal.c_str());
            first = false;
        }
        if (!e->user_story.reason.empty()) {
            if (!first) fputs(", ", out);
            json_escape_string(out, "reason");
            fputs(": ", out);
            json_escape_string(out, e->user_story.reason.c_str());
        }
        fputc('}', out);
    }

    /* Assumption */
    if (!e->assumption.text.empty()) {
        fprintf(out, ",\n    \"assumption\": {");
        json_escape_string(out, "text");
        fputs(": ", out);
        json_escape_string(out, e->assumption.text.c_str());
        if (!e->assumption.status.empty()) {
            fputs(", ", out);
            json_escape_string(out, "status");
            fputs(": ", out);
            json_escape_string(out, e->assumption.status.c_str());
        }
        if (!e->assumption.source.empty()) {
            fputs(", ", out);
            json_escape_string(out, "source");
            fputs(": ", out);
            json_escape_string(out, e->assumption.source.c_str());
        }
        fputc('}', out);
    }

    /* Constraint */
    if (!e->constraint.text.empty()) {
        fprintf(out, ",\n    \"constraint\": {");
        json_escape_string(out, "text");
        fputs(": ", out);
        json_escape_string(out, e->constraint.text.c_str());
        if (!e->constraint.kind.empty()) {
            fputs(", ", out);
            json_escape_string(out, "kind");
            fputs(": ", out);
            json_escape_string(out, e->constraint.kind.c_str());
        }
        if (!e->constraint.source.empty()) {
            fputs(", ", out);
            json_escape_string(out, "source");
            fputs(": ", out);
            json_escape_string(out, e->constraint.source.c_str());
        }
        fputc('}', out);
    }

    /* Traceability links */
    if (store) {
        auto out_links = store->find_by_subject(e->identity.id);
        auto in_links  = store->find_by_object(e->identity.id);
        bool has_any = false;
        for (const auto *t : out_links) if (!t->inferred) { has_any = true; break; }
        if (!has_any)
            for (const auto *t : in_links) if (!t->inferred) { has_any = true; break; }

        if (has_any) {
            fprintf(out, ",\n    \"traceability\": [");
            bool first_link = true;
            for (const auto *t : out_links) {
                if (t->inferred) continue;
                if (!first_link) fputs(", ", out);
                fputs("{", out);
                json_escape_string(out, "relation");
                fputs(": ", out);
                json_escape_string(out, t->predicate.c_str());
                fputs(", ", out);
                json_escape_string(out, "target");
                fputs(": ", out);
                json_escape_string(out, t->object.c_str());
                fputs(", ", out);
                json_escape_string(out, "direction");
                fputs(": \"outgoing\"}", out);
                first_link = false;
            }
            for (const auto *t : in_links) {
                if (t->inferred) continue;
                if (!first_link) fputs(", ", out);
                fputs("{", out);
                json_escape_string(out, "relation");
                fputs(": ", out);
                json_escape_string(out, t->predicate.c_str());
                fputs(", ", out);
                json_escape_string(out, "target");
                fputs(": ", out);
                json_escape_string(out, t->subject.c_str());
                fputs(", ", out);
                json_escape_string(out, "direction");
                fputs(": \"incoming\"}", out);
                first_link = false;
            }
            fputc(']', out);
        }
    }

    fprintf(out, "\n  }");
    if (!last)
        fputc(',', out);
    fputc('\n', out);
}

static void report_write_json(FILE *out, const EntityList *list,
                               const vibe::TripletStore *store)
{
    fputs("[\n", out);
    size_t n = list->size();
    for (size_t i = 0; i < n; i++)
        json_write_entity(out, &(*list)[i], store, i == n - 1);
    fputs("]\n", out);
}

void report_write(FILE *out, const EntityList *list,
                  const vibe::TripletStore *store, ReportFormat fmt)
{
    if (fmt == REPORT_FORMAT_HTML)
        report_write_html(out, list, store);
    else if (fmt == REPORT_FORMAT_JSON)
        report_write_json(out, list, store);
    else
        report_write_md(out, list, store);
}
