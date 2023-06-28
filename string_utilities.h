#pragma once
#include <string>

class StringUtilities
{
public:
	static int Int(const std::wstring& s);
	static double Double(const std::wstring& s);
	static std::string ToUtf8(const std::wstring& s);
	static std::wstring FromUtf8(const char* content);
};
