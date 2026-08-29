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

#pragma once
#include <cstddef>
#include <string>

#include "reflection_export.h"

namespace sqlite_reflection {
/// Unicode transcoding that does not depend on the platform width of wchar_t.
///
/// The library previously converted text with std::wstring_convert and
/// std::codecvt_utf8<wchar_t>. That facet transcodes UTF-8 to and from a single code unit per
/// code point, so where wchar_t is 16 bits (Windows) it silently mishandled everything above
/// U+FFFF: encoding produced CESU-8 (each surrogate half encoded separately, U+1F600 became
/// ED A0 BD ED B8 80 rather than F0 9F 98 80) and decoding truncated the code point to its low
/// 16 bits (U+1F600 became U+F600, U+10000 became NUL). The same code was correct where
/// wchar_t is 32 bits, so identical data behaved differently per platform. <codecvt> is also
/// deprecated since C++17 with no standard replacement, and this library's baseline is C++11.
///
/// Conversion therefore pivots on Unicode code points here, and the wchar_t adapters in
/// StringUtilities select the UTF-16 or UTF-32 path by the width of wchar_t.
///
/// Every function validates its input and throws std::range_error on anything malformed -
/// matching the behavior std::wstring_convert had for ill-formed sequences. Rejected are
/// overlong encodings, truncated sequences, stray or invalid lead and continuation bytes,
/// surrogate code points encoded directly (CESU-8), unpaired UTF-16 surrogates, and anything
/// above U+10FFFF.
///
/// Note that rejecting CESU-8 means text written by a pre-fix build of this library, where
/// wchar_t is 16 bits, no longer reads back. That data was stored ill-formed; because encoding
/// and decoding were wrong in the same way it used to survive a round trip on Windows, so the
/// break surfaces only now. See the text encoding section of README.md.
class REFLECTION_EXPORT UnicodeTranscoder {
public:
    /// Encodes Unicode code points as UTF-8
    static std::string Utf8FromCodePoints(const char32_t* code_points, size_t count);

    /// Decodes UTF-8 into Unicode code points
    static std::u32string CodePointsFromUtf8(const char* utf8_string, size_t byte_count);

    /// Encodes Unicode code points as UTF-16 code units, using a surrogate pair above U+FFFF
    static std::u16string Utf16FromCodePoints(const char32_t* code_points, size_t count);

    /// Decodes UTF-16 code units, combining surrogate pairs, into Unicode code points
    static std::u32string CodePointsFromUtf16(const char16_t* utf16_string, size_t count);
};
}  // namespace sqlite_reflection
