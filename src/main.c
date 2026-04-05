#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "requirement.h"
#include "discovery.h"
#include "yaml_simple.h"
#include "triplet_store_c.h"

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
/* Table rendering — requirements                                      */
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
/* Table rendering — relations                                         */
/* ------------------------------------------------------------------ */

#define SUBJ_MAX_DISPLAY 32
#define OBJ_MAX_DISPLAY  48

static void print_rel_rule(int subj_w, int pred_w, int obj_w)
{
    int widths[] = { subj_w, pred_w, obj_w };
    int ncols    = (int)(sizeof(widths) / sizeof(widths[0]));
    for (int c = 0; c < ncols; c++) {
        putchar('+');
        for (int i = 0; i < widths[c] + 2; i++)
            putchar('-');
    }
    puts("+");
}

static void list_relations(const TripletStore *store)
{
    CTripleList all = triplet_store_find_all(store);

    if (all.count == 0) {
        puts("No relations found.");
        triplet_store_list_free(all);
        return;
    }

    /* Determine column widths from data. */
    int subj_w = (int)strlen("Subject");
    int pred_w = (int)strlen("Relation");
    int obj_w  = (int)strlen("Object");

    for (size_t i = 0; i < all.count; i++) {
        int len;
        len = (int)strlen(all.triples[i].subject);
        if (len > subj_w) subj_w = len;
        len = (int)strlen(all.triples[i].predicate);
        if (len > pred_w) pred_w = len;
        len = (int)strlen(all.triples[i].object);
        if (len > obj_w)  obj_w  = len;
    }

    /* Cap column widths. */
    if (subj_w > SUBJ_MAX_DISPLAY) subj_w = SUBJ_MAX_DISPLAY;
    if (obj_w  > OBJ_MAX_DISPLAY)  obj_w  = OBJ_MAX_DISPLAY;

    print_rel_rule(subj_w, pred_w, obj_w);
    printf("| %-*s | %-*s | %-*s |\n",
           subj_w, "Subject", pred_w, "Relation", obj_w, "Object");
    print_rel_rule(subj_w, pred_w, obj_w);

    for (size_t i = 0; i < all.count; i++) {
        const CTriple *t = &all.triples[i];

        /* Truncate subject if needed. */
        char sbuf[SUBJ_MAX_DISPLAY + 1];
        strncpy(sbuf, t->subject, sizeof(sbuf) - 1);
        sbuf[sizeof(sbuf) - 1] = '\0';
        if (subj_w >= 3 && (int)strlen(t->subject) > subj_w) {
            sbuf[subj_w - 3] = sbuf[subj_w - 2] = sbuf[subj_w - 1] = '.';
            sbuf[subj_w] = '\0';
        }

        /* Truncate object if needed. */
        char obuf[OBJ_MAX_DISPLAY + 1];
        strncpy(obuf, t->object, sizeof(obuf) - 1);
        obuf[sizeof(obuf) - 1] = '\0';
        if (obj_w >= 3 && (int)strlen(t->object) > obj_w) {
            obuf[obj_w - 3] = obuf[obj_w - 2] = obuf[obj_w - 1] = '.';
            obuf[obj_w] = '\0';
        }

        printf("| %-*s | %-*s | %-*s |\n",
               subj_w, sbuf, pred_w, t->predicate, obj_w, obuf);
    }

    print_rel_rule(subj_w, pred_w, obj_w);
    printf("\nTotal: %zu relation(s)\n", all.count);

    triplet_store_list_free(all);
}

/* ------------------------------------------------------------------ */
/* Build the triplet store from the parsed requirement list            */
/* ------------------------------------------------------------------ */

static TripletStore *build_relation_store(const RequirementList *list)
{
    TripletStore *store = triplet_store_create();
    if (!store)
        return NULL;

    for (int i = 0; i < list->count; i++) {
        const Requirement *r = &list->items[i];
        if (yaml_parse_links(r->file_path, r->id, store) < 0)
            fprintf(stderr, "warning: could not parse links from '%s'\n",
                    r->file_path);
    }

    return store;
}

/* ------------------------------------------------------------------ */
/* Entry point                                                         */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    /* Parse optional subcommand and directory arguments. */
    int         show_links = 0;
    const char *root       = ".";

    int arg_idx = 1;

    if (argc >= 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("Usage: %s [links] [directory]\n\n", argv[0]);
            printf("Commands:\n");
            printf("  (default)  List all requirements found in the directory tree.\n");
            printf("  links      List all relations parsed from requirement files.\n\n");
            printf("  directory  Root directory to scan (default: current directory).\n\n");
            printf("  YAML files without a top-level 'id' field are silently ignored.\n");
            return 0;
        }
        if (strcmp(argv[1], "links") == 0) {
            show_links = 1;
            arg_idx    = 2;
        }
    }

    if (argc > arg_idx)
        root = argv[arg_idx];

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

    if (show_links) {
        TripletStore *store = build_relation_store(&list);
        if (!store) {
            fprintf(stderr, "error: failed to create relation store\n");
            req_list_free(&list);
            return 1;
        }
        list_relations(store);
        triplet_store_destroy(store);
    } else {
        list_requirements(&list);
    }

    req_list_free(&list);
    return 0;
}
