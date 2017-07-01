/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 Chris Gregory czipperz@gmail.com
 */

/*! \file vec.h
 *
 * \brief Defines functions for interacting with a vector data
 * structure.
 *
 * It assumes the implmentation of a vector to be:
\code{.c}
struct vec { item_type* pointer; size_t length; size_t capacity; };
\endcode
 *
 * The functions uniformly return -1 on memory allocation failure.
 * They all use rpmalloc and variants to allocate memory.
 *
 * Example:
\code{.c}
struct { int* ints; size_t length; size_t capacity } vector = VEC_INIT;
int i = 3;
if (vec_push(&vector, sizeof(int), &i)) {
    abort();
}
i = 10;
if (vec_push(&vector, sizeof(int), &i)) {
    abort();
}
assert(vec.ints);
assert(vec.ints[0] == 3);
assert(vec.ints[1] == 10);
assert(vec.length == 2);
assert(vec.capacity >= 2);
\endcode
 */

#ifndef CUTIL_VEC_H
#define CUTIL_VEC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/*! \brief Increase the capacity to \c new_cap.
 *
 * \return Returns -1 on memory allocation error, 0 on success. */
int vec_reserve(void* self, size_t sizeof_elem, size_t new_cap);

/*! \brief Reserve an extra element, then insert `str` at `index`.
 *
 * \return Returns -1 on memory allocation error, 0 on success. */
int vec_insert(void* self, size_t sizeof_elem, size_t index,
               const void* elem);

/*! \brief Reserve an extra element, then insert \c str at the end.
 *
 * \return Returns -1 on memory allocation error, 0 on success. */
int vec_push(void* self, size_t sizeof_elem, const void* elem);

/*! \brief Shrink the capacity to its length, reallocating memory.
 *
 * Error handling for this function can be ignored: it is only an
 * optimization.
 *
 * \return Returns -1 on memory allocation error, 0 on success. */
int vec_shrink_to_size(void* self, size_t sizeof_elem);

/*! \brief Remove an element at \c index. */
void vec_remove(void* self, size_t sizeof_elem, size_t index);

#define VEC_INIT {0, 0, 0}

#ifdef __cplusplus
}
#endif

#endif
