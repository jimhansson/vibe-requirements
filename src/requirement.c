#include "requirement.h"
#include <stdlib.h>
#include <string.h>

void req_list_init(RequirementList *list)
{
    list->items    = NULL;
    list->count    = 0;
    list->capacity = 0;
}

int req_list_add(RequirementList *list, const Requirement *req)
{
    if (list->count == list->capacity) {
        int new_cap = (list->capacity == 0) ? 16 : list->capacity * 2;
        Requirement *tmp = realloc(list->items, (size_t)new_cap * sizeof(Requirement));
        if (!tmp)
            return -1;
        list->items    = tmp;
        list->capacity = new_cap;
    }
    list->items[list->count++] = *req;
    return 0;
}

void req_list_free(RequirementList *list)
{
    free(list->items);
    list->items    = NULL;
    list->count    = 0;
    list->capacity = 0;
}
