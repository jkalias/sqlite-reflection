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

#include "internal/unicode.h"

#include <stdexcept>

using namespace sqlite_reflection;

namespace {
const char32_t kMaxCodePoint = 0x10FFFF;
const char32_t kFirstSupplementary = 0x10000;

const char16_t kFirstHighSurrogate = 0xD800;
const char16_t kLastHighSurrogate = 0xDBFF;
const char16_t kFirstLowSurrogate = 0xDC00;
const char16_t kLastLowSurrogate = 0xDFFF;

[[noreturn]] void throw_invalid_unicode(const char* reason) {
    throw std::range_error(std::string("invalid Unicode input: ") + reason);
}

// The surrogate range is exactly the first high through the last low surrogate; the char16_t
// operands widen to char32_t for the comparison.
bool is_surrogate(char32_t code_point) {
    return code_point >= kFirstHighSurrogate && code_point <= kLastLowSurrogate;
}

// A code point that no valid encoding may carry: surrogates are reserved for UTF-16 pairing
// and must never appear on their own, and nothing above U+10FFFF exists.
void reject_if_not_scalar_value(char32_t code_point) {
    if (is_surrogate(code_point)) {
        throw_invalid_unicode("surrogate code point");
    }
    if (code_point > kMaxCodePoint) {
        throw_invalid_unicode("code point above U+10FFFF");
    }
}

bool is_continuation(unsigned char byte) {
    return (byte & 0xC0) == 0x80;
}

// Length of the sequence introduced by this lead byte, or 0 if it cannot introduce one.
// 0x80-0xBF are continuation bytes, 0xC0-0xC1 could only encode overlong ASCII, and 0xF5-0xFF
// could only encode code points above U+10FFFF, so none of them are valid leads.
size_t sequence_length(unsigned char lead) {
    if (lead <= 0x7F) {
        return 1;
    }
    if (lead >= 0xC2 && lead <= 0xDF) {
        return 2;
    }
    if (lead >= 0xE0 && lead <= 0xEF) {
        return 3;
    }
    if (lead >= 0xF0 && lead <= 0xF4) {
        return 4;
    }
    return 0;
}

// Ranges of the second byte that would otherwise admit an overlong encoding, a surrogate, or a
// code point above U+10FFFF. The lead byte alone cannot exclude these (Unicode Table 3-7).
void reject_if_invalid_second_byte(unsigned char lead, unsigned char second) {
    if (lead == 0xE0 && second < 0xA0) {
        throw_invalid_unicode("overlong three-byte sequence");
    }
    if (lead == 0xED && second > 0x9F) {
        throw_invalid_unicode(
            "surrogate code point encoded in UTF-8 (CESU-8), as written by a pre-fix build of this "
            "library where wchar_t is 16 bits");
    }
    if (lead == 0xF0 && second < 0x90) {
        throw_invalid_unicode("overlong four-byte sequence");
    }
    if (lead == 0xF4 && second > 0x8F) {
        throw_invalid_unicode("code point above U+10FFFF");
    }
}
}  // namespace

std::string UnicodeTranscoder::Utf8FromCodePoints(const char32_t* code_points, size_t count) {
    std::string utf8_string;
    utf8_string.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const char32_t code_point = code_points[i];
        reject_if_not_scalar_value(code_point);

        if (code_point <= 0x7F) {
            utf8_string.push_back(static_cast<char>(code_point));
        } else if (code_point <= 0x7FF) {
            utf8_string.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
            utf8_string.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
        } else if (code_point <= 0xFFFF) {
            utf8_string.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
            utf8_string.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
            utf8_string.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
        } else {
            utf8_string.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
            utf8_string.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
            utf8_string.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
            utf8_string.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
        }
    }

    return utf8_string;
}

std::u32string UnicodeTranscoder::CodePointsFromUtf8(const char* utf8_string, size_t byte_count) {
    std::u32string code_points;
    code_points.reserve(byte_count);

    size_t i = 0;
    while (i < byte_count) {
        const auto lead = static_cast<unsigned char>(utf8_string[i]);
        const size_t length = sequence_length(lead);
        if (length == 0) {
            throw_invalid_unicode("byte cannot start a UTF-8 sequence");
        }
        if (i + length > byte_count) {
            throw_invalid_unicode("truncated UTF-8 sequence");
        }

        if (length == 1) {
            code_points.push_back(static_cast<char32_t>(lead));
            i += 1;
            continue;
        }

        const auto second = static_cast<unsigned char>(utf8_string[i + 1]);
        if (!is_continuation(second)) {
            throw_invalid_unicode("missing UTF-8 continuation byte");
        }
        reject_if_invalid_second_byte(lead, second);

        char32_t code_point = 0;
        if (length == 2) {
            code_point = static_cast<char32_t>(lead & 0x1F);
        } else if (length == 3) {
            code_point = static_cast<char32_t>(lead & 0x0F);
        } else {
            code_point = static_cast<char32_t>(lead & 0x07);
        }

        for (size_t j = 1; j < length; ++j) {
            const auto byte = static_cast<unsigned char>(utf8_string[i + j]);
            if (!is_continuation(byte)) {
                throw_invalid_unicode("missing UTF-8 continuation byte");
            }
            code_point = (code_point << 6) | static_cast<char32_t>(byte & 0x3F);
        }

        reject_if_not_scalar_value(code_point);
        code_points.push_back(code_point);
        i += length;
    }

    return code_points;
}

std::u16string UnicodeTranscoder::Utf16FromCodePoints(const char32_t* code_points, size_t count) {
    std::u16string utf16_string;
    utf16_string.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const char32_t code_point = code_points[i];
        reject_if_not_scalar_value(code_point);

        if (code_point < kFirstSupplementary) {
            utf16_string.push_back(static_cast<char16_t>(code_point));
            continue;
        }

        const char32_t offset = code_point - kFirstSupplementary;
        utf16_string.push_back(static_cast<char16_t>(kFirstHighSurrogate + (offset >> 10)));
        utf16_string.push_back(static_cast<char16_t>(kFirstLowSurrogate + (offset & 0x3FF)));
    }

    return utf16_string;
}

std::u32string UnicodeTranscoder::CodePointsFromUtf16(const char16_t* utf16_string, size_t count) {
    std::u32string code_points;
    code_points.reserve(count);

    size_t i = 0;
    while (i < count) {
        const char16_t unit = utf16_string[i];

        if (unit >= kFirstLowSurrogate && unit <= kLastLowSurrogate) {
            throw_invalid_unicode("unpaired low surrogate");
        }

        if (unit < kFirstHighSurrogate || unit > kLastHighSurrogate) {
            code_points.push_back(static_cast<char32_t>(unit));
            i += 1;
            continue;
        }

        if (i + 1 >= count) {
            throw_invalid_unicode("high surrogate at end of input");
        }

        const char16_t low = utf16_string[i + 1];
        if (low < kFirstLowSurrogate || low > kLastLowSurrogate) {
            throw_invalid_unicode("high surrogate not followed by a low surrogate");
        }

        const char32_t high_bits = static_cast<char32_t>(unit - kFirstHighSurrogate) << 10;
        const char32_t low_bits = static_cast<char32_t>(low - kFirstLowSurrogate);
        code_points.push_back(kFirstSupplementary + high_bits + low_bits);
        i += 2;
    }

    return code_points;
}
