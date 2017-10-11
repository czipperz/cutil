/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 Chris Gregory czipperz@gmail.com
 */

#include "../stack_trace.h"
#include <stdio.h>

struct stack_trace_item {
    const char* file;
    int line;
    const char* func;
};
typedef struct stack_trace_item stack_trace_item;

#define stack_trace_max 100
static stack_trace_item stack_trace_storage[stack_trace_max];
static int stack_trace_iter;

void stack_trace_clear(void) {
    stack_trace_iter = 0;
}

void stack_trace_register(const char* file, int line,
                          const char* func) {
    if (stack_trace_iter < stack_trace_max) {
        stack_trace_item* item =
            &stack_trace_storage[stack_trace_iter];
        item->file = file;
        item->line = line;
        item->func = func;
    }
    ++stack_trace_iter;
}

void stack_trace_print(void) {
    fprintf(stderr, "STACK TRACE:\n");
    int i;
    for (i = 0; i != stack_trace_iter && i != stack_trace_max; ++i) {
        fprintf(stderr, "%s:%d: In function %s\n",
                stack_trace_storage[i].file,
                stack_trace_storage[i].line,
                stack_trace_storage[i].func);
    }
    fprintf(stderr, "STACK TRACE COMPLETE.\n");
}
