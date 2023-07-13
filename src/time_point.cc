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

#if HAS_LEGACY_CHRONO
#include "internal/date.h"
#endif
#include "internal/string_utilities.h"

#include <sstream>
#include <iomanip>

using namespace sqlite_reflection;
using namespace std::chrono;

static std::wstring iso_format = L"%FT%T";

TimePoint::TimePoint()
: time_stamp_() {}

TimePoint::TimePoint(const sys_seconds& time_since_unix_epoch)
: time_stamp_(time_since_unix_epoch) {}

TimePoint TimePoint::FromSystemTime(const std::wstring &iso_8601_string) {
    std::wistringstream in{iso_8601_string};
    sys_seconds time_stamp;
#if HAS_LEGACY_CHRONO
    in >> date::parse(iso_format, time_stamp);
#else
    in >> parse(iso_format, tp);
#endif
    return TimePoint(time_stamp);
}

std::wstring TimePoint::SystemTime() const {
    auto now = std::chrono::system_clock::now();
    auto today = date::floor<days>(now);
    
    std::stringstream ss;
    ss << today << ' ' << date::make_time(now - today) << " UTC";
}
