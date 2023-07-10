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

#pragma once
#include "reflection_export.h"

#include <string>
#include <vector>
#include <ctime>

namespace sqlite_reflection {
	/// Some useful string utility functions converting back and forth
	/// between strings and concrete types used by SQLite
	class REFLECTION_EXPORT StringUtilities
	{
	public:
		static int64_t ToInt(const std::wstring& s);
		static std::string FromInt(int64_t value);

		static double ToDouble(const std::wstring& s);
		static std::string FromDouble(double value);

		static std::string ToUtf8(const std::wstring& wide_string);
		static std::wstring FromUtf8(const char* utf8_string);

		static std::time_t ToTime(const std::wstring& utc_iso_8601_string);
		static std::string FromTime(const std::time_t& time);

		static std::string Join(const std::vector<std::string>& list, const std::string& separator);
		static std::string Join(const std::vector<std::string>& list, char c);
	};
}
