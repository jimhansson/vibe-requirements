#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "requirement.h"
#include "entity.h"
#include "discovery.h"
#include "config.h"
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

static void print_rel_rule(int subj_w, int pred_w, int obj_w, int src_w)
{
    int widths[] = { subj_w, pred_w, obj_w, src_w };
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
    int src_w  = (int)strlen("Source");

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

    print_rel_rule(subj_w, pred_w, obj_w, src_w);
    printf("| %-*s | %-*s | %-*s | %-*s |\n",
           subj_w, "Subject", pred_w, "Relation", obj_w, "Object",
           src_w, "Source");
    print_rel_rule(subj_w, pred_w, obj_w, src_w);

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

        const char *src = t->inferred ? "inferred" : "declared";
        printf("| %-*s | %-*s | %-*s | %-*s |\n",
               subj_w, sbuf, pred_w, t->predicate, obj_w, obuf,
               src_w, src);
    }

    print_rel_rule(subj_w, pred_w, obj_w, src_w);
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

    /* Generate inferred inverse triples for all known relation pairs. */
    triplet_store_infer_inverses(store);

    return store;
}

/* ------------------------------------------------------------------ */
/* --strict-links validation                                          */
/* ------------------------------------------------------------------ */

/*
 * For every user-declared triple (A, rel, B) where rel has a known
 * inverse inv(rel), check that (B, inv(rel), A) is ALSO user-declared.
 * If only the inferred direction exists, emit a warning to stderr.
 *
 * The inferred triple (B, inv_pred, A) — already present in the store
 * after triplet_store_infer_inverses() was called — is used as a proxy
 * to determine inv_pred without duplicating the relation-pair table in C.
 *
 * Note: this function is O(n * m) where n is the number of declared
 * triples and m is the average out-degree per subject.  For typical
 * repositories this is acceptable; a hash-based approach can be added
 * later if profiling shows it to be a bottleneck.
 *
 * Returns the number of one-sided link warnings found.
 */
static int check_strict_links(const TripletStore *store)
{
    CTripleList all = triplet_store_find_all(store);
    int warnings = 0;

    for (size_t i = 0; i < all.count; i++) {
        const CTriple *t = &all.triples[i];
        if (t->inferred) continue; /* only inspect user-declared triples */

        /*
         * Look for the inferred inverse (t->object, inv_pred, t->subject).
         * Its existence confirms that t->predicate is in the known-pair
         * table and tells us the expected inverse predicate name.
         */
        CTripleList by_subj = triplet_store_find_by_subject(store, t->object);

        /* Copy the inverse predicate into a local buffer before freeing
         * by_subj, to avoid a use-after-free when writing the warning. */
        char inv_pred_buf[256] = {0};
        int  found_declared    = 0;

        for (size_t j = 0; j < by_subj.count; j++) {
            const CTriple *cand = &by_subj.triples[j];
            if (cand->inferred && strcmp(cand->object, t->subject) == 0) {
                strncpy(inv_pred_buf, cand->predicate,
                        sizeof(inv_pred_buf) - 1);
                break;
            }
        }

        if (inv_pred_buf[0] == '\0') {
            /* No inferred inverse — relation is unknown; skip (Option C). */
            triplet_store_list_free(by_subj);
            continue;
        }

        /* Check whether a user-declared (t->object, inv_pred, t->subject)
         * triple already exists. */
        for (size_t j = 0; j < by_subj.count; j++) {
            const CTriple *cand = &by_subj.triples[j];
            if (!cand->inferred &&
                strcmp(cand->predicate, inv_pred_buf) == 0 &&
                strcmp(cand->object, t->subject) == 0) {
                found_declared = 1;
                break;
            }
        }
        triplet_store_list_free(by_subj);

        if (!found_declared) {
            fprintf(stderr,
                "warning: strict-links: '%s -[%s]-> %s' — "
                "inverse '[%s]' not explicitly declared by '%s'\n",
                t->subject, t->predicate, t->object,
                inv_pred_buf, t->object);
            warnings++;
        }
    }

    triplet_store_list_free(all);
    return warnings;
}

/* ------------------------------------------------------------------ */
/* Table rendering — entities (ECS)                                   */
/* ------------------------------------------------------------------ */

#define ENTITY_TITLE_MAX  48
#define ENTITY_KIND_MAX   12
#define ENTITY_STATUS_MAX 16
#define ENTITY_PRIO_MAX   12

static int cmp_entity_by_id(const void *a, const void *b)
{
    const Entity *ea = (const Entity *)a;
    const Entity *eb = (const Entity *)b;
    return strcmp(ea->identity.id, eb->identity.id);
}

static void print_entity_rule(int id_w, int title_w, int kind_w,
                               int status_w, int prio_w)
{
    int widths[] = { id_w, title_w, kind_w, status_w, prio_w };
    int ncols    = (int)(sizeof(widths) / sizeof(widths[0]));
    for (int c = 0; c < ncols; c++) {
        putchar('+');
        for (int i = 0; i < widths[c] + 2; i++)
            putchar('-');
    }
    puts("+");
}

static void print_entity_row(int id_w, int title_w, int kind_w,
                              int status_w, int prio_w,
                              const char *id, const char *title,
                              const char *kind, const char *status,
                              const char *prio)
{
    char tbuf[ENTITY_TITLE_MAX + 1];
    strncpy(tbuf, title, sizeof(tbuf) - 1);
    tbuf[sizeof(tbuf) - 1] = '\0';
    if ((int)strlen(title) > title_w) {
        tbuf[title_w - 3] = '.';
        tbuf[title_w - 2] = '.';
        tbuf[title_w - 1] = '.';
        tbuf[title_w]     = '\0';
    }

    printf("| %-*s | %-*s | %-*s | %-*s | %-*s |\n",
           id_w,     id,
           title_w,  tbuf,
           kind_w,   kind,
           status_w, status,
           prio_w,   prio);
}

static void list_entities(const EntityList *list)
{
    if (list->count == 0) {
        puts("No entities found.");
        return;
    }

    int id_w     = (int)strlen("ID");
    int title_w  = (int)strlen("Title");
    int kind_w   = (int)strlen("Kind");
    int status_w = (int)strlen("Status");
    int prio_w   = (int)strlen("Priority");

    for (int i = 0; i < list->count; i++) {
        const Entity *e = &list->items[i];
        int len;
        len = (int)strlen(e->identity.id);         if (len > id_w)     id_w     = len;
        len = (int)strlen(e->identity.title);       if (len > title_w)  title_w  = len;
        len = (int)strlen(entity_kind_label(e->identity.kind));
                                                    if (len > kind_w)   kind_w   = len;
        len = (int)strlen(e->lifecycle.status);     if (len > status_w) status_w = len;
        len = (int)strlen(e->lifecycle.priority);   if (len > prio_w)   prio_w   = len;
    }

    if (title_w > ENTITY_TITLE_MAX) title_w = ENTITY_TITLE_MAX;

    print_entity_rule(id_w, title_w, kind_w, status_w, prio_w);
    print_entity_row(id_w, title_w, kind_w, status_w, prio_w,
                     "ID", "Title", "Kind", "Status", "Priority");
    print_entity_rule(id_w, title_w, kind_w, status_w, prio_w);

    for (int i = 0; i < list->count; i++) {
        const Entity *e = &list->items[i];
        print_entity_row(id_w, title_w, kind_w, status_w, prio_w,
                         e->identity.id,
                         e->identity.title,
                         entity_kind_label(e->identity.kind),
                         e->lifecycle.status,
                         e->lifecycle.priority);
    }

    print_entity_rule(id_w, title_w, kind_w, status_w, prio_w);
    printf("\nTotal: %d entity/entities\n", list->count);
}

/* ------------------------------------------------------------------ */
/* Discovery for entities                                              */
/* ------------------------------------------------------------------ */

static void walk_entities(const char *dir, EntityList *list,
                           const VibeConfig *cfg)
{
    DIR *d = opendir(dir);
    if (!d)
        return;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        const char *name = entry->d_name;

        if (name[0] == '.')
            continue;

        char path[512];
        int n = snprintf(path, sizeof(path), "%s/%s", dir, name);
        if (n < 0 || (size_t)n >= sizeof(path)) {
            fprintf(stderr, "warning: path too long, skipping: %s/%s\n",
                    dir, name);
            continue;
        }

        struct stat st;
        if (stat(path, &st) != 0)
            continue;

        if (S_ISDIR(st.st_mode)) {
            if (config_is_ignored_dir(cfg, name))
                continue;
            walk_entities(path, list, cfg);
        } else if (S_ISREG(st.st_mode)) {
            const char *dot = strrchr(name, '.');
            if (!dot)
                continue;
            if (strcmp(dot, ".yaml") != 0 && strcmp(dot, ".yml") != 0)
                continue;
            if (yaml_parse_entities(path, list) < 0)
                fprintf(stderr, "warning: could not parse: %s\n", path);
        }
    }

    closedir(d);
}

static int discover_entities(const char *root_dir, EntityList *list,
                              const VibeConfig *cfg)
{
    DIR *probe = opendir(root_dir);
    if (!probe)
        return -1;
    closedir(probe);

    walk_entities(root_dir, list, cfg);
    return list->count;
}

int main(int argc, char *argv[])
{
    /* Parse optional subcommand and directory arguments. */
    int         show_links    = 0;
    int         strict_links  = 0;
    int         show_entities = 0;
    const char *root          = ".";

    int arg_idx = 1;

    if (argc >= 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("Usage: %s [links|entities] [--strict-links] [directory]\n\n",
                   argv[0]);
            printf("Commands:\n");
            printf("  (default)       List all requirements found in the directory tree.\n");
            printf("  links           List all relations parsed from requirement files.\n");
            printf("  entities        List all entities (all kinds) found in the directory tree.\n\n");
            printf("Options:\n");
            printf("  --strict-links  Warn when a known relation is declared in only one\n");
            printf("                  direction (inverse not explicitly present in YAML).\n");
            printf("                  Exits with a non-zero code if any warnings are found.\n");
            printf("  directory       Root directory to scan (default: current directory).\n\n");
            printf("  YAML files without a top-level 'id' field are silently ignored.\n");
            return 0;
        }
        if (strcmp(argv[1], "links") == 0) {
            show_links = 1;
            arg_idx    = 2;
        } else if (strcmp(argv[1], "entities") == 0) {
            show_entities = 1;
            arg_idx       = 2;
        }
    }

    /* Scan remaining arguments for flags and directory. */
    for (int i = arg_idx; i < argc; i++) {
        if (strcmp(argv[i], "--strict-links") == 0) {
            strict_links = 1;
        } else {
            root = argv[i];
        }
    }

    /* ------------------------------------------------------------------
     * 'entities' subcommand — use ECS parser path.
     * ------------------------------------------------------------------ */
    if (show_entities) {
        EntityList elist;
        entity_list_init(&elist);

        VibeConfig cfg;
        config_load(root, &cfg);

        int found = discover_entities(root, &elist, &cfg);
        if (found < 0) {
            fprintf(stderr, "error: cannot open directory '%s'\n", root);
            entity_list_free(&elist);
            return 1;
        }

        if (elist.count > 1)
            qsort(elist.items, (size_t)elist.count, sizeof(Entity),
                  cmp_entity_by_id);

        list_entities(&elist);
        entity_list_free(&elist);
        return 0;
    }

    /* ------------------------------------------------------------------
     * Default / links / strict-links — legacy RequirementList path.
     * ------------------------------------------------------------------ */
    RequirementList list;
    req_list_init(&list);

    VibeConfig cfg;
    /* config_load() returns -1 when .vibe-req.yaml is absent or unparseable,
     * which is perfectly valid — cfg is zeroed so discovery runs unfiltered. */
    config_load(root, &cfg);

    int found = discover_requirements(root, &list, &cfg);
    if (found < 0) {
        fprintf(stderr, "error: cannot open directory '%s'\n", root);
        req_list_free(&list);
        return 1;
    }

    /* Sort alphabetically by ID before displaying. */
    if (list.count > 1)
        qsort(list.items, (size_t)list.count, sizeof(Requirement), cmp_by_id);

    int exit_code = 0;

    if (show_links || strict_links) {
        TripletStore *store = build_relation_store(&list);
        if (!store) {
            fprintf(stderr, "error: failed to create relation store\n");
            req_list_free(&list);
            return 1;
        }
        if (show_links)
            list_relations(store);
        if (strict_links) {
            int warnings = check_strict_links(store);
            if (warnings > 0) {
                fprintf(stderr, "%d strict-links warning(s) found.\n", warnings);
                exit_code = 1;
            }
        }
        triplet_store_destroy(store);
    } else {
        list_requirements(&list);
    }

    req_list_free(&list);
    return exit_code;
}
