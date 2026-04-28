#include "discovery.h"
#include "config.h"
#include "yaml_simple.h"
#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>

static int is_yaml(const char *name)
{
    const char *dot = strrchr(name, '.');
    if (!dot)
        return 0;
    return strcmp(dot, ".yaml") == 0 || strcmp(dot, ".yml") == 0;
}

static int walk(const char *dir, EntityList *list, const VibeConfig *cfg,
                int fail_fast)
{
    DIR *d = opendir(dir);
    if (!d)
        return 0;

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
            if (walk(path, list, cfg, fail_fast) < 0) {
                closedir(d);
                return -1;
            }
        } else if (S_ISREG(st.st_mode) && is_yaml(name)) {
            if (yaml_parse_entities(path, list) < 0) {
                fprintf(stderr, "%s: could not parse: %s\n",
                        fail_fast ? "error" : "warning", path);
                if (fail_fast) {
                    closedir(d);
                    return -1;
                }
            }
        }
    }

    closedir(d);
    return 0;
}

int discover_entities(const char *root_dir, EntityList *list,
                      const VibeConfig *cfg)
{
    return discover_entities_with_options(root_dir, list, cfg, 0);
}

int discover_entities_with_options(const char *root_dir, EntityList *list,
                                   const VibeConfig *cfg, int fail_fast)
{
    DIR *probe = opendir(root_dir);
    if (!probe)
        return -1;
    closedir(probe);

    if (walk(root_dir, list, cfg, fail_fast) < 0)
        return -2;

    return (int)list->size();
}
