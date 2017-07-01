/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 Chris Gregory czipperz@gmail.com
 */

/* Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt */

#ifndef CUTIL_STACK_TRACE_H
#define CUTIL_STACK_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) ||                                \
    (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) ||  \
    (defined(__ICC) && (__ICC >= 600)) ||               \
    defined(__ghs__) ||                                 \
    (defined(__DMC__) && (__DMC__ >= 0x810))
# define STACK_TRACE_FUNCTION_MACRO __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
# define STACK_TRACE_FUNCTION_MACRO __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) ||       \
    (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
# define STACK_TRACE_FUNCTION_MACRO __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
# define STACK_TRACE_FUNCTION_MACRO __FUNC__
#elif (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)) ||    \
    (defined(__cplusplus) && (__cplusplus >= 201103))
# define STACK_TRACE_FUNCTION_MACRO __func__
#else
# define STACK_TRACE_FUNCTION_MACRO "(unknown)"
#endif

#define STACK_TRACE_REGISTER()                          \
    (stack_trace_register(__FILE__, __LINE__,           \
                          STACK_TRACE_FUNCTION_MACRO))
#define STACK_TRACE_RETURN(x)                   \
    do {                                        \
        STACK_TRACE_REGISTER();                 \
        return x;                               \
    } while (0)
#define STACK_TRACE_THROW(x)                    \
    do {                                        \
        stack_trace_clear();                    \
        STACK_TRACE_REGISTER();                 \
        return x;                               \
    } while (0)
#define STACK_TRACE_PRINT()                     \
    do {                                        \
        STACK_TRACE_REGISTER();                 \
        stack_trace_print();                    \
    } while (0)

void stack_trace_clear(void);
void stack_trace_register(const char* file, int line,
                          const char* func);
void stack_trace_print(void);

#ifdef __cplusplus
}
#endif

#endif

