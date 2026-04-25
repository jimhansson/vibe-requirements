/**
 * @file triplet_store.cpp
 * @brief Implementation of the relation triplet store.
 */

#include "triplet_store.hpp"

#include <algorithm>

namespace vibe {

/* -------------------------------------------------------------------------
 * Built-in relation-pair registry (Option B — inferred inverse).
 *
 * Each entry maps a forward relation name to its inverse.  Symmetric
 * relations (e.g. "conflicts-with") appear with the same string on both
 * sides.  Both directions are listed explicitly so a single lookup suffices.
 *
 * The table can be extended at the project level via .vibe-req.yaml
 * (future work); for now only built-in pairs are supported.
 * ---------------------------------------------------------------------- */

static const struct { const char *forward; const char *inverse; }
k_relation_pairs[] = {
    { "derives-from",          "derived-to"             },
    { "derived-to",            "derives-from"           },
    { "parent",                "child"                  },
    { "child",                 "parent"                 },
    { "verifies",              "verified-by"            },
    { "verified-by",           "verifies"               },
    { "implements",            "implemented-by"         },
    { "implemented-by",        "implements"             },
    { "implemented-in",        "implemented-by-artefact"},
    { "implemented-by-artefact","implemented-in"        },
    { "conflicts-with",        "conflicts-with"         }, /* symmetric */
    { "refines",               "refined-by"             },
    { "refined-by",            "refines"                },
    { "traces-to",             "traced-from"            },
    { "traced-from",           "traces-to"              },
    { "part-of",               "contains"               },
    { "contains",              "part-of"                },
    { "satisfies",             "satisfied-by"           },
    { "satisfied-by",          "satisfies"              },
    { "tests",                 "tested-by"              },
    { "tested-by",             "tests"                  },
};

static constexpr std::size_t k_num_relation_pairs =
    sizeof(k_relation_pairs) / sizeof(k_relation_pairs[0]);

/** Return the inverse relation name, or nullptr if rel is unknown. */
static const char *lookup_inverse(const std::string &predicate) noexcept
{
    for (std::size_t i = 0; i < k_num_relation_pairs; ++i) {
        if (predicate == k_relation_pairs[i].forward)
            return k_relation_pairs[i].inverse;
    }
    return nullptr;
}

/* -------------------------------------------------------------------------
 * Public mutation API
 * ---------------------------------------------------------------------- */

TripleId TripletStore::add(const std::string &subject,
                           const std::string &predicate,
                           const std::string &object)
{
    /* Reject exact duplicates. */
    auto it = by_subject_.find(subject);
    if (it != by_subject_.end()) {
        for (TripleId existing_id : it->second) {
            const auto &slot = triples_[existing_id];
            if (slot && slot->predicate == predicate && slot->object == object) {
                return INVALID_TRIPLE_ID;
            }
        }
    }

    TripleId new_id = triples_.size();
    triples_.push_back(Triple{new_id, subject, predicate, object, /*inferred=*/false});

    by_subject_[subject].push_back(new_id);
    by_object_[object].push_back(new_id);
    by_predicate_[predicate].push_back(new_id);
    ++count_;

    return new_id;
}

bool TripletStore::remove(TripleId id)
{
    if (id >= triples_.size() || !triples_[id]) {
        return false;
    }

    const Triple &t = *triples_[id];
    index_remove(by_subject_,   t.subject,   id);
    index_remove(by_object_,    t.object,    id);
    index_remove(by_predicate_, t.predicate, id);

    triples_[id] = std::nullopt;
    --count_;
    ++dead_count_;
    maybe_compact();
    return true;
}

std::size_t TripletStore::remove_by_subject(const std::string &subject)
{
    auto it = by_subject_.find(subject);
    if (it == by_subject_.end()) return 0;

    /* Copy IDs before erasing to avoid iterator invalidation. */
    std::vector<TripleId> ids = it->second;
    std::size_t removed = 0;

    for (TripleId id : ids) {
        if (!triples_[id]) continue;
        const Triple &t = *triples_[id];
        index_remove(by_object_,    t.object,    id);
        index_remove(by_predicate_, t.predicate, id);
        triples_[id] = std::nullopt;
        --count_;
        ++dead_count_;
        ++removed;
    }
    by_subject_.erase(it);
    maybe_compact();
    return removed;
}

std::size_t TripletStore::remove_by_object(const std::string &object)
{
    auto it = by_object_.find(object);
    if (it == by_object_.end()) return 0;

    std::vector<TripleId> ids = it->second;
    std::size_t removed = 0;

    for (TripleId id : ids) {
        if (!triples_[id]) continue;
        const Triple &t = *triples_[id];
        index_remove(by_subject_,   t.subject,   id);
        index_remove(by_predicate_, t.predicate, id);
        triples_[id] = std::nullopt;
        --count_;
        ++dead_count_;
        ++removed;
    }
    by_object_.erase(it);
    maybe_compact();
    return removed;
}

std::size_t TripletStore::remove_by_predicate(const std::string &predicate)
{
    auto it = by_predicate_.find(predicate);
    if (it == by_predicate_.end()) return 0;

    std::vector<TripleId> ids = it->second;
    std::size_t removed = 0;

    for (TripleId id : ids) {
        if (!triples_[id]) continue;
        const Triple &t = *triples_[id];
        index_remove(by_subject_, t.subject, id);
        index_remove(by_object_,  t.object,  id);
        triples_[id] = std::nullopt;
        --count_;
        ++dead_count_;
        ++removed;
    }
    by_predicate_.erase(it);
    maybe_compact();
    return removed;
}

void TripletStore::clear() noexcept
{
    triples_.clear();
    by_subject_.clear();
    by_object_.clear();
    by_predicate_.clear();
    count_ = 0;
    dead_count_ = 0;
}

std::size_t TripletStore::compact()
{
    if (dead_count_ == 0) return 0;

    /* Build a remapping table: old slot index -> new consecutive index. */
    std::vector<TripleId> remap(triples_.size(), INVALID_TRIPLE_ID);

    std::vector<std::optional<Triple>> new_triples;
    new_triples.reserve(count_);

    TripleId new_id = 0;
    for (std::size_t old_id = 0; old_id < triples_.size(); ++old_id) {
        if (triples_[old_id]) {
            remap[old_id] = new_id;
            Triple t      = std::move(*triples_[old_id]);
            t.id          = new_id;
            new_triples.push_back(std::move(t));
            ++new_id;
        }
    }

    triples_ = std::move(new_triples);

    /* Remap every ID stored in the three index maps. */
    auto remap_vec = [&](std::vector<TripleId> &vec) {
        for (TripleId &id : vec) id = remap[id];
    };

    for (auto &[key, vec] : by_subject_)   remap_vec(vec);
    for (auto &[key, vec] : by_object_)    remap_vec(vec);
    for (auto &[key, vec] : by_predicate_) remap_vec(vec);

    std::size_t reclaimed = dead_count_;
    dead_count_ = 0;
    return reclaimed;
}

void TripletStore::maybe_compact()
{
    if (dead_count_ >= k_compact_threshold && dead_count_ >= count_)
        compact();
}

TripleId TripletStore::add_inferred(const std::string &subject,
                                    const std::string &predicate,
                                    const std::string &object)
{
    /* Reject if an identical triple already exists (any origin). */
    auto it = by_subject_.find(subject);
    if (it != by_subject_.end()) {
        for (TripleId existing_id : it->second) {
            const auto &slot = triples_[existing_id];
            if (slot && slot->predicate == predicate && slot->object == object)
                return INVALID_TRIPLE_ID;
        }
    }

    TripleId new_id = triples_.size();
    triples_.push_back(Triple{new_id, subject, predicate, object, /*inferred=*/true});

    by_subject_[subject].push_back(new_id);
    by_object_[object].push_back(new_id);
    by_predicate_[predicate].push_back(new_id);
    ++count_;

    return new_id;
}

std::size_t TripletStore::infer_inverses()
{
    /* Snapshot current user-declared triples before adding inferred ones,
     * to avoid iterating over triples we are about to add. */
    std::vector<Triple> declared;
    declared.reserve(count_);
    for (const auto &slot : triples_) {
        if (slot && !slot->inferred)
            declared.push_back(*slot);
    }

    std::size_t added = 0;
    for (const auto &t : declared) {
        const char *inv = lookup_inverse(t.predicate);
        if (!inv) continue; /* Unknown relation — Option C silent fallback. */

        TripleId id = add_inferred(t.object, inv, t.subject);
        if (id != INVALID_TRIPLE_ID) ++added;
    }
    return added;
}

/* -------------------------------------------------------------------------
 * Public query API
 * ---------------------------------------------------------------------- */

const Triple *TripletStore::get(TripleId id) const noexcept
{
    if (id >= triples_.size() || !triples_[id]) return nullptr;
    return &(*triples_[id]);
}

std::vector<const Triple *>
TripletStore::find_by_subject(const std::string &subject) const
{
    std::vector<const Triple *> result;
    auto it = by_subject_.find(subject);
    if (it == by_subject_.end()) return result;
    result.reserve(it->second.size());
    for (TripleId id : it->second) {
        if (triples_[id]) result.push_back(&(*triples_[id]));
    }
    return result;
}

std::vector<const Triple *>
TripletStore::find_by_object(const std::string &object) const
{
    std::vector<const Triple *> result;
    auto it = by_object_.find(object);
    if (it == by_object_.end()) return result;
    result.reserve(it->second.size());
    for (TripleId id : it->second) {
        if (triples_[id]) result.push_back(&(*triples_[id]));
    }
    return result;
}

std::vector<const Triple *>
TripletStore::find_by_predicate(const std::string &predicate) const
{
    std::vector<const Triple *> result;
    auto it = by_predicate_.find(predicate);
    if (it == by_predicate_.end()) return result;
    result.reserve(it->second.size());
    for (TripleId id : it->second) {
        if (triples_[id]) result.push_back(&(*triples_[id]));
    }
    return result;
}

std::vector<const Triple *> TripletStore::find_all() const
{
    std::vector<const Triple *> result;
    result.reserve(count_);
    for (const auto &slot : triples_) {
        if (slot) result.push_back(&(*slot));
    }
    return result;
}

/* -------------------------------------------------------------------------
 * Private helpers
 * ---------------------------------------------------------------------- */

void TripletStore::index_remove(
    std::unordered_map<std::string, std::vector<TripleId>> &idx,
    const std::string &key,
    TripleId id)
{
    auto it = idx.find(key);
    if (it == idx.end()) return;

    auto &vec = it->second;
    vec.erase(std::remove(vec.begin(), vec.end(), id), vec.end());
    if (vec.empty()) idx.erase(it);
}

} // namespace vibe
