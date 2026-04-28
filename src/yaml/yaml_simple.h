/**
 * @file yaml/yaml_simple.h
 * @brief Public API for the YAML parsing layer.
 */

#ifndef VIBE_YAML_SIMPLE_H
#define VIBE_YAML_SIMPLE_H

#include "../triplet_store.hpp"
#include "../entity.h"

int yaml_parse_entity(const char *path, Entity *out);
int yaml_parse_entities(const char *path, EntityList *list);
int entity_traceability_to_triplets(const Entity *entity,
                                    vibe::TripletStore *store);
int entity_doc_membership_to_triplets(const Entity *entity,
                                      vibe::TripletStore *store);
int yaml_parse_links(const char *path, const char *subject_id,
                     vibe::TripletStore *store);

#endif /* VIBE_YAML_SIMPLE_H */
