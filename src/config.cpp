#include "config.h"
#include <yaml.h>
#include <cstdio>
#include <cstring>
#include "yaml/yaml_error_utils.h"
#include "string_utils.h"

static int find_vocab_index(const VibeConfig *cfg, const char *field)
{
    if (!cfg || !field)
        return -1;
    for (int i = 0; i < cfg->vocabulary_count; i++) {
        if (str_equal_ci(cfg->vocabulary[i].field, field))
            return i;
    }
    return -1;
}

static void add_vocab_value(VibeConfig *cfg,
                            const char *field,
                            const char *value)
{
    if (!cfg || !field || field[0] == '\0' || !value || value[0] == '\0')
        return;

    int idx = find_vocab_index(cfg, field);
    if (idx < 0) {
        if (cfg->vocabulary_count >= CONFIG_VOCAB_FIELDS_MAX)
            return;
        idx = cfg->vocabulary_count++;
        strncpy(cfg->vocabulary[idx].field, field,
                CONFIG_VOCAB_FIELD_LEN - 1);
        cfg->vocabulary[idx].field[CONFIG_VOCAB_FIELD_LEN - 1] = '\0';
        cfg->vocabulary[idx].value_count = 0;
    }

    if (cfg->vocabulary[idx].value_count >= CONFIG_VOCAB_VALUES_MAX)
        return;

    int value_idx = cfg->vocabulary[idx].value_count++;
    strncpy(cfg->vocabulary[idx].values[value_idx], value,
            CONFIG_VOCAB_VALUE_LEN - 1);
    cfg->vocabulary[idx].values[value_idx][CONFIG_VOCAB_VALUE_LEN - 1] = '\0';
}

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
        yaml_report_parse_error(path, &parser);
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
            if (strcmp(key, "ignore_dirs") == 0) {
                yaml_node_t *seq = yaml_document_get_node(&doc, pair->value);
                if (!seq || seq->type != YAML_SEQUENCE_NODE)
                    continue;

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
                continue;
            }

            if (strcmp(key, "requiredFields") == 0) {
                yaml_node_t *seq = yaml_document_get_node(&doc, pair->value);
                if (!seq || seq->type != YAML_SEQUENCE_NODE)
                    continue;

                yaml_node_item_t *item = seq->data.sequence.items.start;
                yaml_node_item_t *top  = seq->data.sequence.items.top;

                for (; item < top &&
                       cfg->required_fields_count < CONFIG_REQUIRED_FIELDS_MAX;
                     item++) {
                    yaml_node_t *entry = yaml_document_get_node(&doc, *item);
                    if (!entry || entry->type != YAML_SCALAR_NODE)
                        continue;

                    const char *val =
                        reinterpret_cast<const char *>(entry->data.scalar.value);
                    if (!val || val[0] == '\0')
                        continue;
                    strncpy(cfg->required_fields[cfg->required_fields_count],
                            val, CONFIG_REQUIRED_FIELD_LEN - 1);
                    cfg->required_fields[cfg->required_fields_count]
                                        [CONFIG_REQUIRED_FIELD_LEN - 1] = '\0';
                    cfg->required_fields_count++;
                }
                continue;
            }

            if (strcmp(key, "vocabulary") == 0) {
                yaml_node_t *vocab = yaml_document_get_node(&doc, pair->value);
                if (!vocab || vocab->type != YAML_MAPPING_NODE)
                    continue;

                for (yaml_node_pair_t *vp = vocab->data.mapping.pairs.start;
                     vp < vocab->data.mapping.pairs.top; ++vp) {
                    yaml_node_t *vk = yaml_document_get_node(&doc, vp->key);
                    yaml_node_t *vv = yaml_document_get_node(&doc, vp->value);
                    if (!vk || vk->type != YAML_SCALAR_NODE)
                        continue;
                    const char *field =
                        reinterpret_cast<const char *>(vk->data.scalar.value);
                    if (!vv || vv->type != YAML_SEQUENCE_NODE)
                        continue;
                    for (yaml_node_item_t *it = vv->data.sequence.items.start;
                         it < vv->data.sequence.items.top; ++it) {
                        yaml_node_t *entry = yaml_document_get_node(&doc, *it);
                        if (!entry || entry->type != YAML_SCALAR_NODE)
                            continue;
                        const char *val =
                            reinterpret_cast<const char *>(entry->data.scalar.value);
                        add_vocab_value(cfg, field, val);
                    }
                }
                continue;
            }
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
