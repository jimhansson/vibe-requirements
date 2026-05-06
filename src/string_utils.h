#ifndef VIBE_STRING_UTILS_H
#define VIBE_STRING_UTILS_H

#include <cctype>

static inline bool str_equal_ci(const char *a, const char *b)
{
    if (!a || !b)
        return false;
    for (; *a && *b; a++, b++) {
        if (std::tolower(static_cast<unsigned char>(*a)) !=
            std::tolower(static_cast<unsigned char>(*b)))
            return false;
    }
    return *a == '\0' && *b == '\0';
}

#endif /* VIBE_STRING_UTILS_H */
