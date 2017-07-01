/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 Chris Gregory czipperz@gmail.com
 */

#ifndef HEADER_GUARD_TEST_H
#define HEADER_GUARD_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TEST_MODE
#error "Must be in TEST_MODE to include test.h"
#endif

#include <stdio.h>
#include <stddef.h>

extern int failures;
extern int successes;
extern int successes_assert;

#define TEST(name)                                                   \
    static int name() {                                              \
        int _ret = 0;                                                \
        const char* const _name = #name;                             \
        const int _line = __LINE__;
#define END_TEST                                                     \
        return _ret;                                                 \
    }

#define PRINT_TEST_FAILED()                                          \
    do {                                                             \
        fprintf(stderr, "%s:%d: %s() failed:\n", __FILE__,           \
                _line, _name);                                       \
    } while (0)
#define PRINT_ASSERTION_FAILED(str)                                  \
    do {                                                             \
        fprintf(stderr, "%s:%d: Assertion failed %s\n",              \
                __FILE__, __LINE__, str);                            \
    } while (0)

#define CONCLUDE()                                                   \
    do {                                                             \
        if (_ret != 0) {                                             \
            return _ret;                                             \
        }                                                            \
    } while (0)

#define ASSERT_ABORT(cond)                                           \
    do {                                                             \
        if (cond) {                                                  \
            ++successes_assert;                                      \
        } else {                                                     \
            if (_ret == 0) {                                         \
                PRINT_TEST_FAILED();                                 \
            }                                                        \
            PRINT_ASSERTION_FAILED(#cond);                           \
            return 1;                                                \
        }                                                            \
    } while (0)

#define LAZY_CONCLUDE(label)                                         \
    do {                                                             \
        if (_ret != 0) {                                             \
            return _ret;                                             \
        }                                                            \
    } while (0)

#define LAZY_ASSERT(cond)                                            \
    do {                                                             \
        if (cond) {                                                  \
            ++successes_assert;                                      \
        } else {                                                     \
            if (_ret == 0) {                                         \
                PRINT_TEST_FAILED();                                 \
            }                                                        \
            PRINT_ASSERTION_FAILED(#cond);                           \
            _ret = 1;                                                \
        }                                                            \
    } while (0)

#define ASSERT(cond, label)                                          \
    do {                                                             \
        if (cond) {                                                  \
            ++successes_assert;                                      \
        } else {                                                     \
            if (_ret == 0) {                                         \
                PRINT_TEST_FAILED();                                 \
            }                                                        \
            PRINT_ASSERTION_FAILED(#cond);                           \
            _ret = 1;                                                \
            goto label;                                              \
        }                                                            \
    } while (0)

#define RUN(test)                                                    \
    do {                                                             \
        if (test()) {                                                \
            ++failures;                                              \
        } else {                                                     \
            ++successes;                                             \
        }                                                            \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif
