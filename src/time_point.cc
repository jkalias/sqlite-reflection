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

#include "time_point.h"
#include "internal/string_utilities.h"

#include <sstream>
#include <iomanip>

using namespace sqlite_reflection;
using namespace std::chrono;

#if HAS_LEGACY_CHRONO
#include "internal/date.h"
using namespace date;
#else
#define make_time hh_mm_ss
#endif

static std::wstring iso_format = L"%FT%T";

TimePoint::TimePoint() {}

TimePoint::TimePoint(const int64_t& seconds_since_unix_epoch)
	: time_stamp_(seconds(seconds_since_unix_epoch)) {}

TimePoint::TimePoint(const sys_seconds& time_since_unix_epoch)
	: time_stamp_(time_since_unix_epoch) {}

TimePoint TimePoint::FromSystemTime(const std::wstring& iso_8601_string) {
	std::wistringstream in{iso_8601_string};
	sys_seconds time_stamp;
	in >> parse(iso_format, time_stamp);
	return TimePoint(time_stamp);
}

std::wstring TimePoint::SystemTime() const {
	const auto tp = floor<days>(time_stamp_);
	std::stringstream ss;
	ss << tp << "T" << make_time(time_stamp_ - tp) << " UTC";
	return StringUtilities::FromUtf8(ss.str().c_str());
}
