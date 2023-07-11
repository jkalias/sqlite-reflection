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

#include "internal/gmtime.h"
#include "internal/string_utilities.h"

#include <sstream>
#include <iomanip>

using namespace sqlite_reflection;

TimePoint::TimePoint()
	: time_point_(0) {}

TimePoint::TimePoint(const time64_t& time_since_unix_epoch)
	: time_point_(time_since_unix_epoch) {}

TimePoint TimePoint::FromUtcTime(const std::wstring& utc_iso_8601_string) {
	int year, month, day, hour, minute, second;
	std::tm time_tm{};

	const auto date_string = StringUtilities::ToUtf8(utc_iso_8601_string);
	sscanf(date_string.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second);

	time_tm.tm_year = year - 1900;
	time_tm.tm_mon = month - 1;
	time_tm.tm_mday = day;
	time_tm.tm_hour = hour;
	time_tm.tm_min = minute;
	time_tm.tm_sec = second;
	time_tm.tm_isdst = -1;

	const auto time_point = MkTime64(&time_tm);
	return TimePoint(time_point);
}

std::wstring TimePoint::UtcTimestamp() const {
	tm ptm{};
	GmTime64R(&time_point_, &ptm);
	std::stringstream sstr;
	sstr << std::put_time(&ptm, "%FT%T");
	return StringUtilities::FromUtf8(sstr.str().data());
}

std::wstring TimePoint::LocalTimestamp() const {
	tm ptm{};
	Localtime64R(&time_point_, &ptm);
	std::stringstream sstr;
	sstr << std::put_time(&ptm, "%FT%T");
	return StringUtilities::FromUtf8(sstr.str().data());
}
