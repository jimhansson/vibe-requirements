/**
 * @file validate.cpp
 * @brief Implementation of the validate command.
 */

#include "validate.h"

#include <cstdio>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "string_utils.h"

static int finish_validation(int problems)
{
    if (problems == 0)
        printf("validate: OK — no problems found.\n");
    else
        fprintf(stderr, "validate: %d problem(s) found.\n", problems);

    return problems;
}

static bool value_allowed(const std::string &value,
                          const VibeVocabulary &vocab)
{
    for (int i = 0; i < vocab.value_count; i++) {
        if (str_eq_ci(value.c_str(), vocab.values[i]))
            return true;
    }
    return false;
}

static std::string format_allowed_values(const VibeVocabulary &vocab)
{
    std::string out = "[";
    for (int i = 0; i < vocab.value_count; i++) {
        if (i > 0)
            out += ", ";
        out += vocab.values[i];
    }
    out += "]";
    return out;
}

static const std::string *lookup_field_value(const Entity &entity,
                                             const char *field)
{
    if (!field)
        return nullptr;
    if (str_eq_ci(field, "id"))
        return &entity.identity.id;
    if (str_eq_ci(field, "title"))
        return &entity.identity.title;
    if (str_eq_ci(field, "type"))
        return &entity.identity.type_raw;
    if (str_eq_ci(field, "status"))
        return &entity.lifecycle.status;
    if (str_eq_ci(field, "priority"))
        return &entity.lifecycle.priority;
    if (str_eq_ci(field, "owner"))
        return &entity.lifecycle.owner;
    if (str_eq_ci(field, "version"))
        return &entity.lifecycle.version;
    return nullptr;
}

int cmd_validate(const EntityList *elist,
                 const vibe::TripletStore *store,
                 const VibeConfig *cfg)
{
    return cmd_validate_with_options(elist, store, 0, cfg);
}

int cmd_validate_with_options(const EntityList *elist,
                              const vibe::TripletStore *store,
                              int fail_fast,
                              const VibeConfig *cfg)
{
    int problems = 0;

    /* ------------------------------------------------------------------
     * Check 1: duplicate IDs
     * ------------------------------------------------------------------ */
    std::unordered_map<std::string, std::vector<std::string>> id_to_files;
    for (const auto &e : *elist)
        id_to_files[e.identity.id].push_back(e.identity.file_path);

    for (const auto &[id, files] : id_to_files) {
        if (files.size() > 1) {
            fprintf(stderr, "error: duplicate id '%s' found in:\n",
                    id.c_str());
            for (const auto &fp : files)
                fprintf(stderr, "  %s\n", fp.c_str());
            ++problems;
            if (fail_fast)
                return finish_validation(problems);
        }
    }

    /* ------------------------------------------------------------------
     * Check 2: links to non-existent entities
     * ------------------------------------------------------------------ */
    std::unordered_set<std::string> known_ids;
    for (const auto &e : *elist)
        known_ids.insert(e.identity.id);

    for (const auto *triple : store->find_all()) {
        if (triple->inferred)
            continue; /* only validate user-declared links */

        const std::string &obj = triple->object;

        /* Skip objects that look like file paths or URLs */
        if (obj.find('/') != std::string::npos  ||
            obj.find('\\') != std::string::npos ||
            obj.find("://") != std::string::npos)
            continue;

        if (known_ids.find(obj) == known_ids.end()) {
            fprintf(stderr,
                    "error: '%s' links to unknown entity '%s' (relation: %s)\n",
                    triple->subject.c_str(),
                    obj.c_str(),
                    triple->predicate.c_str());
            ++problems;
            if (fail_fast)
                return finish_validation(problems);
        }
    }

    /* ------------------------------------------------------------------
     * Check 3: field vocabulary constraints (opt-in via config)
     * ------------------------------------------------------------------ */
    if (cfg && cfg->vocabulary_count > 0) {
        for (const auto &e : *elist) {
            for (int i = 0; i < cfg->vocabulary_count; i++) {
                const VibeVocabulary &vocab = cfg->vocabulary[i];
                if (vocab.field[0] == '\0')
                    continue;
                const std::string *value = lookup_field_value(e, vocab.field);
                if (!value || value->empty())
                    continue;
                if (!value_allowed(*value, vocab)) {
                    std::string allowed = format_allowed_values(vocab);
                    fprintf(stderr,
                            "error: '%s' (%s) has invalid %s value '%s' "
                            "(allowed: %s)\n",
                            e.identity.id.c_str(),
                            e.identity.file_path.c_str(),
                            vocab.field,
                            value->c_str(),
                            allowed.c_str());
                    ++problems;
                    if (fail_fast)
                        return finish_validation(problems);
                }
            }
        }
    }

    return finish_validation(problems);
}
