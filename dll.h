#pragma once

#ifndef CUTIL_DLL_H
#define CUTIL_DLL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define dll_export __declspec(dllexport)
#define dll_import __declspec(dllimport)
#else
#define dll_export
#define dll_import
#endif

#ifdef CUTIL_IS_COMPILING
#define CUTIL_API dll_export
#else
#define CUTIL_API dll_import
#endif

typedef struct dll dll;

CUTIL_API dll* dll_open(const char* file_name);
CUTIL_API int dll_close(dll*);
CUTIL_API void* dll_symbol(dll*, const char* symbol_name);

#ifdef __cplusplus
}
#endif

#endif
