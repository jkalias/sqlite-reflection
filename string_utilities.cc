#include "string_utilities.h"
#include "cross_platform.h"

int StringUtilities::Int(const std::wstring& s) {
	return cross_platform::ToInt(s.data());
}

double StringUtilities::Double(const std::wstring& s) {
	return cross_platform::ToDouble(s.data());
}

std::string StringUtilities::ToMultibyte(const std::wstring& s) {
	return cross_platform::ToUtf8(s);
}

std::wstring StringUtilities::FromMultibyte(const char* content) {
	if (content == nullptr) {
		return L"";
	}

	size_t string_length = 0;
	while (content[string_length] != '\0') {
		string_length++;
	}

	return cross_platform::FromUtf8(content);
}
