/**
 * @file cxx23_check.cpp
 * @brief Compile-time verification that the toolchain supports C++23.
 *
 * This file is compiled by the `check-compiler` Makefile target.
 * A compilation failure here means the compiler or the -std flag is not
 * set to at least C++23.
 *
 * Note: GCC 13 reports __cplusplus = 202100L for -std=c++23 (the TS/draft
 * value). GCC 14 and Clang 17 report the final value 202302L.  We accept
 * anything greater than the C++20 sentinel (202002L) as a valid C++23
 * compiler.
 */

/* 1. Standard version macro ------------------------------------------------ */
#if __cplusplus <= 202002L
#  error "C++23 or later is required (compile with -std=c++23 or newer)"
#endif

/* 2. if consteval (C++23) -------------------------------------------------- */
static constexpr int probe_if_consteval()
{
    if consteval { return 1; }
    return 2;
}
static_assert(probe_if_consteval() == 1,
              "if consteval must evaluate in a constexpr context");

/* 3. static operator() (C++23) --------------------------------------------- */
struct StaticCallable {
    static constexpr int operator()(int x) noexcept { return x; }
};
static_assert(StaticCallable{}(42) == 42,
              "static operator() must be callable");

/* 4. auto(x) decay-copy (C++23) -------------------------------------------- */
static void probe_auto_decay_copy(int x)
{
    [[maybe_unused]] auto y = auto(x);
}

/* 5. Multidimensional subscript operator (C++23) ---------------------------- */
struct Grid {
    int data[4] = {1, 2, 3, 4};
    constexpr int operator[](int r, int c) const noexcept
    {
        return data[r * 2 + c];
    }
};

int main()
{
    /* Verify decay-copy probe compiles and runs. */
    probe_auto_decay_copy(1);

    /* Verify multidimensional subscript at runtime.
     *
     * In C++23 a defined operator[](int r, int c) takes precedence over the
     * comma operator inside [].  g[0, 1] therefore calls operator[](0, 1),
     * which returns data[0*2+1] = data[1] = 2.
     */
    constexpr Grid g{};
    static_assert(g[0, 1] == 2, "multidimensional subscript operator must work");

    return 0;
}
