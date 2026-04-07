/**
 * @file triplet_store_c.h
 * @brief C language API for the relation triplet store.
 *
 * The triplet store holds (subject, predicate, object) relation records and
 * provides indexed lookup by any component for efficient traceability
 * queries (see design doc section 5).
 *
 * This header is the C-language interface to the C++ TripletStore class.
 * C++ callers may use triplet_store.hpp directly.
 *
 * Memory model
 * ------------
 * Query functions return a @c CTripleList that owns heap-allocated copies of
 * all string fields.  Release each result with @c triplet_store_list_free()
 * when it is no longer needed.
 */

#ifndef TRIPLET_STORE_C_H
#define TRIPLET_STORE_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/** Sentinel value returned when an operation fails to produce a valid ID. */
#define TRIPLE_ID_INVALID ((size_t)(-1))

/** Opaque handle to a TripletStore instance. */
typedef struct TripletStore TripletStore;

/**
 * A single triple record returned by query functions.
 *
 * All string pointers are heap-allocated copies owned by the enclosing
 * @c CTripleList.  Do @em not free them individually; use
 * @c triplet_store_list_free() to release the whole list.
 */
typedef struct {
    size_t  id;        /**< Stable triple identifier. */
    char   *subject;   /**< Subject entity ID (e.g. "REQ-SW-001"). */
    char   *predicate; /**< Relation / predicate name (e.g. "derives-from"). */
    char   *object;    /**< Object entity ID or artefact path. */
    int     inferred;  /**< Non-zero if this triple was synthetically generated
                            by inverse inference; zero if user-declared. */
} CTriple;

/**
 * A list of triple records returned by a query function.
 *
 * Release with @c triplet_store_list_free().
 */
typedef struct {
    CTriple *triples; /**< Array of triple records; NULL when count == 0. */
    size_t   count;   /**< Number of elements in triples[]. */
} CTripleList;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

/**
 * Create a new, empty triplet store.
 * @return  Pointer to the new store, or NULL on allocation failure.
 */
TripletStore *triplet_store_create(void);

/**
 * Destroy @p store and release all memory it owns.
 * Passing NULL is a no-op.
 */
void triplet_store_destroy(TripletStore *store);

/* =========================================================================
 * Mutation
 * ====================================================================== */

/**
 * Add a (subject, predicate, object) triple to @p store.
 *
 * An exact duplicate triple (same subject, predicate, and object) is
 * rejected to keep the graph consistent.
 *
 * @return  Stable ID of the new triple, or @c TRIPLE_ID_INVALID if the
 *          exact triple already exists or any argument is NULL.
 */
size_t triplet_store_add(TripletStore *store,
                         const char   *subject,
                         const char   *predicate,
                         const char   *object);

/**
 * Remove the triple identified by @p id from @p store.
 * @return  1 if the triple was found and removed, 0 otherwise.
 */
int triplet_store_remove(TripletStore *store, size_t id);

/**
 * Remove all triples in @p store whose subject equals @p subject.
 * @return  Number of triples removed.
 */
size_t triplet_store_remove_by_subject(TripletStore *store,
                                       const char   *subject);

/**
 * Remove all triples in @p store whose object equals @p object.
 * @return  Number of triples removed.
 */
size_t triplet_store_remove_by_object(TripletStore *store,
                                      const char   *object);

/**
 * Remove all triples in @p store whose predicate equals @p predicate.
 * @return  Number of triples removed.
 */
size_t triplet_store_remove_by_predicate(TripletStore *store,
                                         const char   *predicate);

/** Remove all triples from @p store. */
void triplet_store_clear(TripletStore *store);

/**
 * For every user-declared triple (A, rel, B) in @p store where @c rel has a
 * known inverse @c inv(rel) in the built-in relation-pair registry, add a
 * synthetic triple (B, inv(rel), A) marked as inferred — unless an identical
 * triple already exists.  Unknown or custom relation names are silently
 * skipped (Option C fallback).
 *
 * Call this once after all user-declared triples have been loaded.
 *
 * @return  Number of synthetic triples added, or 0 when @p store is NULL.
 */
size_t triplet_store_infer_inverses(TripletStore *store);

/* =========================================================================
 * Query
 * ====================================================================== */

/**
 * Find all triples with the given @p subject.
 *
 * The returned @c CTripleList must be released with
 * @c triplet_store_list_free() when no longer needed.
 */
CTripleList triplet_store_find_by_subject(const TripletStore *store,
                                          const char         *subject);

/**
 * Find all triples with the given @p object.
 *
 * The returned @c CTripleList must be released with
 * @c triplet_store_list_free() when no longer needed.
 */
CTripleList triplet_store_find_by_object(const TripletStore *store,
                                         const char         *object);

/**
 * Find all triples with the given @p predicate.
 *
 * The returned @c CTripleList must be released with
 * @c triplet_store_list_free() when no longer needed.
 */
CTripleList triplet_store_find_by_predicate(const TripletStore *store,
                                            const char         *predicate);

/**
 * Return all triples currently in @p store.
 *
 * The returned @c CTripleList must be released with
 * @c triplet_store_list_free() when no longer needed.
 */
CTripleList triplet_store_find_all(const TripletStore *store);

/**
 * Release the memory owned by a @c CTripleList returned by a query function.
 * Passing a list with @c triples == NULL or @c count == 0 is a no-op.
 */
void triplet_store_list_free(CTripleList list);

/* =========================================================================
 * Statistics
 * ====================================================================== */

/**
 * Return the number of active (non-removed) triples in @p store.
 * Returns 0 when @p store is NULL.
 */
size_t triplet_store_count(const TripletStore *store);

#ifdef __cplusplus
}
#endif

#endif /* TRIPLET_STORE_C_H */
