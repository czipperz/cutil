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

static void str_assert_(int cond, const char* condstr,
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
    return (((const str_alloc*)self)->_cap & 1) == 0;
}

static int
str_reserve_internal(str* self, size_t new_cap_bytes) {
    char* ptr;
    if (str_is_inline(self)) {
        if (new_cap_bytes < sizeof(str_alloc)) {
            return 0;
        }
        if ((ptr = rpmalloc(new_cap_bytes * sizeof(char)))) {
            /* use bytes strlen */
            const size_t len = strlen(self->_data);
            memcpy(ptr, self, len + 1);
            ((str_alloc*)self)->blen = len;
        }
        new_cap_bytes <<= 1;
    } else if ((((str_alloc*)self)->_cap >> 1) < new_cap_bytes) {
        if ((((str_alloc*)self)->_cap >> 1) * 2 > new_cap_bytes) {
            /* _cap has a 1 bit at the end */
            new_cap_bytes = ((((str_alloc*)self)->_cap >> 1) * 2);
        }
        ptr = rprealloc(((str_alloc*)self)->str,
                        new_cap_bytes * sizeof(char));
        new_cap_bytes <<= 1;
        new_cap_bytes |= 1;
    } else {
        return 0;
    }
    if (!ptr) {
        return -1;
    }
    ((str_alloc*)self)->str = ptr;
    ((str_alloc*)self)->_cap = new_cap_bytes;
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

static size_t _utf32_to_utf8(uint32_t elem, char* outbuf) {
    return g_unichar_to_utf8(elem, outbuf);
}

static size_t _utf8_strlen_characters(const char* str, size_t len) {
    return g_utf8_strlen(str, len);
}

static void
str_make_inline(str* self) {
    ((str_alloc*)self)->_cap &= ~(size_t)1;
}

static void
str_make_alloc(str* self) {
    ((str_alloc*)self)->_cap |= 1;
}

void
str_destroy(str* self) {
    if (!str_is_inline(self)) {
        rpfree(((str_alloc*)self)->str);
    }
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
        return sizeof(str_alloc) - 1;
    } else {
        return ((const str_alloc*)self)->_cap >> 1;
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
        if ((ptr = rpmalloc(new_cap * sizeof(char)))) {
            /* use bytes strlen */
            const size_t len = strlen((const char*)self);
            memcpy(ptr, self, len + 1);
            ((str_alloc*)self)->blen = len;
        }
    } else {
        /* Expand allocated space */
        ptr = rprealloc(((str_alloc*)self)->str,
                        new_cap * sizeof(char));
    }
    if (!ptr) {
        return -1;
    }
    ((str_alloc*)self)->str = ptr;
    ((str_alloc*)self)->_cap = new_cap << 1;
    str_make_alloc(self);
    return 0;
}

int
str_shrink_to_size(str* self) {
    assert(self);
    if (str_is_inline(self)) {
    } else if (((str_alloc*)self)->blen * sizeof(char) < sizeof(str) - 1) {
        /* Allocated -> Inline */
        char* ptr = ((str_alloc*)self)->str;
        str_make_inline(self);
        memcpy(self->_data, ptr, ((str_alloc*)self)->blen);
    } else {
        /* reallocate */
        char* ptr;
        ptr = rprealloc(((str_alloc*)self)->str,
                        ((str_alloc*)self)->blen * sizeof(char));
        if (!ptr) {
            return -1;
        }
        ((str_alloc*)self)->str = ptr;
        ((str_alloc*)self)->_cap = ((str_alloc*)self)->blen << 1;
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
    assert(self);
    assert(string);
    str_assert(_utf8_v(string, len_bytes));
    if (str_reserve_internal(self, str_len_bytes(self) + len_bytes)) {
        return -1;
    }
    memcpy(str_begin(self), string, len_bytes);
    str_set_len_bytes(self, str_len_bytes(self) + len_bytes);
    return 0;
}
int
str_push_s(str* self, const char* string) {
    assert(string);
    /* use bytes strlen */
    return str_push_sn(self, string, strlen(string));
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
    assert(self);
    assert(pos);
    assert(str_cbegin(self) < pos);
    assert(pos <= str_cend(self));
    str_assert(_utf8_v(string, len_bytes));
    if (str_reserve_internal(self, str_len_bytes(self) + len_bytes)) {
        return -1;
    }
    memmove((char*)pos + len_bytes, pos, len_bytes);
    memcpy((char*)pos, string, len_bytes);
    str_set_len_bytes(self, str_len_bytes(self) + len_bytes);
    return 0;
}
int
str_insert_s(str* self, const char* pos, const char* string) {
    assert(string);
    return str_insert_sn(self, pos, string, strlen(string));
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

#if TEST_MODE
#include "test.h"

TEST(_str_begin) {
    str str = STR_INIT;
    ASSERT(str_begin(&str) == str._data);
    str_copy(&str, "HELLO");
    ASSERT(str_is_inline(&str));
    LAZY_ASSERT(strcmp(str._data, "HELLO") == 0);
    str_destroy(&str);
}
END_TEST

void test_str() {
    RUN(_str_begin);
}
#endif
