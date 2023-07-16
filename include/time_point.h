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

typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> sys_seconds;

namespace sqlite_reflection {
/// A wrapper for expressing points in time based on system time.
/// The epoch is by convention the Unix epoch of 1 January 1970 at midnight (1970-01-01T00:00:00Z)
/// System time is a very good approximation of UTC time, the only difference being the management of leap seconds
/// (system time ignores leap seconds)
/// https://en.wikipedia.org/wiki/Coordinated_Universal_Time
	class REFLECTION_EXPORT TimePoint
	{
	public:
        /// Defaults to start of system time (Unix epoch)
		TimePoint();
        
        // Creates a time representation based on the elapsed seconds from the Unix epoch.
        // Negative valules represent time points before 1970-01-01T00:00:00Z,
        // whereas positive values after 1970-01-01 00:00:00 UTC
		explicit TimePoint(const int64_t& seconds_since_unix_epoch);
        
        // Creates a time representation based on the elapsed seconds from the Unix epoch.
        // Negative valules represent time points before 1970-01-01T00:00:00Z,
        // whereas positive values after 1970-01-01 00:00:00 UTC
		explicit TimePoint(const sys_seconds& time_since_unix_epoch);
        
        /// Creates a time point instance from its equivalent ISO 8601 UTC format
		static TimePoint FromSystemTime(const std::wstring& iso_8601_string);
        
        /// Returns a string representation of this  instance, expressed in ISO 8601 UTC format
        /// https://en.wikipedia.org/wiki/ISO_8601
		std::wstring SystemTime() const;

	private:
        sys_seconds time_stamp_;
	};
}
