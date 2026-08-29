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

#include "internal/string_utilities.h"

#include <numeric>

#include "internal/unicode_transcoder.h"

using namespace sqlite_reflection;

namespace {
// Copies a string of code units into a string of a different unit type, one unit at a time. The
// casts are value-preserving in the directions used below, except for a negative wchar_t on a
// platform where it is signed, which widens to a value above U+10FFFF and is then rejected by
// the validation in UnicodeTranscoder.
template <typename To, typename From>
To cast_each(const From& source) {
    To result;
    result.reserve(source.size());
    for (size_t i = 0; i < source.size(); ++i) {
        result.push_back(static_cast<typename To::value_type>(source[i]));
    }
    return result;
}

// wchar_t holds UTF-16 code units where it is 16 bits wide (Windows) and Unicode code points
// where it is 32 bits wide (Linux, macOS). Selecting the path by width is what keeps text above
// U+FFFF identical across platforms; a single facet cannot serve both, which is the defect this
// replaced. See internal/unicode_transcoder.h.
template <size_t WideCharSize>
struct WideCodec;

template <>
struct WideCodec<2> {
    static std::u32string ToCodePoints(const std::wstring& wide_string) {
        const auto code_units = cast_each<std::u16string>(wide_string);
        return UnicodeTranscoder::CodePointsFromUtf16(code_units.data(), code_units.size());
    }

    static std::wstring FromCodePoints(const std::u32string& code_points) {
        const auto code_units = UnicodeTranscoder::Utf16FromCodePoints(code_points.data(), code_points.size());
        return cast_each<std::wstring>(code_units);
    }
};

template <>
struct WideCodec<4> {
    static std::u32string ToCodePoints(const std::wstring& wide_string) {
        return cast_each<std::u32string>(wide_string);
    }

    static std::wstring FromCodePoints(const std::u32string& code_points) {
        return cast_each<std::wstring>(code_points);
    }
};

// Only the two widths above are implemented; without this the failure is an incomplete-type
// error inside the typedef rather than a statement about portability.
static_assert(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4, "sqlite_reflection supports wchar_t of 16 or 32 bits only");

typedef WideCodec<sizeof(wchar_t)> PlatformWideCodec;
}  // namespace

std::string StringUtilities::FromInt(int64_t value) {
    return std::to_string(value);
}

std::string StringUtilities::FromDouble(double value) {
    auto textual_representation = std::to_string(value);
    if (textual_representation.find('.') != std::string::npos) {
        while (!textual_representation.empty() && textual_representation[textual_representation.length() - 1] == '0') {
            textual_representation.erase(textual_representation.end() - 1);
        }
    }
    return textual_representation;
}

std::string StringUtilities::ToUtf8(const std::wstring& wide_string) {
    const auto code_points = PlatformWideCodec::ToCodePoints(wide_string);
    return UnicodeTranscoder::Utf8FromCodePoints(code_points.data(), code_points.size());
}

std::wstring StringUtilities::FromUtf8(const char* utf8_string, size_t byte_count) {
    const auto code_points = UnicodeTranscoder::CodePointsFromUtf8(utf8_string, byte_count);
    return PlatformWideCodec::FromCodePoints(code_points);
}

std::string StringUtilities::Join(const std::vector<std::string>& list, const std::string& separator) {
    const auto size = list.size();
    if (size == 0) {
        return "";
    }

    if (size == 1) {
        return list[0];
    }

    return std::accumulate(list.begin() + 1, list.end(), list[0],
                           [&](const std::string& a, const std::string& b) { return a + separator + b; });
}
