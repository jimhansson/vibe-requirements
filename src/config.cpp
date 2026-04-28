#include "config.h"
#include <yaml.h>
#include <cstdio>
#include <cstring>

int config_load(const char *root_dir, VibeConfig *cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    if (!root_dir)
        return -1;

    /* Build path: <root_dir>/.vibe-req.yaml */
    char path[CONFIG_PATH_BUF_LEN];
    int n = snprintf(path, sizeof(path), "%s/%s", root_dir, VIBE_CONFIG_FILENAME);
    if (n < 0 || static_cast<size_t>(n) >= sizeof(path))
        return -1;

    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    yaml_parser_t   parser;
    yaml_document_t doc;

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);

    if (!yaml_parser_load(&parser, &doc)) {
        yaml_parser_delete(&parser);
        fclose(f);
        return -1;
    }

    yaml_node_t *root = yaml_document_get_root_node(&doc);
    if (root && root->type == YAML_MAPPING_NODE) {
        yaml_node_pair_t *pair = root->data.mapping.pairs.start;
        yaml_node_pair_t *end  = root->data.mapping.pairs.top;

        for (; pair < end; pair++) {
            yaml_node_t *key_node = yaml_document_get_node(&doc, pair->key);
            if (!key_node || key_node->type != YAML_SCALAR_NODE)
                continue;

            const char *key = reinterpret_cast<const char *>(key_node->data.scalar.value);
            if (strcmp(key, "ignore_dirs") != 0)
                continue;

            yaml_node_t *seq = yaml_document_get_node(&doc, pair->value);
            if (!seq || seq->type != YAML_SEQUENCE_NODE)
                break;

            yaml_node_item_t *item = seq->data.sequence.items.start;
            yaml_node_item_t *top  = seq->data.sequence.items.top;

            for (; item < top && cfg->ignore_dirs_count < CONFIG_IGNORE_DIRS_MAX;
                 item++) {
                yaml_node_t *entry = yaml_document_get_node(&doc, *item);
                if (!entry || entry->type != YAML_SCALAR_NODE)
                    continue;

                const char *val = reinterpret_cast<const char *>(entry->data.scalar.value);
                strncpy(cfg->ignore_dirs[cfg->ignore_dirs_count],
                        val, CONFIG_IGNORE_DIR_LEN - 1);
                cfg->ignore_dirs[cfg->ignore_dirs_count]
                                [CONFIG_IGNORE_DIR_LEN - 1] = '\0';
                cfg->ignore_dirs_count++;
            }
            break;
        }
    }

    yaml_document_delete(&doc);
    yaml_parser_delete(&parser);
    fclose(f);
    return 0;
}

int config_is_ignored_dir(const VibeConfig *cfg, const char *dir_name)
{
    if (!cfg || !dir_name)
        return 0;

    for (int i = 0; i < cfg->ignore_dirs_count; i++) {
        if (strcmp(cfg->ignore_dirs[i], dir_name) == 0)
            return 1;
    }
    return 0;
}
