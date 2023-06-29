#pragma once
#include <string>
#include <vector>

class StringUtilities
{
public:
	static int Int(const std::wstring& s);
	static double Double(const std::wstring& s);
	static std::string ToUtf8(const std::wstring& wide_string);
	static std::wstring FromUtf8(const char* utf8_string);
	static std::string Join(const std::vector<std::string>& list, const std::string& separator);
	static std::string Join(const std::vector<std::string>& list, char c);
};
