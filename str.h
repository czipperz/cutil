/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 Chris Gregory czipperz@gmail.com
 */

/*! \file str.h
 *
 * \brief This file defines the \c str structure, a utf8 string with
 * short string optimization.
 *
 * utf8 conformity is established using glib, using Debug and Release
 * assertions.  Argument validity is checked in Debug builds, such as
 * pointers being in the correct area of the string.
 *
 * Functions return -1 on allocation failure.
 */

#ifndef CUTIL_STR_H
#define CUTIL_STR_H

#include <stddef.h>
#include <stdint.h>

/*! \brief A utf8 string with short string optimization.
 *
 * Every function but \c str_set_len_bytes() that modifies the string
 * will ensure it is utf8 compliant.
 *
 * It is NOT compliant with the \c vec concept.
 *
 * The string will automatically inline itself when its length is
 * less than \c sizeof(str). */
struct str {
    /*! \brief The inline representation. */
    char _data[sizeof(struct {
        char* str;
        size_t blen;
        size_t _cap;
    })];
};
typedef struct str str;

#define STR_INIT {{0}}

/*! \brief Free memory used by \c self . */
void str_destroy(str* self);

/*! \brief Get a mutable pointer to the end of the str. */
char* str_begin(str* self);

/*! \brief Get a constant pointer to the beginning of the str. */
const char* str_cbegin(const str* self);

/*! \brief Get a mutable pointer to the end of the str.*/
char* str_end(str* self);

/*! \brief Get a constant pointer to the end of the str. */
const char* str_cend(const str* self);

/*! \brief Get the number of bytes used by the str.
 *
 * Complexity: O(1) to O(sizeof(str)) */
size_t str_len_bytes(const str* self);

/*! \brief Calculate the number of characters in the str.
 *
 * Complexity: O(n) where n = str_len_bytes(self) */
size_t str_len_characters(const str* self);

/*! \brief Retrieve the capacity in bytes of the string. */
size_t str_cap(const str* self);

/*! \brief Increase the capacity of the string to be at least
 *  \c new_cap.
 *
 * \return -1 on reallocation failure. */
int str_reserve(str* self, size_t new_cap);

/*! \brief Shrink the string's capacity to its length.
 *
 * \return -1 on reallocation failure. */
int str_shrink_to_size(str* self);

/*! \brief Insert \c character at the end of the str.
 *
 * This verifies that \c character is a valid utf32 character.
 *
 * \return -1 on reallocation failure. */
int str_push(str* self, uint32_t character);

/*! \brief Insert \c string at the end of the str.
 *
 * This verifies that \c string is a valid utf8 string.
 *
 * \return -1 on reallocation failure. */
int str_push_s(str* self, const char* string);

/*! \brief Insert \c len_bytes bytes of \c string at the end of the
 *  str.
 *
 * This verifies that \c len_bytes bytes of \c string are a valid utf8
 * string.
 *
 * \return -1 on reallocation failure. */
int str_push_sn(str* self, const char* string, size_t len_bytes);

/*! \brief Insert \c character at \c pos in the str.
 *
 * This verifies that \c character is a valid utf32 character.
 *
 * \return -1 on reallocation failure. */
int str_insert(str* self, const char* pos, uint32_t character);

/*! \brief Insert \c string at \c pos in the str.
 *
 * This verfies that \c string is a valid utf8 string.
 *
 * \return -1 on reallocation failure. */
int str_insert_s(str* self, const char* pos, const char* string);

/*! \brief Insert \c len_bytes bytes of \c string at \c pos of the str.
 *
 * This verifies that \c len_bytes bytes of \c string are a valid utf8
 * string.
 *
 * \return -1 on reallocation failure. */
int str_insert_sn(str* self, const char* pos,
                  const char* string, size_t len_bytes);

/*! \brief Set the byte length of the string to \c len_bytes and add a
 *  null terminator.  THIS DOES NOT CHECK FOR UTF8 VALIDITY!
 *
 * \return The new length, \c len_bytes. */
size_t str_set_len_bytes(str* self, size_t len_bytes);

/*! \brief Copy \c string into the str.
 *
 * This verifies that \c string is a valid utf8 string.
 *
 * \return -1 on reallocation failure. */
int str_copy(str* self, const char* string);

/*! \brief Copy \c len_bytes bytes from \c string into the str.
 *
 * This verifies that \c len_bytes bytes of \c string are a valid utf8
 * string.
 *
 * \return -1 on reallocation failure. */
int str_copy_n(str* self, const char* string, size_t len_bytes);

/* /\*! \brief Erase elements after and including \c begin. */
/*  * */
/*  * May cause deallocation of the string (because it is short enough to */
/*  * be stored in a short string). *\/ */
/* void str_erase_after(str* self, size_t begin_byte); */

/*! \brief Erase elements between \c begin and \c end, excluding \c end.
 *
 * May cause deallocation of the string (because it is short enough to
 * be stored in a short string). */
void str_erase(str* self, size_t begin, size_t end);

/*! \brief Erase \c num elements between \c begin and \c end.
 *
 * May cause deallocation of the string (because it is short enough to
 * be stored in a short string). */
void str_erase_n_bytes(str* self, size_t begin, size_t num);

#endif
