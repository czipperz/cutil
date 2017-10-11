/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 Chris Gregory czipperz@gmail.com
 */

#include "test.h"

#include <stdio.h>
#include "rpmalloc.h"

int failures = 0;
int successes = 0;
int successes_assert = 0;

#define run(test)                                                    \
    do {                                                             \
        void test();                                                 \
        test();                                                      \
    } while (0)

int main() {
    rpmalloc_initialize();
    run(test_vec);
    run(test_str);
    printf("%d of %d succeeded.\n", successes, failures + successes);
    printf("%d assertions succeeded.\n", successes_assert);
    rpmalloc_finalize();
    return failures;
}
