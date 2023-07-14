// MIT License
//
// Copyright (c) 2023 Ioannis Kaliakatsos
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following predicates:
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
#include <chrono>

#if __cplusplus < 201907L
#define HAS_LEGACY_CHRONO
#endif

#ifdef HAS_LEGACY_CHRONO
typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> sys_seconds;
#else
using namespace std::chrono;
#endif

namespace sqlite_reflection {
	class REFLECTION_EXPORT TimePoint
	{
	public:
		TimePoint();
		explicit TimePoint(const int64_t& seconds_since_unix_epoch);
		explicit TimePoint(const sys_seconds& time_since_unix_epoch);
		static TimePoint FromSystemTime(const std::wstring& iso_8601_string);
		//static TimePoint FromLocalTime(const std::wstring& timestamp);

		std::wstring SystemTime() const;
		//std::wstring LocalTimestamp() const;

	private:
        sys_seconds time_stamp_;
	};
}
