/*
 * @file yaml_helpers.c
 * @brief Common buffer-handling and YAML-sequence helper implementations.
 *
 * These utilities reduce duplication across the entity and link parser
 * modules.  They must NOT be included by external consumers directly —
 * use yaml_simple.h instead.
 */

#include "yaml_helpers.h"
#include <string.h>

void yaml_copy_field(char *dst, size_t dst_size, const char *src)
{
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

int yaml_append_to_flat(char *buf, size_t buf_size, int *count,
                        const char *text)
{
    size_t cur_len  = strlen(buf);
    size_t text_len = strlen(text);

    /* Need room for text + separator newline (if buffer is non-empty). */
    size_t need = text_len + (cur_len > 0 ? 1u : 0u);

    if (cur_len + need >= buf_size)
        return 0; /* not enough space */

    if (cur_len > 0)
        buf[cur_len++] = '\n';

    memcpy(buf + cur_len, text, text_len);
    buf[cur_len + text_len] = '\0';
    (*count)++;
    return 1;
}

int yaml_append_pair_entry(char *buf, size_t buf_size, int *count,
                           const char *first, const char *second)
{
    size_t cur_len = strlen(buf);
    size_t f_len   = strlen(first);
    size_t s_len   = strlen(second);
    /* Need: [leading newline] + first + '\t' + second + NUL (1 byte) */
    size_t need = f_len + 1u + s_len + 1u + (cur_len > 0 ? 1u : 0u);

    if (cur_len + need >= buf_size)
        return 0;

    if (cur_len > 0)
        buf[cur_len++] = '\n';

    memcpy(buf + cur_len, first, f_len);
    cur_len += f_len;
    buf[cur_len++] = '\t';
    memcpy(buf + cur_len, second, s_len);
    buf[cur_len + s_len] = '\0';
    (*count)++;
    return 1;
}

void yaml_collect_sequence(yaml_document_t *doc, yaml_node_t *seq,
                            char *buf, size_t buf_size, int *count)
{
    if (!seq || seq->type != YAML_SEQUENCE_NODE)
        return;

    yaml_node_item_t *item = seq->data.sequence.items.start;
    yaml_node_item_t *top  = seq->data.sequence.items.top;

    for (; item < top; item++) {
        yaml_node_t *node = yaml_document_get_node(doc, *item);
        if (!node || node->type != YAML_SCALAR_NODE)
            continue;
        const char *val = (const char *)node->data.scalar.value;
        yaml_append_to_flat(buf, buf_size, count, val);
    }
}
