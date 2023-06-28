#pragma once

#pragma warning(disable : 4996)

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <math.h>
#include <cstdint>
#include <fstream>
#include <vector>

#if !defined(_WIN32) && !defined(WIN32)
#define __unix__
#endif

#ifdef __unix__
#define _SH_DENYRW 0
#define _SH_DENYWR 0
#endif

typedef uint16_t kWchar;

#ifdef __unix__
    #include <ctype.h>
	typedef int errno_t;
    #include <cwchar>
    #include <wctype.h>
    #include <wchar.h>
    #include <unistd.h>
    #define __STDC_FORMAT_MACROS
	#include <inttypes.h>
    #include <linux/limits.h>
    #include <cfloat>
	#include <cstdlib>
	#include <clocale>
    #include <cstring>
	#include <libgen.h>
	#include <sys/stat.h>
	#include <sys/types.h>
#ifndef TRUE
    #define TRUE 1
#endif
#ifndef FALSE
    #define FALSE 0
#endif
#ifndef MAX_PATH
    #define MAX_PATH 600
#endif
    typedef unsigned long HKEY;
#ifndef BOOL
    typedef int BOOL;
#endif

#ifndef _MAX_PATH
    #define _MAX_PATH PATH_MAX
#endif
#ifndef _MAX_FNAME
    #define _MAX_FNAME NAME_MAX
#endif
    #define _MAX_DIR    256 /* max. length of path component */
    #define _MAX_EXT    256 /* max. length of extension component */

    typedef int INT;
    typedef unsigned long DWORD;

	#define LOBYTE(w)           (w & 0xff)
	#define HIBYTE(w)           ((w >> 8) & 0xff)
#else
#include <windows.h>
#include <float.h>
#endif

// ------------ string conversion/formatting/handling
std::wstring kFromLatin1(const std::string& latin1string);
std::string kToLatin1(const std::wstring& widestring);
std::wstring kFromUtf8(const std::string& utf8string);
std::string kToUtf8(const std::wstring& widestring);
std::wstring kFromkWchar(const kWchar* s);
std::wstring ktoRTFW(const std::wstring& data);
void kAssign(std::wstring& wStringData, const kWchar* r);

char* kitoa(int32_t i, char* s, size_t maxLen, int dummy_radix);
wchar_t* kitow(int32_t i, wchar_t* s, size_t maxLen, int dummy_radix);
char* ki64toa(int64_t i, char* s, size_t maxLen, int dummy_radix);
wchar_t* ki64tow(int64_t i, wchar_t* s, size_t maxLen, int radix = 10);
char* kutoa(uint32_t i, char* s, size_t maxLen, int dummy_radix);
wchar_t* kutow(uint32_t i, wchar_t* s, size_t maxLen, int dummy_radix);
wchar_t* kltow(long i, wchar_t* s, size_t maxLen, int radix = 10);
char* ku64toa(uint64_t i, char* s, size_t maxLen, int dummy_radix);
wchar_t* ku64tow(uint64_t i, wchar_t* s, size_t maxLen, int dummy_radix);

int kwtoi(const wchar_t* str);
int64_t kwtoi64(const wchar_t* str);
double kwtof(const wchar_t* str);
long kwtol(const wchar_t* str);

void k_wcsupr_s(wchar_t* string, unsigned size);
char* k_strdup(const char* strSource);
int kstrcasecmp(const char* string1, const char* string2);
double kwcstod(const wchar_t* str, wchar_t** endptr);
char* kstrlwr(char* text);
errno_t kstrncpy_s(char* strDestination, size_t numberOfElements, const char* strSource);
errno_t kstrncat_s(char* strDestination, size_t numberOfElements, const char* strSource);
errno_t k_wcscpy_s(wchar_t* strDestination, size_t numberOfElements, const wchar_t* strSource);
char* kstrtok(char* str, const char* delim, char** saveptr);
wchar_t* kwcstok(wchar_t* wstr, const wchar_t* delim);
std::vector<std::wstring> kToWStringList(const std::wstring& wStringData, wchar_t sep, bool keepEmptyParts);
std::vector<std::string> kToStringList(const char* _buf, char seperator, bool keepEmptyParts);


// ------------ templates/inlines

inline void k_strLower(char* pstr) {
	while (*pstr++) {
		*pstr = (char)tolower(*pstr);
	}
}

inline int kfinite(double x) {
#ifdef __unix__
    return finite(x);
#else
	return _finite(x);
#endif
}

inline int kIsNan(double x) {
#ifdef __unix__
    return isnan(x);
#else
	return _isnan(x);
#endif
}

// floating point to string
inline char* kgcvt(double number, int ndigit, char* buf) {
#ifdef __unix__
    return gcvt(number, ndigit, buf);
#else
	return _gcvt(number, ndigit, buf);
#endif
}

inline double khypot(double x, double y) {
#ifdef __unix__
    return hypot(x, y);
#else
	return _hypot(x, y);
#endif
}

// to upper
inline char* k_strupr(char* s) {
#ifdef __unix__
	for (; *s; s++)
		*s = toupper((unsigned char) *s);
	return s;
#else
	return _strupr(s);
#endif
}

template <size_t size>
inline int k_snprintf_s(char (&buffer)[size], size_t count, const char* arg1, ...) {
	va_list args;
	va_start(args, arg1);
	int res;
#ifdef __unix__
    res = vsnprintf(buffer, size, arg1, args);
#else
	res = _vsnprintf_s(buffer, size, _TRUNCATE, arg1, args);
#endif
	va_end(args);
	return res;
}

inline int ksnprintf_s(char* buffer, size_t size, const char* format, ...) {
	va_list args;
	va_start(args, format);
	int res;
#ifdef __unix__
    res = vsnprintf(buffer, size, format, args);
#else
	res = _vsnprintf_s(buffer, size, _TRUNCATE, format, args);
#endif
	va_end(args);
	return res;
}

#ifdef __unix__
inline int ksprintf_s(char *buffer, const char *format, ...)
#else
template <size_t size>
inline int ksprintf_s(char (&buffer)[size], const char* format, ...)
#endif
{
	va_list args;
	va_start(args, format);
	int res;
#ifdef __unix__
	res = vsprintf(buffer, format, args);
#else
	res = vsprintf_s(buffer, format, args);
#endif
	va_end(args);
	return res;
}

inline int kwsprintf(wchar_t* buffer, const wchar_t* format, ...) {
	va_list args;
	va_start(args, format);
	int res;
#ifdef __unix__
    res = vswprintf(buffer, 2000, format, args);
#else
	res = wsprintfW(buffer, format, args);
#endif
	va_end(args);
	return res;
}

template <size_t size>
int kswprintf_s(wchar_t (&buffer)[size], const wchar_t* format, ...) {
	va_list args;
	va_start(args, format);
	int res;
#ifdef __unix__
    res = vswprintf(buffer, size, format, args);
#else
	res = vswprintf_s(buffer, format, args);
#endif
	va_end(args);
	return res;
}

#ifdef __unix__
inline char *kstrcpy_s(char *strDestination, const char *strSource)
#else
template <size_t size>
errno_t kstrcpy_s(char (&strDestination)[size], const char* strSource)
#endif
{
#ifdef __unix__
    return strcpy(strDestination, strSource);
#else
	return strcpy_s(strDestination, strSource);
#endif
}

template <size_t size>
errno_t kwcscpy_s(wchar_t (&strDestination)[size], const wchar_t* strSource) {
#ifdef __unix__
    wcscpy(strDestination, strSource);
    return 0;
#else
	return wcscpy_s(strDestination, strSource);
#endif
}

#ifdef __unix__
inline char *kstrcat_s(char *strDestination, const char *strSource)
#else
template <size_t size>
errno_t kstrcat_s(char (&strDestination)[size], const char* strSource)
#endif
{
#ifdef __unix__
    return strcat(strDestination, strSource);
#else
	return strcat_s(strDestination, strSource);
#endif
}

template <size_t size>
errno_t kwcscat_s(wchar_t (&strDestination)[size], const wchar_t* strSource) {
#ifdef __unix__
    wcscat(strDestination, strSource);
    return 0;
#else
	return wcscat_s(strDestination, strSource);
#endif
}