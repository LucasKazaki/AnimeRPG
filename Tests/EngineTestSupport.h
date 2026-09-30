#pragma once

// Minimal self-registering test harness for the portable engine suites.
// Checks abort in every build configuration, independent of NDEBUG.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace Astral::Test {

struct TestCase {
    const char* name;
    void (*function)();
};

inline std::vector<TestCase>& Registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const char* name, void (*function)()) { Registry().push_back({name, function}); }
};

// Optional argument: run only tests whose name contains the given substring.
inline int RunAll(int argc, char** argv, const char* suiteName) {
#if defined(_MSC_VER)
    // A failed check must fail the run, not wait on the Debug CRT's abort dialog
    // until ctest's timeout.
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    const char* filter = argc > 1 ? argv[1] : nullptr;
    int ran = 0;
    for (const TestCase& test : Registry()) {
        if (filter && !std::strstr(test.name, filter)) continue;
        std::printf("[ RUN  ] %s\n", test.name);
        std::fflush(stdout);
        test.function();
        std::printf("[  OK  ] %s\n", test.name);
        ++ran;
    }
    std::printf("%s: %d test case(s) passed\n", suiteName, ran);
    return ran > 0 ? 0 : 1;
}

} // namespace Astral::Test

#define ASTRAL_TEST(name)                                                        \
    static void name();                                                          \
    static const ::Astral::Test::Registrar name##_registrar(#name, &name);       \
    static void name()

#define ASTRAL_CHECK(expression)                                                 \
    do {                                                                         \
        if (!(expression)) {                                                     \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                #expression);                                                    \
            std::abort();                                                        \
        }                                                                        \
    } while (0)

#define ASTRAL_CHECK_NEAR(actual, expected, tolerance)                           \
    do {                                                                         \
        const double astralActual = static_cast<double>(actual);                \
        const double astralExpected = static_cast<double>(expected);            \
        if (!(std::fabs(astralActual - astralExpected) <= (tolerance))) {        \
            std::fprintf(stderr, "%s:%d: CHECK_NEAR failed: %s = %.9g, expected %.9g (tol %.3g)\n", \
                __FILE__, __LINE__, #actual, astralActual, astralExpected,      \
                static_cast<double>(tolerance));                                 \
            std::abort();                                                        \
        }                                                                        \
    } while (0)

#define ASTRAL_TEST_MAIN(suiteName)                                              \
    int main(int argc, char** argv) { return ::Astral::Test::RunAll(argc, argv, suiteName); }
