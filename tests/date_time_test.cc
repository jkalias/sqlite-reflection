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

#include <gtest/gtest.h>

#include "datetime_container.h"
#include "database.h"

using namespace sqlite_reflection;

class DateTimeTest : public ::testing::Test
{
	void SetUp() override {
		Database::Initialize();
	}

	void TearDown() override {
		Database::Finalize();
	}
};

std::wstring RemoveLeadingZeros(std::wstring& str) {
    while (str.length() > 0 && str[0] == L'0') {
        str.erase(str.begin());
    }
    return str;
}

void ControlRoundTrip(const int64_t& time_point, const wchar_t* sys_time) {
	DatetimeContainer f;
	f.id = 0;
	f.creation_date = TimePoint(time_point);

	const auto& db = Database::Instance();
	db.Save(f);

	const auto retrieved = db.FetchAll<DatetimeContainer>();
	EXPECT_EQ(1, retrieved.size());

	auto sys_time_actual = retrieved[0].creation_date.SystemTime();
    sys_time_actual = RemoveLeadingZeros(sys_time_actual);
    
    auto sys_time_expected = std::wstring(sys_time);
    sys_time_expected = RemoveLeadingZeros(sys_time_expected);
    
	EXPECT_EQ(sys_time_expected, sys_time_actual);
}

TEST_F(DateTimeTest, UnixEpoch) {
	ControlRoundTrip(0, L"1970-01-01T00:00:00 UTC");
}

TEST_F(DateTimeTest, RoundtripBefore1000) {
    ControlRoundTrip(-32350628573LL, L"944-11-06T10:17:07 UTC");
}

TEST_F(DateTimeTest, RoundtripBefore1900) {
	ControlRoundTrip(-2359097130LL, L"1895-03-30T15:14:30 UTC");
}

TEST_F(DateTimeTest, RoundtripBefore1970) {
	ControlRoundTrip(-1508726294, L"1922-03-11T21:21:46 UTC");
}

TEST_F(DateTimeTest, RoundtripAfter2000) {
	ControlRoundTrip(1688842924, L"2023-07-08T19:02:04 UTC");
}

TEST_F(DateTimeTest, RoundtripAfter2038) {
	ControlRoundTrip(2333089535, L"2043-12-07T08:25:35 UTC");
}
