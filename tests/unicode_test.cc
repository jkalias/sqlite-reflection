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

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "internal/string_utilities.h"

using namespace sqlite_reflection;

// Every wide literal in this file is written with universal-character escapes rather than
// literal non-ASCII source bytes. MSVC is not passed /utf-8, so it decodes non-ASCII source
// bytes in the system codepage; a test written with literal characters would compare a
// mangled literal against the same mangled literal and pass while proving nothing. Escapes
// are charset-independent, and the expected UTF-8 is asserted as exact bytes rather than by
// round-tripping, so a conversion that is wrong in both directions cannot pass either.

namespace {
// A code point is encoded as a surrogate pair in a 16-bit wchar_t (Windows) and as a single
// code unit in a 32-bit wchar_t (Linux, macOS). The expected UTF-8 is identical on both.
const char* const kEmojiUtf8 = "\xF0\x9F\x98\x80";               // U+1F600 GRINNING FACE
const char* const kFirstSupplementaryUtf8 = "\xF0\x90\x80\x80";  // U+10000, first non-BMP
const char* const kLastCodePointUtf8 = "\xF4\x8F\xBF\xBF";       // U+10FFFF, highest valid
const char* const kLastBmpUtf8 = "\xEF\xBF\xBF";                 // U+FFFF, last BMP code point

std::wstring FromUtf8(const std::string& utf8) {
    return StringUtilities::FromUtf8(utf8.data(), utf8.size());
}
}  // namespace

TEST(UnicodeTest, EncodesAsciiToExactBytes) {
    EXPECT_EQ(std::string("Appleseed"), StringUtilities::ToUtf8(L"Appleseed"));
}

TEST(UnicodeTest, EncodesEmptyString) {
    EXPECT_EQ(std::string(), StringUtilities::ToUtf8(std::wstring()));
}

TEST(UnicodeTest, EncodesBmpToExactBytes) {
    // U+03C0 GREEK SMALL LETTER PI, U+03B1 GREEK SMALL LETTER ALPHA: two-byte sequences.
    EXPECT_EQ(std::string("\xCF\x80\xCE\xB1"), StringUtilities::ToUtf8(L"\u03C0\u03B1"));
}

TEST(UnicodeTest, EncodesLastBmpCodePointToExactBytes) {
    EXPECT_EQ(std::string(kLastBmpUtf8), StringUtilities::ToUtf8(L"\uFFFF"));
}

TEST(UnicodeTest, EncodesFirstSupplementaryCodePointToExactBytes) {
    // U+10000 is the low side of the surrogate boundary: the first code point that needs a
    // surrogate pair in UTF-16 and a four-byte UTF-8 sequence.
    EXPECT_EQ(std::string(kFirstSupplementaryUtf8), StringUtilities::ToUtf8(L"\U00010000"));
}

TEST(UnicodeTest, EncodesEmojiToExactBytes) {
    EXPECT_EQ(std::string(kEmojiUtf8), StringUtilities::ToUtf8(L"\U0001F600"));
}

TEST(UnicodeTest, EncodesLastValidCodePointToExactBytes) {
    EXPECT_EQ(std::string(kLastCodePointUtf8), StringUtilities::ToUtf8(L"\U0010FFFF"));
}

TEST(UnicodeTest, EncodesSupplementaryCodePointMixedWithAscii) {
    EXPECT_EQ(std::string("a") + kEmojiUtf8 + "b", StringUtilities::ToUtf8(L"a\U0001F600b"));
}

TEST(UnicodeTest, DecodesAscii) {
    EXPECT_EQ(std::wstring(L"Appleseed"), FromUtf8("Appleseed"));
}

TEST(UnicodeTest, DecodesEmptyString) {
    EXPECT_EQ(std::wstring(), FromUtf8(std::string()));
}

TEST(UnicodeTest, DecodesBmp) {
    EXPECT_EQ(std::wstring(L"\u03C0\u03B1"), FromUtf8("\xCF\x80\xCE\xB1"));
}

TEST(UnicodeTest, DecodesLastBmpCodePoint) {
    EXPECT_EQ(std::wstring(L"\uFFFF"), FromUtf8(kLastBmpUtf8));
}

TEST(UnicodeTest, DecodesFirstSupplementaryCodePoint) {
    EXPECT_EQ(std::wstring(L"\U00010000"), FromUtf8(kFirstSupplementaryUtf8));
}

TEST(UnicodeTest, DecodesEmoji) {
    EXPECT_EQ(std::wstring(L"\U0001F600"), FromUtf8(kEmojiUtf8));
}

TEST(UnicodeTest, DecodesLastValidCodePoint) {
    EXPECT_EQ(std::wstring(L"\U0010FFFF"), FromUtf8(kLastCodePointUtf8));
}

TEST(UnicodeTest, RoundTripsSupplementaryCodePoints) {
    const std::wstring original = L"\U0001F600\U00010000\U0010FFFF";
    EXPECT_EQ(original, FromUtf8(StringUtilities::ToUtf8(original)));
}

TEST(UnicodeTest, SupplementaryCodePointSurvivesLengthAndContent) {
    // Guards against a conversion that silently drops or truncates the pair rather than
    // producing a wrong-but-present result.
    const std::wstring decoded = FromUtf8(kEmojiUtf8);
    EXPECT_FALSE(decoded.empty());
    EXPECT_EQ(std::wstring(L"\U0001F600"), decoded);
}

// ---------------------------------------------------------------------------
// Code-point core, exercised directly so both the UTF-16 and UTF-32 paths are covered on every
// platform rather than only the one matching this machine's wchar_t width. The UTF-16 path is
// where the replaced <codecvt> facet was wrong, and on Linux and macOS it is unreachable
// through the wstring API.
// ---------------------------------------------------------------------------

namespace {
std::u32string CodePoints(const std::string& utf8) {
    return UnicodeTranscoder::CodePointsFromUtf8(utf8.data(), utf8.size());
}

std::string Utf8From(const std::u32string& code_points) {
    return UnicodeTranscoder::Utf8FromCodePoints(code_points.data(), code_points.size());
}

std::u16string Utf16From(const std::u32string& code_points) {
    return UnicodeTranscoder::Utf16FromCodePoints(code_points.data(), code_points.size());
}

std::u32string FromUtf16(const std::u16string& code_units) {
    return UnicodeTranscoder::CodePointsFromUtf16(code_units.data(), code_units.size());
}
}  // namespace

TEST(UnicodeCoreTest, EncodesSupplementaryCodePointAsSurrogatePair) {
    const std::u32string emoji(1, 0x1F600);
    const std::u16string units = Utf16From(emoji);
    ASSERT_EQ(static_cast<size_t>(2), units.size());
    EXPECT_EQ(0xD83D, units[0]);
    EXPECT_EQ(0xDE00, units[1]);
}

TEST(UnicodeCoreTest, CombinesSurrogatePairIntoSupplementaryCodePoint) {
    std::u16string units;
    units.push_back(0xD83D);
    units.push_back(0xDE00);
    const std::u32string code_points = FromUtf16(units);
    ASSERT_EQ(static_cast<size_t>(1), code_points.size());
    EXPECT_EQ(0x1F600u, static_cast<unsigned>(code_points[0]));
}

TEST(UnicodeCoreTest, RoundTripsSurrogateBoundaryThroughUtf16) {
    std::u32string code_points;
    code_points.push_back(0xFFFF);   // last BMP
    code_points.push_back(0x10000);  // first supplementary
    code_points.push_back(0x10FFFF);
    EXPECT_EQ(code_points, FromUtf16(Utf16From(code_points)));
}

TEST(UnicodeCoreTest, EncodesSupplementaryCodePointAsFourUtf8Bytes) {
    EXPECT_EQ(std::string(kEmojiUtf8), Utf8From(std::u32string(1, 0x1F600)));
}

TEST(UnicodeCoreTest, RejectsUnpairedHighSurrogateInUtf16) {
    EXPECT_THROW(FromUtf16(std::u16string(1, 0xD83D)), std::range_error);
}

TEST(UnicodeCoreTest, RejectsUnpairedLowSurrogateInUtf16) {
    EXPECT_THROW(FromUtf16(std::u16string(1, 0xDE00)), std::range_error);
}

TEST(UnicodeCoreTest, RejectsHighSurrogateFollowedByNonSurrogate) {
    std::u16string units;
    units.push_back(0xD83D);
    units.push_back(0x0041);
    EXPECT_THROW(FromUtf16(units), std::range_error);
}

TEST(UnicodeCoreTest, RejectsSurrogateCodePointOnEncode) {
    EXPECT_THROW(Utf8From(std::u32string(1, 0xD800)), std::range_error);
    EXPECT_THROW(Utf16From(std::u32string(1, 0xDFFF)), std::range_error);
}

TEST(UnicodeCoreTest, RejectsCodePointAboveUnicodeMaximumOnEncode) {
    EXPECT_THROW(Utf8From(std::u32string(1, 0x110000)), std::range_error);
}

TEST(UnicodeCoreTest, RejectsInvalidLeadByte) {
    EXPECT_THROW(CodePoints("\xFF"), std::range_error);              // 0xFF is never a lead byte
    EXPECT_THROW(CodePoints("\xC0\xAF"), std::range_error);          // overlong ASCII
    EXPECT_THROW(CodePoints("\x80"), std::range_error);              // stray continuation byte
    EXPECT_THROW(CodePoints("\xF5\x80\x80\x80"), std::range_error);  // above U+10FFFF
}

TEST(UnicodeCoreTest, RejectsTruncatedSequence) {
    EXPECT_THROW(CodePoints("\xF0\x9F\x98"), std::range_error);  // emoji missing its last byte
    EXPECT_THROW(CodePoints("\xCF"), std::range_error);          // two-byte lead, nothing follows
}

TEST(UnicodeCoreTest, RejectsMissingContinuationByte) {
    EXPECT_THROW(CodePoints("\xF0\x9F"
                            "A"
                            "\x80"),
                 std::range_error);
    EXPECT_THROW(CodePoints("\xCF"
                            "A"),
                 std::range_error);
}

TEST(UnicodeCoreTest, RejectsOverlongEncoding) {
    EXPECT_THROW(CodePoints("\xE0\x80\xAF"), std::range_error);      // overlong three-byte
    EXPECT_THROW(CodePoints("\xF0\x80\x80\xAF"), std::range_error);  // overlong four-byte
}

TEST(UnicodeCoreTest, RejectsSurrogateEncodedInUtf8) {
    // CESU-8: exactly what the replaced facet emitted for supplementary code points, so it must
    // not be silently accepted on the way back in either.
    EXPECT_THROW(CodePoints("\xED\xA0\xBD"), std::range_error);
    EXPECT_THROW(CodePoints("\xED\xA0\xBD\xED\xB8\x80"), std::range_error);
}

TEST(UnicodeCoreTest, RejectsCodePointAboveUnicodeMaximumOnDecode) {
    EXPECT_THROW(CodePoints("\xF4\x90\x80\x80"), std::range_error);  // U+110000
}
