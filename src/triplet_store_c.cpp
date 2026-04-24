/**
 * @file triplet_store_c.cpp
 * @brief C API wrapper for the vibe::TripletStore C++ class.
 *
 * All functions in this file are compiled as C++ so that they can
 * construct and call into the C++ class, but they are exported with
 * C linkage (declared in triplet_store_c.h) so that C translation
 * units can link against them without name mangling.
 *
 * Exception safety
 * ----------------
 * All public API functions catch every C++ exception and convert it to
 * a safe error return value (NULL, 0, or TRIPLE_ID_INVALID as appropriate).
 * This prevents undefined behaviour that would result from a C++ exception
 * propagating across the C language boundary.
 *
 * Ownership and lifetime
 * ----------------------
 * - @c triplet_store_create() returns a heap-allocated store; the caller
 *   owns it and must release it with @c triplet_store_destroy().
 * - @c triplet_store_destroy(NULL) is a no-op.
 * - Query functions return a @c CTripleList that contains heap-allocated
 *   copies of all string fields.  The caller owns the list and must release
 *   it with @c triplet_store_list_free() when it is no longer needed.
 * - Pointers inside a @c CTripleList are owned by the list; do not free
 *   individual string fields.
 * - @c triplet_store_list_free() accepts lists with @c triples == NULL
 *   (empty or error-path result) as a no-op.
 */

#include "triplet_store_c.h"
#include "triplet_store.hpp"

#include <cstdlib>
#include <cstring>
#include <new>

/*
 * The C header declares TripletStore as an opaque struct.  The C++ class is
 * vibe::TripletStore.  We bridge them with reinterpret_cast so that the
 * opaque pointer pattern works correctly across the language boundary.
 */
static vibe::TripletStore *to_cpp(TripletStore *s)
{
    return reinterpret_cast<vibe::TripletStore *>(s);
}

static const vibe::TripletStore *to_cpp(const TripletStore *s)
{
    return reinterpret_cast<const vibe::TripletStore *>(s);
}

static TripletStore *from_cpp(vibe::TripletStore *s)
{
    return reinterpret_cast<TripletStore *>(s);
}

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

/** Duplicate a std::string into a heap-allocated C string. */
static char *dup_str(const std::string &s)
{
    char *p = static_cast<char *>(std::malloc(s.size() + 1));
    if (p) std::memcpy(p, s.c_str(), s.size() + 1);
    return p;
}

/**
 * Convert a vector of Triple pointers into an owned CTripleList.
 *
 * Uses calloc so that partially-initialised entries have NULL string
 * pointers; this makes the cleanup loop in the error path safe.
 *
 * On any allocation failure the function frees all memory it has
 * already allocated and returns an empty list ({NULL, 0}).  Each
 * dup_str call is checked immediately so that no further allocations
 * are attempted after the first failure.
 */
static CTripleList make_list(const std::vector<const vibe::Triple *> &src)
{
    CTripleList list{};
    if (src.empty()) return list;

    list.triples = static_cast<CTriple *>(
        std::calloc(src.size(), sizeof(CTriple)));
    if (!list.triples) return list;

    /* Track which entry failed so the cleanup loop can stop early. */
    std::size_t failed_at = 0;

    for (std::size_t i = 0; i < src.size(); ++i) {
        const vibe::Triple *t = src[i];
        list.triples[i].id = t->id;

        /* Allocate each string individually and fail fast on the first error
         * to avoid wasting memory after an OOM condition has been detected.
         * The inferred flag is set only after all strings succeed, keeping
         * each entry either fully initialised or left in the calloc-zero state
         * (all-NULL strings) for safe cleanup. */
        list.triples[i].subject = dup_str(t->subject);
        if (!list.triples[i].subject) { failed_at = i; goto oom; }

        list.triples[i].predicate = dup_str(t->predicate);
        if (!list.triples[i].predicate) { failed_at = i; goto oom; }

        list.triples[i].object = dup_str(t->object);
        if (!list.triples[i].object) { failed_at = i; goto oom; }

        list.triples[i].inferred = t->inferred ? 1 : 0;
    }

    list.count = src.size();
    return list;

oom:
    /* Release all strings up to and including the failing entry.  Entries
     * beyond failed_at were never touched; their pointers are NULL from
     * calloc so std::free() on them would be safe, but we skip them to
     * avoid unnecessary work. */
    for (std::size_t j = 0; j <= failed_at; ++j) {
        std::free(list.triples[j].subject);
        std::free(list.triples[j].predicate);
        std::free(list.triples[j].object);
    }
    std::free(list.triples);
    list.triples = nullptr;
    list.count   = 0;
    return list;
}

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

TripletStore *triplet_store_create(void)
{
    try {
        return from_cpp(new vibe::TripletStore());
    } catch (...) {
        return nullptr;
    }
}

void triplet_store_destroy(TripletStore *store)
{
    delete to_cpp(store);
}

/* -------------------------------------------------------------------------
 * Mutation
 * ---------------------------------------------------------------------- */

size_t triplet_store_add(TripletStore *store,
                         const char   *subject,
                         const char   *predicate,
                         const char   *object)
{
    if (!store || !subject || !predicate || !object) return TRIPLE_ID_INVALID;

    try {
        vibe::TripleId id = to_cpp(store)->add(subject, predicate, object);
        return (id == vibe::INVALID_TRIPLE_ID) ? TRIPLE_ID_INVALID : id;
    } catch (...) {
        return TRIPLE_ID_INVALID;
    }
}

int triplet_store_remove(TripletStore *store, size_t id)
{
    if (!store) return 0;
    try {
        return to_cpp(store)->remove(id) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

size_t triplet_store_remove_by_subject(TripletStore *store, const char *subject)
{
    if (!store || !subject) return 0;
    try {
        return to_cpp(store)->remove_by_subject(subject);
    } catch (...) {
        return 0;
    }
}

size_t triplet_store_remove_by_object(TripletStore *store, const char *object)
{
    if (!store || !object) return 0;
    try {
        return to_cpp(store)->remove_by_object(object);
    } catch (...) {
        return 0;
    }
}

size_t triplet_store_remove_by_predicate(TripletStore *store,
                                         const char   *predicate)
{
    if (!store || !predicate) return 0;
    try {
        return to_cpp(store)->remove_by_predicate(predicate);
    } catch (...) {
        return 0;
    }
}

void triplet_store_clear(TripletStore *store)
{
    if (store) to_cpp(store)->clear();
}

size_t triplet_store_compact(TripletStore *store)
{
    if (!store) return 0;
    try {
        return to_cpp(store)->compact();
    } catch (...) {
        return 0;
    }
}

size_t triplet_store_infer_inverses(TripletStore *store)
{
    if (!store) return 0;
    try {
        return to_cpp(store)->infer_inverses();
    } catch (...) {
        return 0;
    }
}

/* -------------------------------------------------------------------------
 * Query
 * ---------------------------------------------------------------------- */

CTripleList triplet_store_find_by_subject(const TripletStore *store,
                                          const char         *subject)
{
    if (!store || !subject) return CTripleList{};
    try {
        return make_list(to_cpp(store)->find_by_subject(subject));
    } catch (...) {
        return CTripleList{};
    }
}

CTripleList triplet_store_find_by_object(const TripletStore *store,
                                         const char         *object)
{
    if (!store || !object) return CTripleList{};
    try {
        return make_list(to_cpp(store)->find_by_object(object));
    } catch (...) {
        return CTripleList{};
    }
}

CTripleList triplet_store_find_by_predicate(const TripletStore *store,
                                            const char         *predicate)
{
    if (!store || !predicate) return CTripleList{};
    try {
        return make_list(to_cpp(store)->find_by_predicate(predicate));
    } catch (...) {
        return CTripleList{};
    }
}

CTripleList triplet_store_find_all(const TripletStore *store)
{
    if (!store) return CTripleList{};
    try {
        return make_list(to_cpp(store)->find_all());
    } catch (...) {
        return CTripleList{};
    }
}

void triplet_store_list_free(CTripleList list)
{
    if (!list.triples) return;
    for (size_t i = 0; i < list.count; ++i) {
        std::free(list.triples[i].subject);
        std::free(list.triples[i].predicate);
        std::free(list.triples[i].object);
    }
    std::free(list.triples);
}

/* -------------------------------------------------------------------------
 * Statistics
 * ---------------------------------------------------------------------- */

size_t triplet_store_count(const TripletStore *store)
{
    if (!store) return 0;
    return to_cpp(store)->count();
}

size_t triplet_store_slot_count(const TripletStore *store)
{
    if (!store) return 0;
    return to_cpp(store)->slot_count();
}
