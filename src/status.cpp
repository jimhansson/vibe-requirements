/**
 * @file status.cpp
 * @brief Status summary command for the vibe-req CLI.
 */

#include "status.h"

#include <cstdio>
#include <cstring>
#include <map>
#include <string>

static const char *display_value(const std::string &value)
{
    return value.empty() ? "(none)" : value.c_str();
}

static int decimal_width(int value)
{
    int width = 1;
    while (value >= 10) {
        value /= 10;
        width++;
    }
    return width;
}

static void print_count_rule(int label_w, int count_w)
{
    putchar('+');
    for (int i = 0; i < label_w + 2; i++)
        putchar('-');
    putchar('+');
    for (int i = 0; i < count_w + 2; i++)
        putchar('-');
    puts("+");
}

static void print_count_row(int label_w, int count_w,
                            const char *label, int count)
{
    printf("| %-*s | %*d |\n", label_w, label, count_w, count);
}

static void print_map_counts(const char *header, const char *label_header,
                             const std::map<std::string, int> &counts)
{
    printf("\n%s:\n", header);

    if (counts.empty()) {
        printf("(none)\n");
        return;
    }

    int label_w = (int)strlen(label_header);
    int count_w = (int)strlen("Count");

    for (const auto &entry : counts) {
        int len = (int)strlen(display_value(entry.first));
        if (len > label_w)
            label_w = len;
        len = decimal_width(entry.second);
        if (len > count_w)
            count_w = len;
    }

    print_count_rule(label_w, count_w);
    printf("| %-*s | %-*s |\n", label_w, label_header, count_w, "Count");
    print_count_rule(label_w, count_w);

    for (const auto &entry : counts)
        print_count_row(label_w, count_w,
                        display_value(entry.first), entry.second);

    print_count_rule(label_w, count_w);
}

static void print_kind_counts(const int *kind_counts, int kind_count)
{
    printf("\nBy kind:\n");

    int rows = 0;
    for (int i = 0; i < kind_count; i++) {
        if (kind_counts[i] > 0)
            rows++;
    }

    if (rows == 0) {
        printf("(none)\n");
        return;
    }

    int kind_w  = (int)strlen("Kind");
    int count_w = (int)strlen("Count");

    for (int i = 0; i < kind_count; i++) {
        if (kind_counts[i] <= 0)
            continue;
        int len = (int)strlen(entity_kind_label((EntityKind)i));
        if (len > kind_w)
            kind_w = len;
        len = decimal_width(kind_counts[i]);
        if (len > count_w)
            count_w = len;
    }

    print_count_rule(kind_w, count_w);
    printf("| %-*s | %-*s |\n", kind_w, "Kind", count_w, "Count");
    print_count_rule(kind_w, count_w);

    for (int i = 0; i < kind_count; i++) {
        if (kind_counts[i] <= 0)
            continue;
        print_count_row(kind_w, count_w,
                        entity_kind_label((EntityKind)i), kind_counts[i]);
    }

    print_count_rule(kind_w, count_w);
}

void cmd_status(const EntityList *elist)
{
    constexpr int KIND_COUNT = ENTITY_KIND_UNKNOWN + 1;
    int kind_counts[KIND_COUNT] = {};
    std::map<std::string, int> status_counts;
    std::map<std::string, int> priority_counts;

    for (const auto &e : *elist) {
        status_counts[e.lifecycle.status]++;
        priority_counts[e.lifecycle.priority]++;

        int kind = (int)e.identity.kind;
        if (kind < 0 || kind >= KIND_COUNT)
            kind = ENTITY_KIND_UNKNOWN;
        kind_counts[kind]++;
    }

    printf("Status Summary\n");
    printf("==============\n");
    printf("Total entities: %d\n", (int)elist->size());

    print_map_counts("By status", "Status", status_counts);
    print_map_counts("By priority", "Priority", priority_counts);
    print_kind_counts(kind_counts, KIND_COUNT);
}
