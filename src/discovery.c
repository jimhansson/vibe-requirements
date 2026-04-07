#include "discovery.h"
#include "config.h"
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

static void walk(const char *dir, RequirementList *list, const VibeConfig *cfg)
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
            /* Skip directories listed in ignore_dirs. */
            if (config_is_ignored_dir(cfg, name))
                continue;
            walk(path, list, cfg);
        } else if (S_ISREG(st.st_mode) && is_yaml(name)) {
            if (yaml_parse_requirements(path, list) < 0)
                fprintf(stderr, "warning: could not parse: %s\n", path);
        }
    }

    closedir(d);
}

int discover_requirements(const char *root_dir, RequirementList *list,
                          const VibeConfig *cfg)
{
    /* Probe that the directory exists before recursing. */
    DIR *probe = opendir(root_dir);
    if (!probe)
        return -1;
    closedir(probe);

    walk(root_dir, list, cfg);
    return list->count;
}
