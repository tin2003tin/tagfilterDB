
#ifndef STORAGE_TAGFILTERDB_INCLUDE_EXPORT_H_
#define STORAGE_TAGFILTERDB_INCLUDE_EXPORT_H_

#if !defined(TAGFILTERDB_EXPORT)

#if defined(TAGFILTERDB_SHARED_LIBRARY)
#if defined(_WIN32)

#if defined(TAGFILTERDB_COMPILE_LIBRARY)
#define TAGFILTERDB_EXPORT __declspec(dllexport)
#else
#define TAGFILTERDB_EXPORT __declspec(dllimport)
#endif // defined(TAGFILTERDB_COMPILE_LIBRARY)

#else // defined(_WIN32)
#if defined(TAGFILTERDB_COMPILE_LIBRARY)
#define TAGFILTERDB_EXPORT __attribute__((visibility("default")))
#else
#define TAGFILTERDB_EXPORT
#endif
#endif // defined(_WIN32)

#else // defined(TAGFILTERDB_SHARED_LIBRARY)
#define TAGFILTERDB_EXPORT
#endif

#endif // !defined(TAGFILTERDB_EXPORT)

#endif // STORAGE_TAGFILTERDB_INCLUDE_EXPORT_H_
