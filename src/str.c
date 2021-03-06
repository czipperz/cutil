/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 Chris Gregory czipperz@gmail.com
 */

#include "str.h"
#include <assert.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "rpmalloc.h"

/*! \brief The representation of a \c str that has allocated its
 *  contents.
 *
 *  It's not really safe to use this structure if you don't know what
 *  you're doing, so it is hidden from the user. */
struct str_alloc {
    /*! \brief The beginning of the string's contents. */
    char* str;
    /*! \brief Number of bytes used by the string.  This is NOT the
     *  number of characters. */
    size_t blen;
    /*! \brief Number of bytes that can be used by the string before
     *  it needs to reallocate its memory.
     *
     * To get the capacity use
\code{.c}
(str._cap >> 1)
\endcode
     */
    size_t _cap;
};
typedef struct str_alloc str_alloc;

static void
str_assert_(int cond, const char* condstr,
            const char* file, int line) {
    if (!cond) {
        printf("%s:%d: Assertion failed: %s", file, line, condstr);
        abort();
    }
}
#define str_assert(cond) (str_assert_(cond, #cond, __FILE__, __LINE__))

/*! \brief Test if \c self 's representation is stored in the \c _data
 *  member. */
static int
str_is_inline(const str* self) {
    return (((const char*)self)[sizeof(str) - 1] & 1) == 0;
}

static void
str_make_inline(str* self) {
    ((char*)self)[sizeof(str) - 1] &= ~1;
}

static void
str_make_alloc(str* self) {
    ((char*)self)[sizeof(str) - 1] |= 1;
}

static int
is_little_endian(void) {
    /* little endian has least significant byte first */
    int i = 1;
    return ((char*)&i)[0] == 1;
}

static size_t
str_alloc_cap(const str_alloc* self) {
    size_t cap;
    if (is_little_endian()) {
        /* On little endian, since shift right shifts left, we have to
         * do a roundabout method.  Move the byte with the flag to the
         * first byte then shift right. */
        char* _cap = (char*)&self->_cap;
        char* c = (char*)&cap;
        memcpy(c + 1, _cap, sizeof(size_t) - 1);
        c[0] = _cap[sizeof(size_t) - 1];
    } else {
        /* On big endian, shift right actually shifts right one. */
        cap = self->_cap;
    }
    return cap >> 1;
}

static void
str_alloc_set_cap(str_alloc* self, size_t cap) {
    cap <<= 1;
    cap |= 1;
    if (is_little_endian()) {
        /* Put first byte at the end */
        char* _cap = (char*)&self->_cap;
        memcpy(_cap, (char*)&cap + 1, sizeof(size_t) - 1);
        _cap[sizeof(size_t) - 1] = ((char*)&cap)[0];
    } else {
        /* Store directly */
        self->_cap = cap;
    }
}

static int
str_reserve_internal(str* self, size_t new_cap_bytes) {
    char* ptr;
    if (str_is_inline(self)) {
        if (new_cap_bytes < sizeof(str) - 1) {
            return 0;
        }
        if ((ptr = rpmalloc(new_cap_bytes * sizeof(char)))) {
            /* use bytes strlen */
            const size_t len = strlen(self->_data);
            memcpy(ptr, self, len + 1);
            ((str_alloc*)self)->blen = len;
        }
    } else if (str_alloc_cap((str_alloc*)self) < new_cap_bytes) {
        if (str_alloc_cap((str_alloc*)self) * 2 > new_cap_bytes) {
            /* _cap has a 1 bit at the end */
            new_cap_bytes = str_alloc_cap((str_alloc*)self) * 2;
        }
        ptr = rprealloc(((str_alloc*)self)->str,
                        new_cap_bytes * sizeof(char));
    } else {
        return 0;
    }
    if (!ptr) {
        return -1;
    }
    ((str_alloc*)self)->str = ptr;
    str_alloc_set_cap((str_alloc*)self, new_cap_bytes);
    return 0;
}

static int
_utf32_v(uint32_t character) {
    return g_unichar_validate(character);
}

static int
_utf8_v(const char* str, size_t len) {
    return g_utf8_validate(str, len, 0);
}

static size_t
_utf32_to_utf8(uint32_t elem, char* outbuf) {
    return g_unichar_to_utf8(elem, outbuf);
}

static size_t
_utf8_strlen_characters(const char* str, size_t len) {
    return g_utf8_strlen(str, len);
}

void
str_init(str* self) { memset(self, 0, sizeof(str)); }

void
str_destroy(str* self) {
    if (!str_is_inline(self)) {
        rpfree(((str_alloc*)self)->str);
    }
    str_init(self);
}

const char*
str_cbegin(const str* self) {
    if (str_is_inline(self)) {
        return self->_data;
    } else {
        return ((const str_alloc*)self)->str;
    }
}
char*
str_begin(str* self) {
    return (char*)str_cbegin(self);
}

const char*
str_cend(const str* self) {
    return str_cbegin(self) + str_len_bytes(self);
}
char*
str_end(str* self) {
    return (char*)str_cend(self);
}

size_t
str_len_bytes(const str* self) {
    if (str_is_inline(self)) {
        return strlen(self->_data);
    } else {
        return ((const str_alloc*)self)->blen;
    }
}

size_t
str_len_characters(const str* self) {
    return _utf8_strlen_characters(str_cbegin(self), str_len_bytes(self));
}

size_t
str_cap(const str* self) {
    if (str_is_inline(self)) {
        return sizeof(str) - 1;
    } else {
        return str_alloc_cap((const str_alloc*)self);
    }
}

int
str_reserve(str* self, size_t new_cap) {
    char* ptr;
    if (new_cap < str_cap(self)) {
        /* If the new_cap can be stored inline and we are inline,
         * immediately return */
        return 0;
    }
    if (str_is_inline(self)) {
        /* Inline -> Allocated */
        if ((ptr = rpmalloc((new_cap + 1) * sizeof(char)))) {
            /* use bytes strlen */
            const size_t len = strlen((const char*)self);
            memcpy(ptr, self, len + 1);
            ((str_alloc*)self)->blen = len;
        }
    } else {
        /* Expand allocated space */
        ptr = rprealloc(((str_alloc*)self)->str,
                        (new_cap + 1) * sizeof(char));
    }
    if (!ptr) {
        return -1;
    }
    ((str_alloc*)self)->str = ptr;
    str_alloc_set_cap((str_alloc*)self, new_cap);
    return 0;
}

int
str_shrink_to_size(str* self) {
    assert(self);

    if (str_is_inline(self)) {
        /* do nothing */
    } else if (((str_alloc*)self)->blen * sizeof(char) < sizeof(str) - 1) {
        /* Allocated -> Inline */
        char* ptr = ((str_alloc*)self)->str;
        str_make_inline(self);
        memcpy(self->_data, ptr, ((str_alloc*)self)->blen);
        rpfree(ptr);
    } else {
        /* reallocate */
        char* ptr;
        ptr = rprealloc(((str_alloc*)self)->str,
                        ((str_alloc*)self)->blen * sizeof(char));
        if (!ptr) {
            return -1;
        }
        ((str_alloc*)self)->str = ptr;
        str_alloc_set_cap((str_alloc*)self, ((str_alloc*)self)->blen);
    }
    return 0;
}

size_t
str_set_len_bytes(str* self, size_t len_bytes) {
    if (str_is_inline(self)) {
        self->_data[len_bytes] = 0;
    } else {
        ((str_alloc*)self)->str[len_bytes] = 0;
        ((str_alloc*)self)->blen = len_bytes;
    }
    return len_bytes;
}

int
str_push_sn(str* self, const char* string, size_t len_bytes) {
    size_t self_len;
    assert(self);
    assert(string);
    str_assert(_utf8_v(string, len_bytes));

    self_len = str_len_bytes(self);
    if (str_reserve_internal(self, self_len + len_bytes)) {
        return -1;
    }
    memcpy(str_begin(self) + self_len, string, len_bytes);
    str_set_len_bytes(self, self_len + len_bytes);
    return 0;
}
int
str_push_s(str* self, const char* string) {
    assert(string);

    /* use bytes strlen */
    return str_push_sn(self, string, strlen(string));
}
int
str_push_str(str* self, const str* string) {
    assert(string);
    return str_push_sn(self, str_cbegin(string), str_len_bytes(string));
}
int
str_push(str* self, uint32_t elem) {
    char outbuf[6];
    size_t size;
    str_assert(_utf32_v(elem));

    size = _utf32_to_utf8(elem, outbuf);
    return str_push_sn(self, outbuf, size);
}

int
str_insert_sn(str* self, const char* pos,
              const char* string, size_t len_bytes) {
    size_t self_len;
    assert(self);
    assert(pos);
    assert(str_cbegin(self) <= pos);
    assert(pos <= str_cend(self));
    str_assert(_utf8_v(string, len_bytes));

    self_len = str_len_bytes(self);
    if (str_reserve_internal(self, self_len + len_bytes)) {
        return -1;
    }
    /* removing const is safe because it is held as non-const by
     * self. */
    memmove((char*)pos + len_bytes, pos, self_len - (pos - str_cbegin(self)));
    memcpy((char*)pos, string, len_bytes);
    str_set_len_bytes(self, self_len + len_bytes);
    return 0;
}
int
str_insert_s(str* self, const char* pos, const char* string) {
    assert(string);
    return str_insert_sn(self, pos, string, strlen(string));
}
int
str_insert_str(str* self, const char* pos, const str* string) {
    assert(string);
    return str_insert_sn(self, pos, str_cbegin(string),
                         str_len_bytes(string));
}
int
str_insert(str* self, const char* pos, uint32_t elem) {
    char outbuf[6];
    int size;
    str_assert(_utf32_v(elem));

    size = _utf32_to_utf8(elem, outbuf);
    return str_insert_sn(self, pos, outbuf, size);
}

int
str_copy_n(str* self, const char* string, size_t len_bytes) {
    assert(self);
    assert(string);
    str_assert(_utf8_v(string, len_bytes));

    if (len_bytes <= sizeof(str_alloc) - 1) {
        str_destroy(self);
        memcpy(self->_data, string, len_bytes);
        self->_data[len_bytes] = 0;
        str_make_inline(self);
        return 0;
    } else {
        if (str_reserve(self, len_bytes)) {
            return -1;
        }
        memcpy(str_begin(self), string, len_bytes * sizeof(char));
        str_set_len_bytes(self, len_bytes);
        return 0;
    }
}
int
str_copy(str* self, const char* string) {
    assert(string);

    return str_copy_n(self, string, strlen(string));
}
int
str_copy_str(str* self, const str* string) {
    assert(self);
    assert(string);

    return str_copy_n(self, str_cbegin(string), str_len_bytes(string));
}

void
str_erase(str* self, size_t begin, size_t end) {
    str_erase_n_bytes(self, begin, end - begin);
}
void
str_erase_n_bytes(str* self, size_t begin, size_t num) {
    /* if (str_len_bytes(self) - num < sizeof(str)) { */
    /*     char* ptr = str_begin(self); */
    /*     size_t len = str_len_bytes(self); */
    /*     memmove(self->_data, ptr, begin); */
    /*     memmove(self->_data + begin, ptr + begin + num, */
    /*             len + 1 - begin - num); */
    /*     rpfree(ptr); */
    /* } else { */
    /*     char* ptr = str_begin(self); */
    /*     size_t len = str_len_bytes(self); */
    /*     memmove(ptr + begin, ptr + begin + num, */
    /*             len + 1 - begin - num); */
    /*     str_set_len_bytes(self, len - num); */
    /* } */
    char* ptr = str_begin(self);
    size_t len = str_len_bytes(self);
    memmove(ptr + begin, ptr + begin + num,
            len + 1 - begin - num);
    str_set_len_bytes(self, len - num);
    assert(ptr[len-num] == 0);
}

#ifdef TEST_MODE
#include "test.h"

TEST(test_str_begin) {
    str str = STR_INIT;
    ASSERT_ABORT(str_begin(&str) == str._data);
    str_copy(&str, "HELLO");
    ASSERT(str_is_inline(&str), cleanup);
    ASSERT(strcmp(str._data, "HELLO") == 0, cleanup);
cleanup:
    str_destroy(&str);
}
END_TEST

TEST(test_str_reserve_and_push) {
    str s = STR_INIT;
    const char* b;

    if (str_reserve(&s, 5)) { goto cleanup; }
    ASSERT(str_cbegin(&s), cleanup);
    ASSERT(str_len_bytes(&s) == 0, cleanup);
    ASSERT(str_cap(&s) >= 5, cleanup);
    ASSERT(str_cbegin(&s)[0] == '\0', cleanup);
    b = str_cbegin(&s);

    ASSERT(strcmp(str_cbegin(&s), "") == 0, cleanup);
    ASSERT(!str_push(&s, 'a'), cleanup);
    ASSERT(strcmp(str_cbegin(&s), "a") == 0, cleanup);
    ASSERT(!str_push(&s, 'b'), cleanup);
    ASSERT(strcmp(str_cbegin(&s), "ab") == 0, cleanup);
    ASSERT(!str_push(&s, 'c'), cleanup);
    ASSERT(strcmp(str_cbegin(&s), "abc") == 0, cleanup);
    ASSERT(!str_push(&s, 'd'), cleanup);
    ASSERT(strcmp(str_cbegin(&s), "abcd") == 0, cleanup);
    ASSERT(!str_push(&s, 'e'), cleanup);
    LAZY_ASSERT(str_cbegin(&s) == b);
    LAZY_ASSERT(str_len_bytes(&s) == 5);
    LAZY_ASSERT(str_cap(&s) >= 5);
    LAZY_ASSERT(str_cbegin(&s)[5] == '\0');
    LAZY_CONCLUDE(cleanup);
    ASSERT(strcmp(str_cbegin(&s), "abcde") == 0, cleanup);

    if (str_push(&s, 'f')) {
        ASSERT(str_cbegin(&s) == b, cleanup);
        ASSERT(str_len_bytes(&s) == 5, cleanup);
        ASSERT(str_cap(&s) >= 5, cleanup);

        goto cleanup;
    }
    ASSERT(strcmp(str_cbegin(&s), "abcdef") == 0, cleanup);
    ASSERT(str_len_bytes(&s) == 6, cleanup);
    ASSERT(str_cap(&s) >= 6, cleanup);

cleanup:
    str_destroy(&s);
}
END_TEST

TEST(test_str_shrink_to_size) {
    str s = STR_INIT;

    if (str_reserve(&s, 5)) { goto cleanup; }
    ASSERT(!str_push(&s, 'a'), cleanup);

    if (str_shrink_to_size(&s)) {
        goto cleanup;
    }
    LAZY_ASSERT(str_len_bytes(&s) == 1);
    LAZY_ASSERT(str_cap(&s) >= 1);
    LAZY_CONCLUDE(cleanup);
    ASSERT(strcmp(str_cbegin(&s), "a") == 0, cleanup);

cleanup:
    str_destroy(&s);
}
END_TEST

TEST(test_str_set_len) {
    str s = STR_INIT;
    const char* begin;

    if (str_reserve(&s, 3)) { goto cleanup; }
    LAZY_ASSERT(str_cbegin(&s));
    begin = str_cbegin(&s);
    LAZY_ASSERT(str_cap(&s) >= 3);
    LAZY_CONCLUDE(cleanup);
    memset(str_begin(&s), 'a', 4 * sizeof(char));

    str_set_len_bytes(&s, 3);
    LAZY_ASSERT(str_cbegin(&s));
    LAZY_ASSERT(str_cbegin(&s) == begin);
    LAZY_ASSERT(str_len_bytes(&s) == 3);
    LAZY_ASSERT(str_cap(&s) >= 3);
    LAZY_CONCLUDE(cleanup);
    LAZY_ASSERT(str_cbegin(&s)[3] == '\0');
    LAZY_ASSERT(strcmp(str_cbegin(&s), "aaa") == 0);
    LAZY_CONCLUDE(cleanup);

    str_set_len_bytes(&s, 1);
    LAZY_ASSERT(str_cbegin(&s));
    LAZY_ASSERT(str_cbegin(&s) == begin);
    LAZY_ASSERT(str_len_bytes(&s) == 1);
    LAZY_ASSERT(str_cap(&s) >= 3);
    LAZY_CONCLUDE(cleanup);
    LAZY_ASSERT(str_cbegin(&s)[1] == '\0');
    LAZY_ASSERT(strcmp(str_cbegin(&s), "a") == 0);
    LAZY_CONCLUDE(cleanup);

cleanup:
    str_destroy(&s);
}
END_TEST

TEST(test_str_copy_1) {
    str s = STR_INIT;
    if (str_copy(&s, "HI")) {
        goto cleanup;
    }
    LAZY_ASSERT(str_cbegin(&s));
    LAZY_ASSERT(str_len_bytes(&s) == 2);
    LAZY_ASSERT(str_cap(&s) >= 2);
    LAZY_CONCLUDE(cleanup1);
    ASSERT(strcmp(str_cbegin(&s), "HI") == 0, cleanup);
cleanup:
    str_destroy(&s);
}
END_TEST

TEST(test_str_copy_2) {
    str s = STR_INIT;
    if (str_copy(&s, "Hi my name is czipperz and I like to write "
                        "really long strings.")) {
        goto cleanup;
    }
    LAZY_ASSERT(str_cbegin(&s));
    LAZY_ASSERT(str_len_bytes(&s) == 63);
    LAZY_ASSERT(str_cap(&s) >= 63);
    LAZY_CONCLUDE(cleanup);
    ASSERT(strcmp(str_cbegin(&s), "Hi my name is czipperz and I "
                                    "like to write really long "
                                    "strings.") == 0,
            cleanup);
cleanup:
    str_destroy(&s);
}
END_TEST

TEST(test_str_erase_n_bytes) {
    str s = STR_INIT;
    if (str_copy(&s, "HI")) {
        goto cleanup;
    }
    str_erase_n_bytes(&s, 1, 1);
    LAZY_ASSERT(strcmp(str_cbegin(&s), "H") == 0);
    LAZY_ASSERT(str_len_bytes(&s) == 1);
    LAZY_ASSERT(str_cap(&s) >= 1);
    LAZY_CONCLUDE(cleanup);
cleanup:
    str_destroy(&s);
}
END_TEST

TEST(test_str_insert) {
    str s = STR_INIT;
    if (str_reserve(&s, 5)) {
        goto cleanup;
    }
    ASSERT(!str_insert(&s, str_cbegin(&s), 'o'), cleanup);
    ASSERT(strcmp(str_cbegin(&s), "o") == 0, cleanup);
    ASSERT(!str_insert(&s, str_cbegin(&s), 'l'), cleanup);
    ASSERT(strcmp(str_cbegin(&s), "lo") == 0, cleanup);
    ASSERT(!str_insert(&s, str_cbegin(&s), 'l'), cleanup);
    ASSERT(strcmp(str_cbegin(&s), "llo") == 0, cleanup);
    ASSERT(!str_insert(&s, str_cbegin(&s), 'e'), cleanup);
    ASSERT(strcmp(str_cbegin(&s), "ello") == 0, cleanup);
    ASSERT(!str_insert(&s, str_cbegin(&s), 'h'), cleanup);

    LAZY_ASSERT(str_cbegin(&s));
    LAZY_ASSERT(str_len_bytes(&s) == 5);
    LAZY_ASSERT(str_cap(&s) >= 5);
    LAZY_CONCLUDE(cleanup);

    ASSERT(strcmp(str_cbegin(&s), "hello") == 0, cleanup);

cleanup:
    str_destroy(&s);
}
END_TEST

void test_str(void) {
    RUN(test_str_begin);
    RUN(test_str_reserve_and_push);
    RUN(test_str_shrink_to_size);
    RUN(test_str_set_len);
    RUN(test_str_copy_1);
    RUN(test_str_copy_2);
    RUN(test_str_erase_n_bytes);
    RUN(test_str_insert);
}
#endif
