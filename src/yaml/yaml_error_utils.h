/**
 * @file yaml_error_utils.h
 * @brief Shared YAML parse error reporting helpers.
 */

#ifndef VIBE_YAML_ERROR_UTILS_H
#define VIBE_YAML_ERROR_UTILS_H

#include <yaml.h>
#include <cstdio>

static inline void yaml_report_parse_error(const char *path,
                                           const yaml_parser_t *parser)
{
    if (!path || !parser)
        return;

    const char *problem = parser->problem ? parser->problem : "yaml parse error";
    size_t line = parser->problem_mark.line + 1;
    size_t column = parser->problem_mark.column + 1;

    fprintf(stderr, "error: %s:%zu:%zu: %s\n", path, line, column, problem);
}

#endif /* VIBE_YAML_ERROR_UTILS_H */
