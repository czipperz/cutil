/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 Chris Gregory czipperz@gmail.com
 */

/*! \file hashmap.h
 *
 * \brief Two implementation of a hash map optimized differently.
 *
 * \c hashmap optimizes for speed while sacrificing memory usage.
 * This uses an array of arrays approach.
 *
 * \c hashtable optimizes for memory usage while sacrificing speed.
 * This uses an array approach.
 */

#ifndef CUTIL_HASHMAP_H
#define CUTIL_HASHMAP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hashmap hashmap;
hashmap* hashmap_new(size_t (*hash)(const void*));
void hashmap_destroy(hashmap*);
int hashmap_contains(const hashmap*, const void* element, size_t size);
int hashmap_insert(hashmap*, const void* element, size_t size);
int hashmap_erase(hashmap*, const void* element, size_t size);
void hashmap_iterate(hashmap*, size_t size,
                     void (*fun)(void* element, void* userdata),
                     void* userdata);

typedef struct hashtable hashtable;
hashtable* hashtable_new(size_t (*hash)(const void*));
void hashtable_destroy(hashtable*);
int hashtable_contains(const hashtable*, const void* element, size_t size);
int hashtable_insert(hashtable*, const void* element, size_t size);
int hashtable_erase(hashtable*, const void* element);

size_t str_hash(const void*);
size_t size_t_hash(const void*);

#ifdef __cplusplus
}
#endif

#endif
