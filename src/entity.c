#include "entity.h"
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * EntityList lifecycle
 * --------------------------------------------------------------------- */

void entity_list_init(EntityList *list)
{
    list->items    = NULL;
    list->count    = 0;
    list->capacity = 0;
}

int entity_list_add(EntityList *list, const Entity *entity)
{
    if (list->count == list->capacity) {
        int new_cap = (list->capacity == 0) ? 16 : list->capacity * 2;
        Entity *tmp = realloc(list->items, (size_t)new_cap * sizeof(Entity));
        if (!tmp)
            return -1;
        list->items    = tmp;
        list->capacity = new_cap;
    }
    list->items[list->count++] = *entity;
    return 0;
}

void entity_list_free(EntityList *list)
{
    free(list->items);
    list->items    = NULL;
    list->count    = 0;
    list->capacity = 0;
}

/* -----------------------------------------------------------------------
 * Kind helpers
 * --------------------------------------------------------------------- */

EntityKind entity_kind_from_string(const char *type_str)
{
    if (!type_str || type_str[0] == '\0')
        return ENTITY_KIND_REQUIREMENT;

    if (strcmp(type_str, "functional")    == 0 ||
        strcmp(type_str, "non-functional") == 0 ||
        strcmp(type_str, "nonfunctional")  == 0)
        return ENTITY_KIND_REQUIREMENT;

    if (strcmp(type_str, "group") == 0)
        return ENTITY_KIND_GROUP;

    if (strcmp(type_str, "story")      == 0 ||
        strcmp(type_str, "user-story") == 0)
        return ENTITY_KIND_STORY;

    if (strcmp(type_str, "design-note") == 0 ||
        strcmp(type_str, "design_note") == 0 ||
        strcmp(type_str, "design")      == 0)
        return ENTITY_KIND_DESIGN_NOTE;

    if (strcmp(type_str, "section") == 0)
        return ENTITY_KIND_SECTION;

    if (strcmp(type_str, "assumption") == 0)
        return ENTITY_KIND_ASSUMPTION;

    if (strcmp(type_str, "constraint") == 0)
        return ENTITY_KIND_CONSTRAINT;

    if (strcmp(type_str, "test-case") == 0 ||
        strcmp(type_str, "test_case") == 0 ||
        strcmp(type_str, "test")      == 0)
        return ENTITY_KIND_TEST_CASE;

    if (strcmp(type_str, "external")   == 0 ||
        strcmp(type_str, "directive")  == 0 ||
        strcmp(type_str, "standard")   == 0 ||
        strcmp(type_str, "regulation") == 0)
        return ENTITY_KIND_EXTERNAL;

    return ENTITY_KIND_UNKNOWN;
}

const char *entity_kind_label(EntityKind kind)
{
    switch (kind) {
    case ENTITY_KIND_REQUIREMENT: return "requirement";
    case ENTITY_KIND_GROUP:       return "group";
    case ENTITY_KIND_STORY:       return "story";
    case ENTITY_KIND_DESIGN_NOTE: return "design-note";
    case ENTITY_KIND_SECTION:     return "section";
    case ENTITY_KIND_ASSUMPTION:  return "assumption";
    case ENTITY_KIND_CONSTRAINT:  return "constraint";
    case ENTITY_KIND_TEST_CASE:   return "test-case";
    case ENTITY_KIND_EXTERNAL:    return "external";
    default:                      return "unknown";
    }
}
