#include "yaml_simple.h"
#include <stdio.h>
#include <string.h>

/* Remove trailing whitespace and line-ending characters. */
static void rtrim(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (unsigned char)s[n - 1] <= ' ')
        s[--n] = '\0';
}

/* Remove surrounding double-quotes from s in-place. */
static void unquote(char *s)
{
    if (s[0] != '"')
        return;
    char *end = strchr(s + 1, '"');
    if (!end) {
        /* Unclosed quote — just strip the leading '"'. */
        memmove(s, s + 1, strlen(s));
        return;
    }
    size_t len = (size_t)(end - (s + 1));
    memmove(s, s + 1, len);
    s[len] = '\0';
}

/*
 * Attempt to match a top-level scalar assignment "key: value" on line.
 * line – one raw text line (may include trailing newline)
 * key  – field name without colon (e.g. "id")
 * out  – destination buffer
 * size – size of destination buffer
 *
 * Returns:
 *   1  value found and stored in out
 *   2  block-scalar marker found ("|" or ">"); out is cleared
 *   0  this key is not on this line
 */
static int try_extract(const char *line, const char *key, char *out, size_t size)
{
    size_t klen = strlen(key);
    if (strncmp(line, key, klen) != 0)
        return 0;
    if (line[klen] != ':')
        return 0;

    const char *v = line + klen + 1;
    while (*v == ' ' || *v == '\t')
        v++;

    /* Block-scalar markers — value continues on following indented lines. */
    if (*v == '|' || *v == '>') {
        out[0] = '\0';
        return 2;
    }

    /* Empty value. */
    if (*v == '\0' || *v == '\r' || *v == '\n') {
        out[0] = '\0';
        return 1;
    }

    strncpy(out, v, size - 1);
    out[size - 1] = '\0';
    rtrim(out);
    unquote(out);
    return 1;
}

int yaml_parse_requirement(const char *path, Requirement *out)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    memset(out, 0, sizeof(*out));
    strncpy(out->file_path, path, REQ_PATH_LEN - 1);

    char line[4096];

    /* State for consuming block-scalar bodies. */
    int    in_block       = 0;
    char  *block_dst      = NULL;
    size_t block_max      = 0;
    int    block_got_line = 0; /* capture only the first content line */

    while (fgets(line, (int)sizeof(line), f)) {
        /* Count leading spaces to determine indent level. */
        int indent = 0;
        while (line[indent] == ' ' || line[indent] == '\t')
            indent++;

        /* Detect blank or comment lines. */
        int is_blank   = (line[indent] == '\n' || line[indent] == '\r' || line[indent] == '\0');
        int is_comment = (line[indent] == '#');

        if (in_block) {
            if (is_blank || is_comment) {
                /* A blank line ends the block scalar. */
                if (is_blank)
                    in_block = 0;
                continue;
            }
            if (indent > 0) {
                /* Still inside the block-scalar body. */
                if (!block_got_line && block_dst) {
                    strncpy(block_dst, line + indent, block_max - 1);
                    block_dst[block_max - 1] = '\0';
                    rtrim(block_dst);
                    block_got_line = 1;
                }
                continue;
            }
            /* indent == 0 and non-blank: new top-level key; exit block. */
            in_block = 0;
        }

        if (is_blank || is_comment)
            continue;

        /* Only inspect top-level keys (indent == 0). */
        if (indent != 0)
            continue;

        int r;

#define TRY(field, buf, bufsz)                                            \
        r = try_extract(line, field, out->buf, bufsz);                    \
        if (r == 1) continue;                                             \
        if (r == 2) {                                                     \
            in_block       = 1;                                           \
            block_dst      = out->buf;                                    \
            block_max      = bufsz;                                       \
            block_got_line = 0;                                           \
            continue;                                                     \
        }

        TRY("id",          id,          REQ_ID_LEN)
        TRY("title",       title,       REQ_TITLE_LEN)
        TRY("type",        type,        REQ_TYPE_LEN)
        TRY("status",      status,      REQ_STATUS_LEN)
        TRY("priority",    priority,    REQ_PRIORITY_LEN)
        TRY("description", description, REQ_DESC_LEN)

#undef TRY
    }

    fclose(f);
    return (out->id[0] != '\0') ? 0 : -1;
}
