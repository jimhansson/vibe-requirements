/**
 * @file triplet_store.cpp
 * @brief Implementation of the relation triplet store.
 */

#include "triplet_store.hpp"

#include <algorithm>

namespace vibe {

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
    triples_.push_back(Triple{new_id, subject, predicate, object});

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
        ++removed;
    }
    by_subject_.erase(it);
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
        ++removed;
    }
    by_object_.erase(it);
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
        ++removed;
    }
    by_predicate_.erase(it);
    return removed;
}

void TripletStore::clear() noexcept
{
    triples_.clear();
    by_subject_.clear();
    by_object_.clear();
    by_predicate_.clear();
    count_ = 0;
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
