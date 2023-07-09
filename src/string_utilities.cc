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

#include "internal/string_utilities.h"

#include <codecvt>
#include <numeric>
#include <time.h>
#include <sstream>
#include <iomanip>
#ifndef _MSC_VER
#include <locale>
#else
#define timegm _mkgmtime
#endif

using namespace sqlite_reflection;

int64_t StringUtilities::ToInt(const std::wstring& s) {
    int result = 0;
    try {
        result = std::stoi(s);
    }
    catch (...) {}
    return result;
}

std::string StringUtilities::FromInt(int64_t value) {
    return std::to_string(value);
}

double StringUtilities::ToDouble(const std::wstring& s) {
    double result = 0.0;
    try {
        result = std::stod(s);
    }
    catch (...) {}
    return result;
}

std::string StringUtilities::FromDouble(double value) {
    auto textual_representation = std::to_string(value);
    if (textual_representation.find(".") != std::string::npos) {
        while (textual_representation.length() > 0 && textual_representation[textual_representation.length() - 1] == '0') {
            textual_representation.erase(textual_representation.end() - 1);
        }
    }
    return textual_representation;
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

std::time_t StringUtilities::ToTime(const std::wstring& utc_iso_8601_string) {
    int year, month, day, hour, minute, second;
    std::tm time_tm{};
    
    const auto date_string = ToUtf8(utc_iso_8601_string);
    sscanf(date_string.c_str(), "%d-%d-%dT%d:%d:%dZ", &year, &month, &day, &hour, &minute, &second);
    
    time_tm.tm_year = year - 1900;
    time_tm.tm_mon = month - 1;
    time_tm.tm_mday = day;
    time_tm.tm_hour = hour;
    time_tm.tm_min = minute;
    time_tm.tm_sec = second;
    time_tm.tm_isdst = -1;
    
    auto time_point = timegm(&time_tm);
    return time_point;
}

std::string StringUtilities::FromTime(const std::time_t& time) {
    auto ptm = gmtime(&time);
    std::stringstream sstr;
    sstr << std::put_time(ptm, "%FT%TZ");
    return sstr.str();
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
