#pragma once

#pragma warning(disable : 4018)	// signed/unsigned warnings
#pragma warning(disable : 4100)
#pragma warning(disable : 4251)
#pragma warning(disable : 4512)
#pragma warning(disable : 4996)

#ifdef _X64_
#pragma warning(disable : 4267) // warning C4267: 'argument' : conversion from 'size_t' to 'int', possible loss of data
#pragma warning(disable : 4244) // warning C4244: '=' : conversion from '__int64' to 'int', possible loss of data
#endif

#if defined(_WIN32) || defined(WIN32)
#ifdef BUILD_SQLITE_REFLECTION
#define REFLECTION_EXPORT __declspec( dllexport )
#else
#define REFLECTION_EXPORT __declspec( dllimport )
#endif
#else
#define REFLECTION_EXPORT __attribute__ ((__visibility__("default")))
#endif
