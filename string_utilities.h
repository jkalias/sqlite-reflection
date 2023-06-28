#pragma once
#include <clocale>

#define MAX_BUFFER_SIZE 2048

struct StringUtilities
{
	static void Initialize() {
		std::setlocale(LC_ALL, "");
	}

	static int Int(const std::wstring& s) {
		char mbstr[MAX_BUFFER_SIZE];
		std::wcstombs(mbstr, s.c_str(), MAX_BUFFER_SIZE);
		return atoi(mbstr);
	}

	static double Double(const std::wstring& s) {
		char mbstr[MAX_BUFFER_SIZE];
		std::wcstombs(mbstr, s.c_str(), MAX_BUFFER_SIZE);
		return atof(mbstr);
	}

	static std::string ToMultibyte(const std::wstring& s) {
		char multi_byte_string[MAX_BUFFER_SIZE];
		std::wcstombs(multi_byte_string, s.c_str(), 100);
		return multi_byte_string;
	}

	static std::wstring FromMultibyte(const char* content) {
		if (content == nullptr) {
			return L"";
		}

		size_t string_length = 0;
		while (content[string_length] != '\0') {
			string_length++;
		}

		size_t number_of_copied_bytes;
		const auto dest = new wchar_t[string_length];
		try {
			number_of_copied_bytes = mbstowcs(dest, content, string_length);
		}
		catch (...) {
			delete[] dest;
			return L"";
		}

		if (number_of_copied_bytes == string_length) {
			// it's an ascii string
			std::string ascii(content);
			return std::wstring(ascii.begin(), ascii.end());
		}

		std::wstring unicode_text(dest);
		delete[] dest;
		return unicode_text;
	}
};
