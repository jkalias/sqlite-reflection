#pragma once

#pragma warning(disable : 4996)

#include <string>

#if !defined(_WIN32) && !defined(WIN32)
#define __unix__
#endif

namespace cross_platform {
	std::wstring FromUtf8(const std::string& utf8_string);
	std::string ToUtf8(const std::wstring& wide_string);
	int ToInt(const wchar_t* str);
	double ToDouble(const wchar_t* str);
}
