// MIT License
//
// Copyright (c) 2023 Ioannis Kaliakatsos
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "string_utilities.h"

#include <codecvt>
#include <numeric>

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

std::string StringUtilities::Join(const std::vector<std::string>& list, const std::string& separator) {
	const auto size = list.size();
	if (size == 0) {
		return "";
	}

	if (size == 1) {
		return list[0];
	}

	return std::accumulate(
		list.begin() + 1,
		list.end(),
		list[0],
		[&](const std::string& a, const std::string& b){
			return a + separator + b;
		});
}

std::string StringUtilities::Join(const std::vector<std::string>& list, char c) {
	return Join(list, std::string(1, c));
}
