/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 Chris Gregory czipperz@gmail.com
 */

/*! \file hashmap.h
 *
 * \brief An implementation of a hash map.
 *
 * It uses an array of arrays approach.
 */

#ifndef CUTIL_HASHMAP_H
#define CUTIL_HASHMAP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hashmap hashmap;
/*! \brief Create a hashmap that will map elements to hashes based on this hashing function.
 *
 * Returns null on error (in malloc).
 */
hashmap* hashmap_new(size_t (*hash)(const void*));
/*! \brief Destroy the hashmap.  It is illegal to be used past this point. */
void hashmap_destroy(hashmap*);
/*! \brief Get the number of items in this hash map.
 *
 * This has O(1) performance.
 */
size_t hashmap_size(const hashmap*);
/*! \brief Check if the element is contained in this hash map.
 *
 * This has amortized O(1) performance (assuming the hash algorithm is semi random).
 */
int hashmap_contains(const hashmap*, const void* element, size_t size);
/*! \brief Insert an element into this hash map.
 *
 * If the element already was in the hash map, returns 1.
 * If an error occured, return -1 (this does not corrupt the hash map).
 * Otherwise returns 0.
 */
int hashmap_insert(hashmap*, const void* element, size_t size);
/*! \brief Erase an elememnt from this hash map.
 *
 * If the element didn't exist in the hash map, returns 1.
 * Otherwise returns 0.
 */
int hashmap_erase(hashmap*, const void* element, size_t size);

/*! \brief Iterate through the hash map.
 *
 * This is more efficient than creating an iterator, but is more
 * intrusive into your code.
 */
void hashmap_iterate(hashmap*, size_t size,
                     void (*fun)(void* element, void* userdata),
                     void* userdata);

typedef struct hashmap_iterator hashmap_iterator;
/*! \brief An iterator into the hash map.
 *
 * This is less efficient than using the iterate function, but is less
 * intrusive into your code.
 *
 * Copying an iterator (via a bitwise copy) is perfectly legal.
 */
struct hashmap_iterator {
    hashmap* _hashmap;
    size_t _outer;
    size_t _inner;
};
/*! \brief Create a new iterator.
 *
 * This is needed to get the iterator to start pointing at the correct element.
 *
 * Given this code:
\code{.c}
hashmap_iterator iterator = {hashmap};
void* peek_first = hashmap_iterator_peek(&iterator);
void* next_first = hashmap_iterator_next(&iterator);
\endcode
 *
 * Based on how the internal data is structured, \c peek_first can be
 * invalid and \c next_first will retrieve the first element.  Or,
 * given a different internal structure, \c peek_first can be the
 * first element and \c next_first will retrieve the second element.
 *
 * Make this consistent by calling this new method.
 */
hashmap_iterator hashmap_iterator_new(hashmap*);
/*! \brief Retrieve the next element and increment the iterator.
 *
 * Calling next then next may return two different pointers (one or
 * more can be null).
 *
 * Calling next then peek may return two different pointers (one or
 * more can be null).
 *
 * Calling peek then next will return the same pointer (it may be
 * null).
 */
void* hashmap_iterator_next(hashmap_iterator*, size_t size);
/*! \brief Retrieve the next element.
 *
 * Calling peek then peek will return the same pointer (it may be
 * null).
 */
void* hashmap_iterator_peek(const hashmap_iterator*, size_t size);

size_t str_hash(const void*);
size_t size_t_hash(const void*);

#ifdef __cplusplus
}
#endif

#endif
