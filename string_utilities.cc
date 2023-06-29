#include "string_utilities.h"

#include <codecvt>

int StringUtilities::Int(const std::wstring& s) {
	int result = 0;
	try {
		result = std::stoi(s);
	}
	catch (...) { }
	return result;
}

double StringUtilities::Double(const std::wstring& s) {
	double result = 0.0;
	try {
		result = std::stod(s);
	}
	catch (...) { }
	return result;
}

std::string StringUtilities::ToUtf8(const std::wstring& wide_string) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	auto utf8_string = converter.to_bytes(wide_string.data());
	return utf8_string;
}

std::wstring StringUtilities::FromUtf8(const char* utf8_string) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	auto wide_string = converter.from_bytes(utf8_string);
	return wide_string;
}
