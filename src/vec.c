/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 Chris Gregory czipperz@gmail.com
 */

#include "vec.h"

#include <assert.h>
#include <string.h>
#include "rpmalloc.h"

/*! \brief Generic vector implementation.  We use type erasure of the
 *  input and then casting to get all different vectors to look the
 *  same. */
struct vec {
    char* ptr;
    size_t len;
    size_t cap;
};

/*! \brief Reallocate the memory */
static int _realloc(struct vec* self, size_t size, size_t new_cap) {
    char* temp;
    assert(self);
    temp = rprealloc(self->ptr, size * new_cap);
    if (!temp) {
        return -1;
    }
    self->ptr = temp;
    self->cap = new_cap;
    return 0;
}

/*! \brief Reserve used for auto reserves. */
static int _reserve(struct vec* self, size_t size, size_t new_cap) {
    assert(self);
    if (new_cap > self->cap) {
        /* If the user has done a vec_reserve() to ensure a specific
         * length, only expand it if we need to.  Then we do a
         * doubling routine starting at minimum capacity of 16. */
        if (new_cap < 16) {
            new_cap = 16;
        }
        if (new_cap < self->cap * 2) {
            new_cap = self->cap * 2;
        }
        return _realloc(self, size, new_cap);
    }
    return 0;
}

int vec_reserve(void* s, size_t size, size_t new_cap) {
    struct vec* self = s;
    assert(self);
    if (new_cap > self->cap) {
        return _realloc(self, size, new_cap);
    }
    return 0;
}

int vec_insert(void* s, size_t size, size_t index, const void* elem) {
    struct vec* self = s;
    assert(self);
    assert(elem);
    assert(index <= self->len);
    if (_reserve(self, size, self->len + 1)) {
        return -1;
    }
    memmove(size * (index + 1) + self->ptr, size * index + self->ptr,
            size * (self->len - index));
    ++self->len;
    memcpy(size * index + self->ptr, elem, size);
    return 0;
}

int vec_push(void* s, size_t size, const void* elem) {
    struct vec* self = s;
    assert(self);
    return vec_insert(self, size, self->len, elem);
}

int vec_shrink_to_size(void* s, size_t size) {
    struct vec* self = s;
    assert(self);
    return _realloc(self, size, self->len);
}

void vec_remove(void* s, size_t size, size_t index) {
    struct vec* self = s;
    assert(self);
    assert(self->ptr);
    assert(index < self->len);
    --self->len;
    memmove(self->ptr + index, self->ptr + index + 1,
            size * (self->len - index));
    --self->len;
}

#if TEST_MODE
#include "test.h"

struct ivec {
    int* ptr;
    size_t len;
    size_t cap;
};

TEST(test_vec_push) {
    struct ivec v = VEC_INIT;
    int el;
    ASSERT(v.ptr == 0);
    ASSERT(v.len == 0);
    ASSERT(v.cap == 0);

    el = 1;
    if (vec_push(&v, sizeof(int), &el)) {
        ASSERT(v.ptr == 0);
        ASSERT(v.len == 0);
        ASSERT(v.cap == 0);
        return 0;
    }
    ASSERT(v.ptr);
    ASSERT(v.len == 1);
    ASSERT(v.cap >= 1);
    ASSERT(v.ptr[0] == 1);

    el = 2;
    if (vec_push(&v, sizeof(int), &el)) {
        ASSERT(v.ptr);
        ASSERT(v.len == 1);
        ASSERT(v.cap == 1);

        rpfree(v.ptr);
        return 0;
    }
    ASSERT(v.ptr);
    ASSERT(v.len == 2);
    ASSERT(v.cap >= 2);
    ASSERT(v.ptr[0] == 1);
    ASSERT(v.ptr[1] == 2);

    rpfree(v.ptr);
}
END_TEST

TEST(test_vec_reserve) {
    struct ivec v = VEC_INIT;
    int* b;

    if (vec_reserve(&v, sizeof(int), 3)) {
        return 0;
    }
    ASSERT(v.ptr);
    ASSERT(v.len == 0);
    ASSERT(v.cap == 3);
    b = v.ptr;
    ASSERT(v.ptr == b);

    {
        int el = 1;
        ASSERT(!vec_push(&v, sizeof(int), &el));
        ASSERT(v.ptr == b);
        ASSERT(v.len == 1);
        ASSERT(v.cap == 3);
        ASSERT(v.ptr[0] == 1);
    }
    {
        int el = 2000;
        ASSERT(!vec_push(&v, sizeof(int), &el));
        ASSERT(v.ptr == b);
        ASSERT(v.len == 2);
        ASSERT(v.cap == 3);
        ASSERT(v.ptr[0] == 1);
        ASSERT(v.ptr[1] == 2000);
    }
    {
        int el = 12303;
        ASSERT(!vec_push(&v, sizeof(int), &el));
        ASSERT(v.ptr == b);
        ASSERT(v.len == 3);
        ASSERT(v.cap == 3);
        ASSERT(v.ptr[0] == 1);
        ASSERT(v.ptr[1] == 2000);
        ASSERT(v.ptr[2] == 12303);
    }

    rpfree(v.ptr);
}
END_TEST

TEST(test_vec_shrink_to_size) {
    struct ivec v = VEC_INIT;
    int* b;
    int el;

    if (vec_reserve(&v, sizeof(int), 3)) {
        return 0;
    }
    ASSERT(v.ptr);
    ASSERT(v.len == 0);
    ASSERT(v.cap == 3);
    b = v.ptr;

    el = 13;
    ASSERT(!vec_push(&v, sizeof(int), &el));

    if (vec_shrink_to_size(&v, sizeof(int))) {
        ASSERT(v.ptr == b);
        ASSERT(v.len == 1);
        ASSERT(v.cap == 3);

        rpfree(v.ptr);
        return 0;
    }
    ASSERT(v.len == 1);
    ASSERT(v.cap == 1);

    rpfree(v.ptr);
}
END_TEST

TEST(test_vec_insert) {
    struct ivec v = VEC_INIT;
    int el;

    el = 20;
    if (vec_insert(&v, sizeof(int), 0, &el)) {
        ASSERT(v.ptr == 0);
        ASSERT(v.len == 0);
        ASSERT(v.cap == 0);
        return 0;
    }
    ASSERT(v.ptr);
    ASSERT(v.len == 1);
    ASSERT(v.cap >= 1);
    ASSERT(v.ptr[0] == 20);

    el = 13;
    if (vec_insert(&v, sizeof(int), 0, &el)) {
        ASSERT(v.ptr);
        ASSERT(v.len == 1);
        ASSERT(v.cap >= 1);

        rpfree(v.ptr);
        return 0;
    }
    ASSERT(v.ptr);
    ASSERT(v.len == 2);
    ASSERT(v.cap >= 2);
    ASSERT(v.ptr[0] == 13);
    ASSERT(v.ptr[1] == 20);

    rpfree(v.ptr);
}
END_TEST

void test_vec() {
    RUN(test_vec_push);
    RUN(test_vec_reserve);
    RUN(test_vec_shrink_to_size);
    RUN(test_vec_insert);
}
#endif
