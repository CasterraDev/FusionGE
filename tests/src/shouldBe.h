#include <core/logger.h>
#include <math/fsnmath.h>

/**
 * @brief Expects expected to be equal to actual.
 */
#define should_be(expected, actual)                                                              \
    if (actual != expected) {                                                                           \
        FERROR("--> Expected %lld, but got: %lld. File: %s:%d.", expected, actual, __FILE__, __LINE__); \
        return false;                                                                                   \
    }

/**
 * @brief Expects expected to NOT be equal to actual.
 */
#define should_not_be(expected, actual)                                                                   \
    if (actual == expected) {                                                                                    \
        FERROR("--> Expected %d != %d, but they are equal. File: %s:%d.", expected, actual, __FILE__, __LINE__); \
        return false;                                                                                            \
    }

/**
 * @brief Expects expected to be actual given a tolerance of FLOAT_EPSILON.
 */
#define float_should_be(expected, actual)                                                        \
    if (fsnAbs(expected - actual) > 0.001f) {                                                         \
        FERROR("--> Expected %f, but got: %f. File: %s:%d.", expected, actual, __FILE__, __LINE__); \
        return false;                                                                               \
    }

/**
 * @brief Expects actual to be true.
 */
#define should_be_true(actual)                                                      \
    if (actual != true) {                                                              \
        FERROR("--> Expected true, but got: false. File: %s:%d.", __FILE__, __LINE__); \
        return false;                                                                  \
    }

/**
 * @brief Expects actual to be false.
 */
#define should_be_false(actual)                                                     \
    if (actual != false) {                                                             \
        FERROR("--> Expected false, but got: true. File: %s:%d.", __FILE__, __LINE__); \
        return false;                                                                  \
    }