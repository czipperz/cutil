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
