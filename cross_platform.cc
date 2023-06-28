#include "cross_platform.h"
#include <vector>
#include <time.h>
#include <set>
#include <algorithm>
#ifdef __unix__
#include <iconv.h>
#include <dirent.h>
#include <sys/sendfile.h>  // sendfile
#include <fcntl.h>         // open
#else
#include <tchar.h>
#include <psapi.h>
#endif

#ifndef __unix__
/*
https://msdn.microsoft.com/en-us/library/windows/desktop/dd374130(v=vs.85).aspx
https://msdn.microsoft.com/en-us/library/windows/desktop/dd317756(v=vs.85).aspx
*/
constexpr UINT LATIN_CODEPAGE = 1252;
#endif


#ifndef __linux
constexpr UINT UTF8_CODEPAGE = CP_UTF8;
#endif

std::wstring kFromUtf8(const std::string& utf8string)
{
#ifdef __unix__
	setlocale(LC_ALL, "");
	const char *str = utf8string.c_str();
	wchar_t buffer[1024];
	int ret;
	ret = mbstowcs(buffer, str, 1024);
	if (ret == 1023) {
		// error in conversion
		buffer[1023] = L'\0';
	}
	std::wstring result(buffer);
	return result;
#else
	int widesize = ::MultiByteToWideChar(UTF8_CODEPAGE, 0, utf8string.c_str(), -1, nullptr, 0);
	if (widesize == 0) {
		int err = GetLastError();
		std::string str = ("Error " + std::to_string(err) + " in conversion of UTF8 string: \n") + utf8string;
		throw std::exception(str.c_str());
	}
	std::vector<wchar_t> resultstring(widesize);
	int convresult = ::MultiByteToWideChar(UTF8_CODEPAGE, 0, utf8string.c_str(), -1, &resultstring[0], widesize);
	if (convresult != widesize) {
		throw std::exception("kFromUtf8: widesize error");
	}
	return std::wstring(&resultstring[0]);
#endif
}

std::string kToUtf8(const std::wstring& widestring)
{
#ifdef __unix__
	setlocale(LC_ALL, "");
	const wchar_t *str = widestring.c_str();
	char buffer[1024];
	int ret;
	ret = wcstombs(buffer, str, 1024);
	if (ret == 1023) {
		// error in conversion
		buffer[0] = '\0';
	}
	std::string result(buffer);
	return result;
#else
	int utf8size = ::WideCharToMultiByte(UTF8_CODEPAGE, 0, widestring.c_str(), -1, NULL, 0, NULL, NULL);
	if (utf8size == 0) {
		throw std::exception("kToUtf8: Error in conversion.");
	}
	std::vector<char> resultstring(utf8size);
	int convresult = ::WideCharToMultiByte(UTF8_CODEPAGE, 0, widestring.c_str(), -1, &resultstring[0], utf8size, NULL, NULL);
	if (convresult != utf8size) {
		throw std::exception("La falla!");
	}
	return std::string(&resultstring[0]);
#endif
}

std::wstring kFromkWchar(const kWchar *s)
{
	std::wstring wStringData;
#ifdef __unix__
	wStringData = L"";
	unsigned int i = 0;
	wchar_t c;
	while (s[i]) {
		c = s[i];
		wStringData += c;
		i++;
	}
#else
	wStringData = (wchar_t*)s;
#endif
	return wStringData;
}

char *kitoa(int32_t i, char* s, size_t maxLen, int dummy_radix) {
#ifdef __unix__
	snprintf(s, maxLen, "%" PRIi32, i);
#else
	if (_itoa_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = '\0';
	}
#endif
	return s;
}

wchar_t *kitow(int32_t i, wchar_t* s, size_t maxLen, int dummy_radix) {
#ifdef __unix__
	swprintf(s, maxLen, L"%" PRIi32, i);
#else
	if (_itow_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = L'\0';
	}
#endif
	return s;
}

char *ki64toa(int64_t i, char* s, size_t maxLen, int dummy_radix)
{
#ifdef __unix__
	snprintf(s, maxLen, "%" PRIi64, i);
#else
	if (_i64toa_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = '\0';
	}
#endif
	return s;
}

wchar_t *ki64tow(int64_t i, wchar_t* s, size_t maxLen, int dummy_radix)
{
#ifdef __unix__
	swprintf(s, maxLen, L"%" PRIi64, i);
#else
	if (_i64tow_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = L'\0';
	}
#endif
	return s;
}

char *kutoa(uint32_t i, char* s, size_t maxLen, int dummy_radix) {
#ifdef __unix__
	snprintf(s, maxLen, "%" PRIu32, i);
#else
	if (_ultoa_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = '\0';
	}
#endif
	return s;
}

wchar_t *kutow(uint32_t i, wchar_t* s, size_t maxLen, int dummy_radix) {
#ifdef __unix__
	swprintf(s, maxLen, L"%" PRIu32, i);
#else
	if (_ultow_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = L'\0';
	}
#endif
	return s;
}

wchar_t *kltow(long i, wchar_t* s, size_t maxLen, int radix) {
#ifdef __unix__
	swprintf(s, maxLen, L"%" PRIu32, i);
#else
	if (_ltow_s(i, s, maxLen, radix) != 0) {
		s[0] = L'\0';
	}
#endif
	return s;
}

char *ku64toa(uint64_t i, char* s, size_t maxLen, int dummy_radix) {
#ifdef __unix__
	snprintf(s, maxLen, "%" PRIu64, i);
#else
	if (_ui64toa_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = '\0';
	}
#endif
	return s;
}

wchar_t *ku64tow(uint64_t i, wchar_t* s, size_t maxLen, int dummy_radix) {
#ifdef __unix__
	swprintf(s, maxLen, L"%" PRIu64, i);
#else
	if (_ui64tow_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = L'\0';
	}
#endif
	return s;
}

int kwtoi(const wchar_t *str)
{
#ifdef __unix__
	std::string sstr = kToUtf8(str);
    char *tmp = sstr.data();
	return atoi(tmp);
#else
	return _wtoi(str);
#endif
}

int64_t kwtoi64(const wchar_t *str)
{
#ifdef __unix__
    wchar_t *ptr;
    int64_t result = wcstoll(str, &ptr, 10);
	return result;
#else
	return _wtoi64(str);
#endif
}

double kwtof(const wchar_t *str)
{
#ifdef __unix__
	std::string sstr = kToUtf8(str);
	char *tmp = sstr.data();
	return atof(tmp);
#else
	return _wtof(str);
#endif
}

long kwtol(const wchar_t *str)
{
#ifdef __unix__
	std::string sstr = kToUtf8(str);
	char *tmp = sstr.data();
	return atof(tmp);
#else
	return _wtol(str);
#endif
}

void k_wcsupr_s(wchar_t *string, unsigned size)
{
#ifdef __unix__
	for (int i = 0; i < size; i++)
	{
		string[i] = towupper(string[i]);
	}
#else
	_wcsupr_s(string, size);
#endif
}

char * k_strdup(const char *strSource)
{
#ifdef __unix__
	return strdup(strSource);
#else
	return _strdup(strSource);
#endif
}

int kstrcasecmp(const char *string1, const char *string2)
{
#ifdef __unix__
	return strcasecmp(string1, string2);
#else
	return _stricmp(string1, string2);
#endif
}

double kwcstod(const wchar_t *str, wchar_t **endptr)
{
#ifdef __unix__
	std::string sstr = kToUtf8(str);
	char *tmp = sstr.data();
	return strtof(tmp, (char**)endptr);
#else
	return wcstod(str, endptr);
#endif
}

char *kstrlwr(char *text)
{
#ifdef __unix__
	char *origtext = text;
	while (*text != '\0')
	{
		if ((*text > 64) && (*text < 91)) *text += 32;
		text++;
	}
	return (origtext);
#else
	return _strlwr(text);
#endif
}

errno_t kstrncpy_s(char *strDestination, size_t numberOfElements, const char *strSource)
{
#ifdef __unix__
	strncpy(strDestination, strSource, numberOfElements);
	return 0;
#else
	return strcpy_s(strDestination, numberOfElements, strSource);
#endif
}

errno_t kstrncat_s(char *strDestination, size_t numberOfElements, const char *strSource)
{
#ifdef __unix__
	strncat(strDestination, strSource, numberOfElements);
	return 0;
#else
	return strcat_s(strDestination, numberOfElements, strSource);
#endif
}

errno_t k_wcscpy_s(wchar_t *strDestination, size_t numberOfElements, const wchar_t *strSource)
{
#ifdef __unix__
	wcscpy(strDestination, strSource);
	return 0;
#else
	return wcscpy_s(strDestination, numberOfElements, strSource);
#endif
}

char *kstrtok(char *str, const char *delim, char **saveptr)
{
#ifdef __unix__
	return strtok_r(str, delim, saveptr);
#else
	return strtok_s(str, delim, saveptr);
#endif
}

wchar_t *kwcstok(wchar_t *wstr, const wchar_t *delim)
{
#ifdef __unix__
	wchar_t *state;
	return wcstok(wstr, delim, &state);
#else
	return wcstok(wstr, delim);
#endif
}