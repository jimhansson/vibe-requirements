#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "requirement.h"
#include "discovery.h"

/* ------------------------------------------------------------------ */
/* Comparison helper for qsort — sort requirements by ID.             */
/* ------------------------------------------------------------------ */

static int cmp_by_id(const void *a, const void *b)
{
    const Requirement *ra = (const Requirement *)a;
    const Requirement *rb = (const Requirement *)b;
    return strcmp(ra->id, rb->id);
}

/* ------------------------------------------------------------------ */
/* Table rendering                                                     */
/* ------------------------------------------------------------------ */

#define TITLE_MAX_DISPLAY 52  /* truncate long titles to this width */

static void print_rule(int id_w, int title_w, int type_w,
                        int status_w, int priority_w)
{
    int widths[] = { id_w, title_w, type_w, status_w, priority_w };
    int ncols    = (int)(sizeof(widths) / sizeof(widths[0]));
    for (int c = 0; c < ncols; c++) {
        putchar('+');
        for (int i = 0; i < widths[c] + 2; i++)
            putchar('-');
    }
    puts("+");
}

static void print_row(int id_w, int title_w, int type_w,
                       int status_w, int priority_w,
                       const char *id, const char *title,
                       const char *type, const char *status,
                       const char *priority)
{
    /* Truncate title if it exceeds display width. */
    char tbuf[TITLE_MAX_DISPLAY + 1];
    strncpy(tbuf, title, sizeof(tbuf) - 1);
    tbuf[sizeof(tbuf) - 1] = '\0';
    if ((int)strlen(title) > title_w) {
        tbuf[title_w - 3] = '.';
        tbuf[title_w - 2] = '.';
        tbuf[title_w - 1] = '.';
        tbuf[title_w]     = '\0';
    }

    printf("| %-*s | %-*s | %-*s | %-*s | %-*s |\n",
           id_w,       id,
           title_w,    tbuf,
           type_w,     type,
           status_w,   status,
           priority_w, priority);
}

static void list_requirements(const RequirementList *list)
{
    if (list->count == 0) {
        puts("No requirements found.");
        return;
    }

    /* Determine column widths from data. */
    int id_w       = (int)strlen("ID");
    int title_w    = (int)strlen("Title");
    int type_w     = (int)strlen("Type");
    int status_w   = (int)strlen("Status");
    int priority_w = (int)strlen("Priority");

    for (int i = 0; i < list->count; i++) {
        const Requirement *r = &list->items[i];
        int len;
        len = (int)strlen(r->id);       if (len > id_w)       id_w       = len;
        len = (int)strlen(r->title);    if (len > title_w)    title_w    = len;
        len = (int)strlen(r->type);     if (len > type_w)     type_w     = len;
        len = (int)strlen(r->status);   if (len > status_w)   status_w   = len;
        len = (int)strlen(r->priority); if (len > priority_w) priority_w = len;
    }

    /* Cap title column. */
    if (title_w > TITLE_MAX_DISPLAY)
        title_w = TITLE_MAX_DISPLAY;

    print_rule(id_w, title_w, type_w, status_w, priority_w);
    print_row(id_w, title_w, type_w, status_w, priority_w,
              "ID", "Title", "Type", "Status", "Priority");
    print_rule(id_w, title_w, type_w, status_w, priority_w);

    for (int i = 0; i < list->count; i++) {
        const Requirement *r = &list->items[i];
        print_row(id_w, title_w, type_w, status_w, priority_w,
                  r->id, r->title, r->type, r->status, r->priority);
    }

    print_rule(id_w, title_w, type_w, status_w, priority_w);
    printf("\nTotal: %d requirement(s)\n", list->count);
}

/* ------------------------------------------------------------------ */
/* Entry point                                                         */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    const char *root = ".";

    if (argc >= 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("Usage: %s [directory]\n\n", argv[0]);
            printf("  List all requirements found in the given directory tree.\n");
            printf("  Defaults to the current directory.\n\n");
            printf("  YAML files without a top-level 'id' field are silently ignored.\n");
            return 0;
        }
        root = argv[1];
    }

    RequirementList list;
    req_list_init(&list);

    int found = discover_requirements(root, &list);
    if (found < 0) {
        fprintf(stderr, "error: cannot open directory '%s'\n", root);
        req_list_free(&list);
        return 1;
    }

    /* Sort alphabetically by ID before displaying. */
    if (list.count > 1)
        qsort(list.items, (size_t)list.count, sizeof(Requirement), cmp_by_id);

    list_requirements(&list);

    req_list_free(&list);
    return 0;
}
