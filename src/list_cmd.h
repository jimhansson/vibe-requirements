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
#include "triplet_store.hpp"

/**
 * Build a TripletStore from all traceability links declared in @p list.
 *
 * Iterates every entity in @p list and adds its traceability entries to a
 * newly-created TripletStore, then calls infer_inverses() to
 * populate the symmetric reverse relations.
 *
 * @param list  source entity list (may be empty)
 * @return      heap-allocated TripletStore; caller must delete it when done.
 *              Returns NULL on allocation failure.
 */
vibe::TripletStore *build_entity_relation_store(const EntityList *list);

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
void list_relations(const vibe::TripletStore *store);

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
void cmd_trace_entity(const EntityList *elist, const vibe::TripletStore *store,
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
int check_strict_links(const vibe::TripletStore *store);

/**
 * Collect a document entity and all entities that are members of it.
 *
 * The output always includes the document entity itself followed by every
 * entity whose relation graph contains a `part-of -> <doc_id>` edge. This
 * covers both `documents:` YAML membership and explicit `part-of` /
 * `contains` traceability relations.
 *
 * @param all     source entity list containing the full discovered dataset
 * @param store   TripletStore populated from @p all and with inverses inferred
 * @param doc_id  document entity identifier to collect
 * @param out     destination list initialised by the caller
 * @return        0 on success
 *               -1 if no entity with @p doc_id exists
 *               -2 if the matching entity is not a document
 *               -3 on allocation failure while building @p out
 */
int collect_document_entities(const EntityList *all,
                               const vibe::TripletStore *store,
                              const char *doc_id, EntityList *out);


#endif /* VIBE_LIST_CMD_H */
