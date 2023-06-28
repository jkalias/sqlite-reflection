#include "cross_platform.h"

#include <codecvt>
#ifdef __unix__
#include <locale>
#else
#include <windows.h>
#endif

namespace cross_platform {
	std::wstring FromUtf8(const std::string& utf8_string) {
		std::wstring_convert<std::codecvt_utf8<wchar_t> > converter;
		auto wide_string = converter.from_bytes(utf8_string.data());
		return wide_string;
	}

	std::string ToUtf8(const std::wstring& wide_string) {
		std::wstring_convert<std::codecvt_utf8<wchar_t> > converter;
		auto utf8_string = converter.to_bytes(wide_string.data());
		return utf8_string;
	}

	int ToInt(const wchar_t* str) {
#ifdef __unix__
		std::string sstr = ToUtf8(str);
		auto tmp = sstr.data();
		return atoi(tmp);
#else
		return _wtoi(str);
#endif
	}

	double ToDouble(const wchar_t* str) {
#ifdef __unix__
		std::string sstr = ToUtf8(str);
		auto tmp = sstr.data();
		return atof(tmp);
#else
		return _wtof(str);
#endif
	}
}
