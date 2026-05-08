// MIT License
//
// Copyright (c) 2026 Ioannis Kaliakatsos
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

#include <iomanip>
#include <sstream>

#include "internal/string_utilities.h"

// This would not have been at all possible without this amazing library
#include "internal/date.h"

using namespace sqlite_reflection;

#if __cplusplus < 201907L
#define LEGACY_CHRONO
#else
#ifdef _WIN32
#define MODERN_WIN_CHRONO
#else
#define MODERN_UNIX_CHRONO
#endif
#endif

#if defined(LEGACY_CHRONO) || !defined(MODERN_WIN_CHRONO)
using namespace date;
#endif

constexpr wchar_t iso_format[] = L"%FT%TZ";

TimePoint::TimePoint() {}

TimePoint::TimePoint(const int64_t& seconds_since_unix_epoch)
    : time_stamp_(std::chrono::seconds(seconds_since_unix_epoch)) {}

TimePoint::TimePoint(const sys_seconds& time_since_unix_epoch) : time_stamp_(time_since_unix_epoch) {}

TimePoint TimePoint::FromSystemTime(const std::wstring& iso_8601_string) {
    std::wistringstream in{iso_8601_string};
    sys_seconds time_stamp;
    in >> date::parse(iso_format, time_stamp);
    return TimePoint(time_stamp);
}

std::wstring TimePoint::SystemTime() const {
    const auto tp = date::floor<date::days>(time_stamp_);
    const auto ymd = date::year_month_day(tp);
    const auto time = date::make_time(time_stamp_ - tp);

    std::stringstream ss;
    ss << static_cast<int>(ymd.year()) << "-" << std::setfill('0') << std::setw(2) << static_cast<unsigned>(ymd.month())
       << "-" << std::setfill('0') << std::setw(2) << static_cast<unsigned>(ymd.day()) << "T" << std::setfill('0')
       << std::setw(2) << time.hours().count() << ":" << std::setfill('0') << std::setw(2) << time.minutes().count()
       << ":" << std::setfill('0') << std::setw(2) << time.seconds().count() << "Z";
    const auto utf8_string = ss.str();
    return StringUtilities::FromUtf8(utf8_string.data(), utf8_string.size());
}
