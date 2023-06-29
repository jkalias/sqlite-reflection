#include "string_utilities.h"

#include <codecvt>

int StringUtilities::Int(const std::wstring& s) {
	return std::stoi(s);
}

double StringUtilities::Double(const std::wstring& s) {
	return std::stod(s);
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
