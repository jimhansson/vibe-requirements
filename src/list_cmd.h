/**
 * @file list_cmd.h
 * @brief Entity/relation table rendering and trace helpers for the vibe-req CLI.
 *
 * Declares functions extracted from main.c so that they can be independently
 * unit-tested and reused.  All functions print to stdout (or stderr for
 * warnings).
 */

#ifndef VIBE_LIST_CMD_H
#define VIBE_LIST_CMD_H

#include "entity.h"
#include "triplet_store_c.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Build a TripletStore from all traceability links declared in @p list.
 *
 * Iterates every entity in @p list and adds its traceability entries to a
 * newly-created TripletStore, then calls triplet_store_infer_inverses() to
 * populate the symmetric reverse relations.
 *
 * @param list  source entity list (may be empty)
 * @return      heap-allocated TripletStore; caller must call
 *              triplet_store_destroy() when done.
 *              Returns NULL on allocation failure.
 */
TripletStore *build_entity_relation_store(const EntityList *list);

/**
 * Print all entities in @p list as a formatted ASCII table to stdout.
 *
 * Columns: ID | Title | Kind | Status | Priority.  Titles longer than 48
 * characters are truncated with "…".  Prints "No entities found." when the
 * list is empty.
 *
 * @param list  entity list to display (may be empty)
 */
void list_entities(const EntityList *list);

/**
 * Print all relations in @p store as a formatted ASCII table to stdout.
 *
 * Columns: Subject | Relation | Object | Source.  Subjects longer than 32
 * characters and objects longer than 48 characters are truncated.  Prints
 * "No relations found." when the store is empty.
 *
 * @param store  populated TripletStore (may be empty)
 */
void list_relations(const TripletStore *store);

/**
 * Show the full traceability chain for the entity identified by @p id.
 *
 * Looks up the entity in @p elist to display its kind, title, and status.
 * Then prints all outgoing (declared) links from @p store and all incoming
 * (declared) links.
 *
 * @param elist  entity list to search for entity metadata (may be empty)
 * @param store  TripletStore containing all relation triples
 * @param id     entity identifier to trace (must not be NULL)
 */
void cmd_trace_entity(const EntityList *elist, const TripletStore *store,
                      const char *id);

/**
 * Validate that every declared bidirectional link has its inverse declared.
 *
 * For every user-declared triple (A, rel, B) where @p rel belongs to a
 * known symmetric pair, emits a warning to stderr when (B, inv(rel), A) is
 * absent from the declared triples.
 *
 * @param store  TripletStore populated and with inverses already inferred
 * @return       number of one-sided link warnings found (0 = all good)
 */
int check_strict_links(const TripletStore *store);

#ifdef __cplusplus
}
#endif

#endif /* VIBE_LIST_CMD_H */
