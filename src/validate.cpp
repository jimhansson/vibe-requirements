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

int cmd_validate(const EntityList *elist, const vibe::TripletStore *store)
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
        }
    }

    if (problems == 0)
        printf("validate: OK — no problems found.\n");
    else
        fprintf(stderr, "validate: %d problem(s) found.\n", problems);

    return problems;
}
