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
		Database::Initialize("");
	}

	void TearDown() override {
		Database::Finalize();
	}
};

void ControlRoundTrip(const time64_t time_point, const wchar_t* utc_time_stamp) {
	DatetimeContainer f;
	f.creation_date = TimePoint(time_point);

	const auto& db = Database::Instance();
	db.Save(f);

	const auto retrieved = db.FetchAll<DatetimeContainer>();
	EXPECT_EQ(1, retrieved.size());

	const auto timestamp = retrieved[0].creation_date.UtcTimestamp();
	EXPECT_EQ(std::wstring(utc_time_stamp), timestamp);
}

TEST_F(DateTimeTest, UnixEpoch) {
	ControlRoundTrip(0, L"1970-01-01T00:00:00");
}

TEST_F(DateTimeTest, RoundtripBefore1000) {
#ifdef _WIN32
	ControlRoundTrip(-32350628573LL, L"0944-11-06T10:17:07");
#else
	ControlRoundTrip(-32350628573LL, L"944-11-06T10:17:07");
#endif
}

TEST_F(DateTimeTest, RoundtripBefore1900) {
	ControlRoundTrip(-2359097130LL, L"1895-03-30T15:14:30");
}

TEST_F(DateTimeTest, RoundtripBefore1970) {
	ControlRoundTrip(-1508726294, L"1922-03-11T21:21:46");
}

TEST_F(DateTimeTest, RoundtripAfter2000) {
	ControlRoundTrip(1688842924, L"2023-07-08T19:02:04");
}

TEST_F(DateTimeTest, RoundtripAfter2038) {
	ControlRoundTrip(2333089535, L"2043-12-07T08:25:35");
}
