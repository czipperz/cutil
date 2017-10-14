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

hashmap* hashmap_new(size_t (*hash)(const void*)) {
    hashmap* hashmap = rpmalloc(sizeof(hashmap));
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

void hashmap_destroy(hashmap* hashmap) {
    size_t i;
    for (i = 0; i != hashmap->len; ++i) {
        rpfree(hashmap->mods[i].elems);
    }
    rpfree(hashmap->mods);
    rpfree(hashmap);
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

int hashmap_contains(const hashmap* hashmap, const void* element, size_t size) {
    size_t hash = hashmap->hash(element);
    size_t mod = hash % hashmap->len;
    int contains;
    hashmap_bsearch(hashmap, hash, mod, &contains, size);
    return contains;
}

static int hashmap_insert_no_resize(hashmap* hashmap, const void* element, size_t size) {
    size_t hash = hashmap->hash(element);
    size_t mod = hash % hashmap->len;
    int contains;
    size_t index = hashmap_bsearch(hashmap, hash, mod, &contains, size);
    if (contains) {
        return 1;
    } else {
        return vec_insert(&hashmap->mods[mod], size, index, element);
    }
}

static int hashmap_resize(hashmap* hashmap, size_t size) {
    const size_t len = hashmap->len;
    elemvec* mods = rpcalloc(len * 2, sizeof(*hashmap->mods));
    size_t i;
    if (!mods) {
        return 0;
    }
    {
        void* modsb = mods;
        mods = hashmap->mods;
        hashmap->mods = modsb;
        hashmap->len *= 2;
    }

    for (i = 0; i != len; ++i) {
        size_t j;
        for (j = 0; j != mods[i].len; ++j) {
            if (hashmap_insert_no_resize(hashmap, &mods[i].elems[j * size], size)) {
                hashmap_destroy(hashmap);
                hashmap->mods = mods;
                hashmap->len = len;
                return -1;
            }
        }
    }

    for (i = 0; i != hashmap->len; ++i) {
        rpfree(mods[i].elems);
    }
    rpfree(mods);

    return 0;
}

static size_t elems_per_len = 2;

int hashmap_insert(hashmap* hashmap, const void* element, size_t size) {
    int err;
    if (hashmap->elems >= elems_per_len * hashmap->len) {
        if (hashmap_resize(hashmap, size)) {
            return -1;
        }
    }
    if ((err = hashmap_insert_no_resize(hashmap, element, size))) {
        return err;
    } else {
        ++hashmap->elems;
        return 0;
    }
}

int hashmap_erase(hashmap* hashmap, const void* element, size_t size) {
    size_t hash = hashmap->hash(element);
    size_t mod = hash % hashmap->len;
    int contains;
    size_t index = hashmap_bsearch(hashmap, hash, mod, &contains, size);
    if (contains) {
        vec_remove(&hashmap->mods[mod], size, index);
        --hashmap->elems;
    }
    return contains;
}

void hashmap_iterate(hashmap* hashmap, size_t size, void (*fun)(void*, void*), void* userdata) {
    size_t mod;
    for (mod = 0; mod != hashmap->len; ++mod) {
        elemvec* vec = &hashmap->mods[mod];
        size_t i;
        for (i = 0; i != vec->len; ++i) {
            fun(&vec->elems[i * size], userdata);
        }
    }
}

size_t size_t_hash(const void* v) {
    return *(const size_t*)v;
}

size_t str_hash(const void* v) {
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
    ASSERT(!hashmap_insert(hashmap, &num, sizeof(size_t)), cleanup);
    ASSERT(hashmap_contains(hashmap, &num, sizeof(size_t)), cleanup);
    num = 8;
    ASSERT(!hashmap_contains(hashmap, &num, sizeof(size_t)), cleanup);
cleanup:
    hashmap_destroy(hashmap);
}
END_TEST

TEST(test_hashmap_erase) {
    hashmap* hashmap = hashmap_new(size_t_hash);
    size_t num;
    ASSERT(hashmap, cleanup);
    num = 3;
    ASSERT(!hashmap_insert(hashmap, &num, sizeof(size_t)), cleanup);
    ASSERT(hashmap_contains(hashmap, &num, sizeof(size_t)), cleanup);
    num = 8;
    ASSERT(!hashmap_contains(hashmap, &num, sizeof(size_t)), cleanup);
    ASSERT(!hashmap_erase(hashmap, &num, sizeof(size_t)), cleanup);
    num = 3;
    ASSERT(hashmap_erase(hashmap, &num, sizeof(size_t)), cleanup);
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
        ASSERT(!hashmap_insert(hashmap, &num, sizeof(size_t)), cleanup);
        i = -1;
        ASSERT(!hashmap_contains(hashmap, &i, sizeof(size_t)), cleanup);
        for (i = 0; i <= num; ++i) {
            ASSERT(hashmap_contains(hashmap, &i, sizeof(size_t)), cleanup);
        }
        i = num + 1;
        ASSERT(!hashmap_contains(hashmap, &i, sizeof(size_t)), cleanup);
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
