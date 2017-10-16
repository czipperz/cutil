/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 Chris Gregory czipperz@gmail.com
 */

#include "../hashmap.h"
#include "../rpmalloc.h"
#include "../vec.h"
#include "../str.h"
#include <assert.h>
#include <string.h>

typedef struct elemvec elemvec;
struct elemvec {
    char* elems;
    size_t len;
    size_t cap;
};

struct hashmap {
    size_t (*hash)(const void*);
    size_t elems;
    elemvec* mods;
    size_t len;
};

hashmap*
hashmap_new(size_t (*hash)(const void*)) {
    hashmap* hashmap = rpmalloc(sizeof(struct hashmap));
    if (hashmap) {
        hashmap->len = 8;
        hashmap->mods = rpcalloc(hashmap->len, sizeof(elemvec));
        if (!hashmap->mods) {
            rpfree(hashmap);
            return 0;
        }
        hashmap->elems = 0;
        hashmap->hash = hash;
    }
    return hashmap;
}

static void hashmap_destroy_(hashmap* hashmap) {
    size_t i;
    for (i = 0; i != hashmap->len; ++i) {
        rpfree(hashmap->mods[i].elems);
    }
    rpfree(hashmap->mods);
}

void
hashmap_destroy(hashmap* hashmap) {
    hashmap_destroy_(hashmap);
    rpfree(hashmap);
}

size_t
hashmap_size(const hashmap* hashmap) {
    return hashmap->elems;
}

static size_t hashmap_bsearch(const hashmap* hashmap, size_t hash,
                              size_t mod, int* contains, size_t size) {
    size_t min = 0;
    size_t max = hashmap->mods[mod].len;
    *contains = 0;
    while (min < max) {
        size_t mid = (min + max) / 2;
        size_t h = hashmap->hash(&hashmap->mods[mod].elems[mid * size]);
        if (hash < h) {
            max = mid;
        } else if (hash == h) {
            *contains = 1;
            return mid;
        } else {
            min = mid + 1;
        }
    }
    assert(min == max);
    return min;
}

int
hashmap_contains(const hashmap* hashmap, const void* key, size_t key_size, size_t value_size) {
    size_t hash = hashmap->hash(key);
    size_t mod = hash % hashmap->len;
    int contains;
    hashmap_bsearch(hashmap, hash, mod, &contains, key_size + value_size);
    return contains;
}

static int hashmap_insert_no_resize(hashmap* hashmap,
                                    const void* key, size_t key_size,
                                    const void* value, size_t value_size) {
    size_t hash = hashmap->hash(key);
    size_t mod = hash % hashmap->len;
    int contains;
    size_t index = hashmap_bsearch(hashmap, hash, mod, &contains, key_size + value_size);
    if (contains) {
        return 1;
    } else {
        if (vec_make_space(&hashmap->mods[mod], key_size + value_size, index)) {
            return -1;
        } else {
            memcpy(&hashmap->mods[mod].elems[index * (key_size + value_size)],
                   key, key_size);
            memcpy(&hashmap->mods[mod].elems[index * (key_size + value_size) + key_size],
                   value, value_size);
            return 0;
        }
    }
}

static int hashmap_resize(hashmap* hashmap, size_t size, size_t new_len) {
    const size_t len = hashmap->len;
    elemvec* mods = hashmap->mods;
    size_t i;
    hashmap->mods = rpcalloc(new_len, sizeof(*hashmap->mods));
    if (!hashmap->mods) {
        hashmap->mods = mods;
        return -1;
    }

    hashmap->len = new_len;

    for (i = 0; i != len; ++i) {
        size_t j;
        for (j = 0; j != mods[i].len; ++j) {
            if (hashmap_insert_no_resize(hashmap, &mods[i].elems[j * size],
                                         size, /* this does not do anything to j */ mods, 0)) {
                hashmap_destroy_(hashmap);
                hashmap->mods = mods;
                hashmap->len = len;
                return -1;
            }
        }
    }

    for (i = 0; i != len; ++i) {
        rpfree(mods[i].elems);
    }
    rpfree(mods);

    return 0;
}

int
hashmap_reserve(hashmap* hashmap, size_t cap, size_t key_size, size_t value_size) {
    if (cap > hashmap->len * 2) {
        size_t new_len = hashmap->elems;
        size_t i;
        --new_len;
        for (i = 1; i < sizeof(size_t); ++i) {
            new_len |= new_len >> i;
        }
        ++new_len;
        new_len *= 2;
        return hashmap_resize(hashmap, key_size + value_size, new_len);
    } else {
        return 0;
    }
}

int
hashmap_insert(hashmap* hashmap, const void* key, size_t key_size,
               const void* value, size_t value_size) {
    int err;
    if (hashmap->elems >= hashmap->len * 2) {
        if (hashmap_resize(hashmap, key_size + value_size, hashmap->len * 2)) {
            return -1;
        }
    }
    if ((err = hashmap_insert_no_resize(hashmap, key, key_size, value, value_size))) {
        return err;
    } else {
        ++hashmap->elems;
        return 0;
    }
}

int
hashmap_erase(hashmap* hashmap, const void* key, size_t key_size, size_t value_size) {
    size_t hash = hashmap->hash(key);
    size_t mod = hash % hashmap->len;
    int contains;
    size_t index = hashmap_bsearch(hashmap, hash, mod, &contains, key_size + value_size);
    if (contains) {
        vec_remove(&hashmap->mods[mod], key_size + value_size, index);
        --hashmap->elems;
    }
    return !contains;
}

void
hashmap_iterate(hashmap* hashmap, size_t key_size, size_t value_size,
                void (*fun)(void*, void*, void*), void* userdata) {
    size_t mod;
    for (mod = 0; mod != hashmap->len; ++mod) {
        elemvec* vec = &hashmap->mods[mod];
        size_t i;
        for (i = 0; i != vec->len; ++i) {
            char* key = &vec->elems[i * (key_size + value_size)];
            fun(key, key + key_size, userdata);
        }
    }
}

hashmap_iterator
hashmap_iterator_new(hashmap* hashmap) {
    size_t mod;
    hashmap_iterator iterator;
    iterator._hashmap = hashmap;
    iterator._inner = 0;
    for (mod = 0; mod != hashmap->len && hashmap->mods[mod].len == 0; ++mod) {}
    iterator._outer = mod;
    return iterator;
}

hashmap_pair
hashmap_iterator_next(hashmap_iterator* iterator,
                      size_t key_size, size_t value_size) {
    hashmap_pair pair;
    if (iterator->_outer == iterator->_hashmap->len) {
        pair.key = 0;
        pair.value = 0;
    } else {
        ++iterator->_inner;
        if (iterator->_inner == iterator->_hashmap->mods[iterator->_outer].len) {
            iterator->_inner = 0;
            for (++iterator->_outer;
                 iterator->_outer != iterator->_hashmap->len;
                 ++iterator->_outer) {
                if (iterator->_hashmap->mods[iterator->_outer].len != 0) {
                    goto ret;
                }
            }
        }
ret:
        pair.key = &iterator->_hashmap->mods[iterator->_outer]
            .elems[iterator->_inner * (key_size + value_size)];
        pair.value = (char*)pair.key + key_size;
    }
    return pair;
}

hashmap_pair
hashmap_iterator_peek(const hashmap_iterator* iterator,
                      size_t key_size, size_t value_size) {
    hashmap_pair pair;
    if (iterator->_outer == iterator->_hashmap->len) {
        pair.key = 0;
        pair.value = 0;
    } else {
        pair.key = &iterator->_hashmap->mods[iterator->_outer]
            .elems[iterator->_inner * (key_size + value_size)];
        pair.value = (char*)pair.key + key_size;
    }
    return pair;
}

void*
hashmap_lookup(hashmap* hashmap, const void* key, size_t key_size, size_t value_size) {
    size_t hash = hashmap->hash(key);
    size_t mod = hash % hashmap->len;
    int contains;
    size_t index = hashmap_bsearch(hashmap, hash, mod, &contains, key_size + value_size);
    if (contains) {
        return &hashmap->mods[mod].elems[index * (key_size + value_size) + key_size];
    } else {
        return 0;
    }
}

size_t
size_t_hash(const void* v) {
    return *(const size_t*)v;
}

size_t
str_hash(const void* v) {
    const str* s = v;
    size_t total = 1212382;
    const char* i = str_cbegin(s);
    const char* e = str_cend(s);
    for (; i != e; ++i) {
        total *= 31;
        total += *i;
    }
    return total;
}

#ifdef TEST_MODE
#include "test.h"

TEST(test_hashmap_contains) {
    hashmap* hashmap = hashmap_new(size_t_hash);
    size_t num;
    ASSERT(hashmap, cleanup);
    num = 3;
    ASSERT(!hashmap_insert(hashmap, &num, sizeof(size_t), &num, sizeof(size_t)), cleanup);
    ASSERT(hashmap_contains(hashmap, &num, sizeof(size_t), sizeof(size_t)), cleanup);
    num = 8;
    ASSERT(!hashmap_contains(hashmap, &num, sizeof(size_t), sizeof(size_t)), cleanup);
cleanup:
    hashmap_destroy(hashmap);
}
END_TEST

TEST(test_hashmap_erase) {
    hashmap* hashmap = hashmap_new(size_t_hash);
    size_t num;
    ASSERT(hashmap, cleanup);
    num = 3;
    ASSERT(!hashmap_insert(hashmap, &num, sizeof(size_t), &num, sizeof(size_t)), cleanup);
    ASSERT(hashmap_contains(hashmap, &num, sizeof(size_t), sizeof(size_t)), cleanup);
    num = 8;
    ASSERT(!hashmap_contains(hashmap, &num, sizeof(size_t), sizeof(size_t)), cleanup);
    ASSERT(hashmap_erase(hashmap, &num, sizeof(size_t), sizeof(size_t)), cleanup);
    num = 3;
    ASSERT(!hashmap_erase(hashmap, &num, sizeof(size_t), sizeof(size_t)), cleanup);
cleanup:
    hashmap_destroy(hashmap);
}
END_TEST

TEST(test_hashmap_mass_addition) {
    hashmap* hashmap = hashmap_new(size_t_hash);
    size_t num;
    ASSERT(hashmap, cleanup);
    for (num = 0; num != 30; ++num) {
        size_t i;
        ASSERT(!hashmap_insert(hashmap, &num, sizeof(size_t), &num, sizeof(size_t)), cleanup);
        i = -1;
        ASSERT(!hashmap_contains(hashmap, &i, sizeof(size_t), sizeof(size_t)), cleanup);
        for (i = 0; i <= num; ++i) {
            ASSERT(hashmap_contains(hashmap, &i, sizeof(size_t), sizeof(size_t)), cleanup);
            ASSERT(i == *(size_t*)hashmap_lookup(hashmap, &i, sizeof(size_t), sizeof(size_t)), cleanup);
        }
        i = num + 1;
        ASSERT(!hashmap_contains(hashmap, &i, sizeof(size_t), sizeof(size_t)), cleanup);
        ASSERT(!hashmap_lookup(hashmap, &i, sizeof(size_t), sizeof(size_t)), cleanup);
    }
cleanup:
    hashmap_destroy(hashmap);
}
END_TEST

void test_hashmap(void) {
    RUN(test_hashmap_contains);
    RUN(test_hashmap_erase);
    RUN(test_hashmap_mass_addition);
}
#endif
