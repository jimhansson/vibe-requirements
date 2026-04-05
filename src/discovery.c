#include "discovery.h"
#include "yaml_simple.h"
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

static int is_yaml(const char *name)
{
    const char *dot = strrchr(name, '.');
    if (!dot)
        return 0;
    return strcmp(dot, ".yaml") == 0 || strcmp(dot, ".yml") == 0;
}

static void walk(const char *dir, RequirementList *list)
{
    DIR *d = opendir(dir);
    if (!d)
        return;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        const char *name = entry->d_name;

        /* Skip hidden entries and the special "." / ".." entries. */
        if (name[0] == '.')
            continue;

        char path[REQ_PATH_LEN];
        int n = snprintf(path, sizeof(path), "%s/%s", dir, name);
        if (n < 0 || (size_t)n >= sizeof(path)) {
            fprintf(stderr, "warning: path too long, skipping: %s/%s\n", dir, name);
            continue;
        }

        struct stat st;
        if (stat(path, &st) != 0)
            continue;

        if (S_ISDIR(st.st_mode)) {
            walk(path, list);
        } else if (S_ISREG(st.st_mode) && is_yaml(name)) {
            Requirement req;
            if (yaml_parse_requirement(path, &req) == 0) {
                if (req_list_add(list, &req) != 0)
                    fprintf(stderr, "warning: out of memory, skipping: %s\n", path);
            }
        }
    }

    closedir(d);
}

int discover_requirements(const char *root_dir, RequirementList *list)
{
    /* Probe that the directory exists before recursing. */
    DIR *probe = opendir(root_dir);
    if (!probe)
        return -1;
    closedir(probe);

    walk(root_dir, list);
    return list->count;
}
