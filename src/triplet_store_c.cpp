/**
 * @file triplet_store_c.cpp
 * @brief C API wrapper for the vibe::TripletStore C++ class.
 *
 * All functions in this file are compiled as C++ so that they can
 * construct and call into the C++ class, but they are exported with
 * C linkage (declared in triplet_store_c.h) so that C translation
 * units can link against them without name mangling.
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
 */
static CTripleList make_list(const std::vector<const vibe::Triple *> &src)
{
    CTripleList list{};
    if (src.empty()) return list;

    list.triples = static_cast<CTriple *>(
        std::calloc(src.size(), sizeof(CTriple)));
    if (!list.triples) return list;

    for (std::size_t i = 0; i < src.size(); ++i) {
        const vibe::Triple *t = src[i];
        list.triples[i].id        = t->id;
        list.triples[i].subject   = dup_str(t->subject);
        list.triples[i].predicate = dup_str(t->predicate);
        list.triples[i].object    = dup_str(t->object);
        list.triples[i].inferred  = t->inferred ? 1 : 0;

        /* On allocation failure clean up everything allocated so far. */
        if (!list.triples[i].subject  ||
            !list.triples[i].predicate ||
            !list.triples[i].object) {
            for (std::size_t j = 0; j <= i; ++j) {
                std::free(list.triples[j].subject);
                std::free(list.triples[j].predicate);
                std::free(list.triples[j].object);
            }
            std::free(list.triples);
            list.triples = nullptr;
            list.count   = 0;
            return list;
        }
    }

    list.count = src.size();
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

    vibe::TripleId id = to_cpp(store)->add(subject, predicate, object);
    return (id == vibe::INVALID_TRIPLE_ID) ? TRIPLE_ID_INVALID : id;
}

int triplet_store_remove(TripletStore *store, size_t id)
{
    if (!store) return 0;
    return to_cpp(store)->remove(id) ? 1 : 0;
}

size_t triplet_store_remove_by_subject(TripletStore *store, const char *subject)
{
    if (!store || !subject) return 0;
    return to_cpp(store)->remove_by_subject(subject);
}

size_t triplet_store_remove_by_object(TripletStore *store, const char *object)
{
    if (!store || !object) return 0;
    return to_cpp(store)->remove_by_object(object);
}

size_t triplet_store_remove_by_predicate(TripletStore *store,
                                         const char   *predicate)
{
    if (!store || !predicate) return 0;
    return to_cpp(store)->remove_by_predicate(predicate);
}

void triplet_store_clear(TripletStore *store)
{
    if (store) to_cpp(store)->clear();
}

size_t triplet_store_compact(TripletStore *store)
{
    if (!store) return 0;
    return to_cpp(store)->compact();
}

size_t triplet_store_infer_inverses(TripletStore *store)
{
    if (!store) return 0;
    return to_cpp(store)->infer_inverses();
}

/* -------------------------------------------------------------------------
 * Query
 * ---------------------------------------------------------------------- */

CTripleList triplet_store_find_by_subject(const TripletStore *store,
                                          const char         *subject)
{
    if (!store || !subject) return CTripleList{};
    return make_list(to_cpp(store)->find_by_subject(subject));
}

CTripleList triplet_store_find_by_object(const TripletStore *store,
                                         const char         *object)
{
    if (!store || !object) return CTripleList{};
    return make_list(to_cpp(store)->find_by_object(object));
}

CTripleList triplet_store_find_by_predicate(const TripletStore *store,
                                            const char         *predicate)
{
    if (!store || !predicate) return CTripleList{};
    return make_list(to_cpp(store)->find_by_predicate(predicate));
}

CTripleList triplet_store_find_all(const TripletStore *store)
{
    if (!store) return CTripleList{};
    return make_list(to_cpp(store)->find_all());
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
