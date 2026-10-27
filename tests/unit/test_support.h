/*
 * Minimal host-test assertion/runner support, plain C99, no third-party
 * framework (none is pinned in this repo yet; introducing one is an
 * open decision, not made unilaterally here).
 *
 * Header-only by design: as long as a single tests/unit source file
 * includes it, the counters below have a single definition. If a
 * second test file is added later with its own main(), move these
 * counters into a shared .c file and declare them extern here instead.
 */
#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include <stdio.h>

static unsigned int g_test_pass_count = 0U;
static unsigned int g_test_fail_count = 0U;

#define TEST_CHECK(cond, msg) \
    do { \
        if (cond) { \
            g_test_pass_count++; \
        } else { \
            g_test_fail_count++; \
            (void)printf("  FAIL: %s (%s:%d)\n", (msg), __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_RUN(fn) \
    do { \
        unsigned int fail_before = g_test_fail_count; \
        (void)printf("RUN  %s\n", #fn); \
        fn(); \
        if (g_test_fail_count == fail_before) { \
            (void)printf("PASS %s\n", #fn); \
        } \
    } while (0)

#endif /* TEST_SUPPORT_H */
