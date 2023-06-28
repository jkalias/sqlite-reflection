#include "cross_platform.h"
#include <vector>
#ifdef __unix__
//#include <iconv.h>
//#include <dirent.h>
//#include <sys/sendfile.h>  // sendfile
//#include <fcntl.h>         // open
#else
#include <tchar.h>
#include <windows.h>
#include <psapi.h>
#endif

namespace cross_platform {
	std::wstring FromUtf8(const std::string& utf8_string) {
#ifdef __unix__
		setlocale(LC_ALL, "");
		const char* str = utf8string.c_str();
		wchar_t buffer[1024];
		int ret;
		ret = mbstowcs(buffer, str, 1024);
		if (ret == 1023) {
			// error in conversion
			buffer[1023] = L'\0';
		}
		return std::wstring(buffer);
#else
		const int wide_size = ::MultiByteToWideChar(CP_UTF8, 0, utf8_string.c_str(), -1, nullptr, 0);
		if (wide_size == 0) {
			const int err = GetLastError();
			std::string str = "Error " + std::to_string(err) + " in conversion of UTF8 string: \n" + utf8_string;
			throw std::exception(str.c_str());
		}
		std::vector<wchar_t> result(wide_size);
		const int convresult = ::MultiByteToWideChar(CP_UTF8, 0, utf8_string.c_str(), -1, &result[0], wide_size);
		if (convresult != wide_size) {
			throw std::exception("FromUtf8: wides_ize error");
		}
		return std::wstring(&result[0]);
#endif
	}

	std::string ToUtf8(const std::wstring& wide_string) {
#ifdef __unix__
		setlocale(LC_ALL, "");
		const wchar_t* str = wide_string.c_str();
		char buffer[1024];
		int ret;
		ret = wcstombs(buffer, str, 1024);
		if (ret == 1023) {
			// error in conversion
			buffer[0] = '\0';
		}
		return std::string(buffer);
#else
		const int utf8_size = ::WideCharToMultiByte(CP_UTF8, 0, wide_string.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (utf8_size == 0) {
			throw std::exception("ToUtf8: Error in conversion.");
		}
		std::vector<char> result(utf8_size);
		const int convresult = ::WideCharToMultiByte(CP_UTF8, 0, wide_string.c_str(), -1, &result[0], utf8_size, nullptr, nullptr);
		if (convresult != utf8_size) {
			throw std::exception("ToUtf8: wide_size error");
		}
		return std::string(&result[0]);
#endif
	}

	std::wstring FromWideChar(const wide_char_t* s) {
		std::wstring str;
#ifdef __unix__
		str = L"";
		unsigned int i = 0;
		wchar_t c;
		while (s[i]) {
			c = s[i];
			str += c;
			i++;
		}
#else
		// ReSharper disable once CppCStyleCast
		str = (wchar_t*)s;
#endif
		return str;
	}

	void Assign(std::wstring& wide_string, const wide_char_t* s) {
#ifdef __unix__
		wide_string = L"";
		unsigned int i = 0;
		wchar_t c;
		while (s[i]) {
			c = s[i];
			wide_string += c;
			i++;
		}
#else
		// ReSharper disable once CppCStyleCast
		wide_string = (wchar_t*)s;
#endif
	}

	int ToInt(const wchar_t* str) {
#ifdef __unix__
		std::string sstr = ToUtf8(str);
		char* tmp = sstr.data();
		return atoi(tmp);
#else
		return _wtoi(str);
#endif
	}

	double ToDouble(const wchar_t* str) {
#ifdef __unix__
		std::string sstr = ToUtf8(str);
		char* tmp = sstr.data();
		return atof(tmp);
#else
		return _wtof(str);
#endif
	}
}
