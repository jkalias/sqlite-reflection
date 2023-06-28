#pragma once
#include <string>

class StringUtilities
{
public:
	static int Int(const std::wstring& s);
	static double Double(const std::wstring& s);
	static std::string ToMultibyte(const std::wstring& s);
	static std::wstring FromMultibyte(const char* content);
};
