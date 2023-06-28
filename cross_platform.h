#pragma once

#pragma warning(disable : 4996)

#include <string>
#include <cstdint>
#include <fstream>

#if !defined(_WIN32) && !defined(WIN32)
#define __unix__
#endif

#ifdef __unix__
//#define _SH_DENYRW 0
//#define _SH_DENYWR 0
#endif

typedef uint16_t wide_char_t;

#ifdef __unix__
//#include <ctype.h>
//#include <cwchar>
//#include <wctype.h>
//#include <wchar.h>
//#include <unistd.h>
//#define __STDC_FORMAT_MACROS
//#include <inttypes.h>
//#include <linux/limits.h>
//#include <cfloat>
//#include <cstdlib>
//#include <clocale>
//#include <cstring>
//#include <libgen.h>
//#include <sys/stat.h>
//#include <sys/types.h>
//#ifndef TRUE
//#define TRUE 1
//#endif
//#ifndef FALSE
//#define FALSE 0
//#endif
//#ifndef MAX_PATH
//#define MAX_PATH 600
//#endif
//	typedef unsigned long HKEY;
//#ifndef BOOL
//	typedef int BOOL;
//#endif
//
//#ifndef _MAX_PATH
//#define _MAX_PATH PATH_MAX
//#endif
//#ifndef _MAX_FNAME
//#define _MAX_FNAME NAME_MAX
//#endif
//#define _MAX_DIR    256 /* max. length of path component */
//#define _MAX_EXT    256 /* max. length of extension component */
//
//	typedef int INT;
//	typedef unsigned long DWORD;
//
//#define LOBYTE(w)           (w & 0xff)
//#define HIBYTE(w)           ((w >> 8) & 0xff)
#endif

namespace cross_platform {
	std::wstring FromUtf8(const std::string& utf8_string);
	std::string ToUtf8(const std::wstring& wide_string);
	std::wstring FromWideChar(const wide_char_t* s);
	void Assign(std::wstring& wide_string, const wide_char_t* c);
	int ToInt(const wchar_t* str);
	double ToDouble(const wchar_t* str);
}
