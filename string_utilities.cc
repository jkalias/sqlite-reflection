#include "string_utilities.h"
#include "cross_platform.h"

int StringUtilities::Int(const std::wstring& s) {
	return cross_platform::ToInt(s.data());
}

double StringUtilities::Double(const std::wstring& s) {
	return cross_platform::ToDouble(s.data());
}

std::string StringUtilities::ToUtf8(const std::wstring& s) {
	return cross_platform::ToUtf8(s);
}

std::wstring StringUtilities::FromUtf8(const char* content) {
	return cross_platform::FromUtf8(content);
}
