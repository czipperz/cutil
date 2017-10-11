/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 Chris Gregory czipperz@gmail.com
 */

#include "../dll.h"

#ifdef _WIN32
#include "../rpmalloc.h"
#include <Windows.h>

/* On Windows we try to avoid extra allocation but it sometimes is
 * impossible.  The allocation is only needed if HMODULE is bigger
 * than a pointer.  If it isn't then we cast it to and from a
 * pointer. */
struct dll {
    HMODULE module;
};

dll*
dll_open(const char* file_name) {
    HMODULE module = LoadLibrary(file_name);
    if (!module) {
        return 0;
    }
    if (sizeof(HMODULE) > sizeof(dll*)) {
        dll* dll = rpmalloc(sizeof(struct dll));
        if (!dll) {
            FreeLibrary(module);
        }
        dll->module = module;
        return dll;
    } else {
        return (dll*) module;
    }
}

int
dll_close(dll* dll) {
    HMODULE module;
    if (sizeof(HMODULE) > sizeof(dll*)) {
        module = dll->module;
        rpfree(dll);
    } else {
        module = (HMODULE) dll;
    }
    return !FreeLibrary(module);
}

void*
dll_symbol(dll* dll, const char* symbol_name) {
    HMODULE module;
    if (sizeof(HMODULE) > sizeof(dll*)) {
        module = dll->module;
    } else {
        module = (HMODULE) dll;
    }
    return GetProcAddress(module, symbol_name);
}

/* End _WIN32 only */
#else
#include <dlfcn.h>

dll*
dll_open(const char* file_name) {
    return dlopen(file_name,
                  /* Mimick LoadLibrary on Windows which doesn't allow
                   * options. */
                  RTLD_NOW | RTLD_GLOBAL);
}

int
dll_close(dll* dll) {
    return dlclose(dll);
}

void*
dll_symbol(dll* dll, const char* symbol_name) {
    return dlsym(dll, symbol_name);
}
#endif
