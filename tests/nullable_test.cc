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

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <type_traits>

#include "database.h"
#include "nullable_record.h"
#include "person.h"

using namespace sqlite_reflection;

// Under C++17 and later, a nullable field is literally std::optional of the underlying type
// (fcpp::optional_t aliases std::optional there); under C++11 an equivalent fallback is used.
// The tests below only use the API surface common to both (has_value, operator*, assignment).
#if __cplusplus >= 201703L
static_assert(std::is_same<decltype(NullableRecord::maybe_int), std::optional<int64_t>>::value,
              "a nullable INT field must be std::optional<int64_t> under C++17+");
static_assert(std::is_same<decltype(NullableRecord::maybe_text), std::optional<std::wstring>>::value,
              "a nullable TEXT field must be std::optional<std::wstring> under C++17+");
#endif

class NullableTest : public ::testing::Test {
    void SetUp() override {
        Database::Initialize("");
    }

    void TearDown() override {
        Database::Finalize();
    }
};

TEST_F(NullableTest, UnsetNullableFieldsRoundTripAsNull) {
    const auto db = Database::Instance();

    NullableRecord unset;
    unset.label = L"unset";
    unset.id = 1;
    db->Save(unset);

    // Every nullable field hydrates back to the empty/absent state
    const auto fetched = db->Fetch<NullableRecord>(1);
    EXPECT_EQ(L"unset", fetched.label);
    EXPECT_FALSE(fetched.maybe_int.has_value());
    EXPECT_FALSE(fetched.maybe_real.has_value());
    EXPECT_FALSE(fetched.maybe_text.has_value());
    EXPECT_FALSE(fetched.maybe_bool.has_value());
    EXPECT_FALSE(fetched.maybe_date.has_value());

    // SQL-level proof that a real NULL was stored: SQLite's own IS NULL matches the row for
    // each storage class, which a bound value or empty string never would
    EXPECT_EQ(1, db->Fetch<NullableRecord>(&IsNull(&NullableRecord::maybe_int)).size());
    EXPECT_EQ(1, db->Fetch<NullableRecord>(&IsNull(&NullableRecord::maybe_real)).size());
    EXPECT_EQ(1, db->Fetch<NullableRecord>(&IsNull(&NullableRecord::maybe_text)).size());
    EXPECT_EQ(1, db->Fetch<NullableRecord>(&IsNull(&NullableRecord::maybe_bool)).size());
    EXPECT_EQ(1, db->Fetch<NullableRecord>(&IsNull(&NullableRecord::maybe_date)).size());
}

TEST_F(NullableTest, SetNullableFieldsRoundTripEqual) {
    const auto db = Database::Instance();

    NullableRecord set;
    set.label = L"set";
    set.maybe_int = static_cast<int64_t>(42);
    set.maybe_real = 2.5;
    set.maybe_text = std::wstring(L"hello");
    set.maybe_bool = true;
    set.maybe_date = TimePoint(static_cast<int64_t>(1600000000));
    set.id = 1;
    db->Save(set);

    const auto fetched = db->Fetch<NullableRecord>(1);
    ASSERT_TRUE(fetched.maybe_int.has_value());
    EXPECT_EQ(42, *fetched.maybe_int);
    ASSERT_TRUE(fetched.maybe_real.has_value());
    EXPECT_EQ(2.5, *fetched.maybe_real);
    ASSERT_TRUE(fetched.maybe_text.has_value());
    EXPECT_EQ(L"hello", *fetched.maybe_text);
    ASSERT_TRUE(fetched.maybe_bool.has_value());
    EXPECT_TRUE(*fetched.maybe_bool);
    ASSERT_TRUE(fetched.maybe_date.has_value());
    EXPECT_EQ(TimePoint(static_cast<int64_t>(1600000000)).SystemTime(), (*fetched.maybe_date).SystemTime());
}

TEST_F(NullableTest, NullIsDistinguishableFromEmptyStringAndZero) {
    const auto db = Database::Instance();

    // The #20 core case: an absent value, an explicitly stored empty string, and an explicitly
    // stored zero must all be distinguishable after a round-trip
    NullableRecord absent;
    absent.label = L"absent";
    absent.id = 1;
    db->Save(absent);

    NullableRecord present;
    present.label = L"present";
    present.maybe_text = std::wstring();
    present.maybe_int = static_cast<int64_t>(0);
    present.maybe_real = 0.0;
    present.id = 2;
    db->Save(present);

    const auto fetched_absent = db->Fetch<NullableRecord>(1);
    EXPECT_FALSE(fetched_absent.maybe_text.has_value());
    EXPECT_FALSE(fetched_absent.maybe_int.has_value());
    EXPECT_FALSE(fetched_absent.maybe_real.has_value());

    const auto fetched_present = db->Fetch<NullableRecord>(2);
    ASSERT_TRUE(fetched_present.maybe_text.has_value());
    EXPECT_EQ(L"", *fetched_present.maybe_text);
    ASSERT_TRUE(fetched_present.maybe_int.has_value());
    EXPECT_EQ(0, *fetched_present.maybe_int);
    ASSERT_TRUE(fetched_present.maybe_real.has_value());
    EXPECT_EQ(0.0, *fetched_present.maybe_real);
}

TEST_F(NullableTest, SchemaEmitsNotNullForNonNullableColumnsOnly) {
    const auto db = Database::Instance();

    // Non-nullable columns now carry NOT NULL in the generated schema, so SQLite itself
    // rejects a NULL - both in a purely non-nullable record and for the non-nullable member
    // of a mixed record
    EXPECT_ANY_THROW(
        db->UnsafeSql("INSERT INTO Person (first_name, last_name, age, is_vaccinated) VALUES (NULL, 'x', 1, 0)"));
    EXPECT_ANY_THROW(db->UnsafeSql("INSERT INTO NullableRecord (label) VALUES (NULL)"));

    // Nullable columns carry no NOT NULL: explicit and implicit (omitted-column) NULLs are
    // accepted
    db->UnsafeSql("INSERT INTO NullableRecord (label, maybe_int) VALUES ('x', NULL)");
    const auto all = db->FetchAll<NullableRecord>();
    ASSERT_EQ(1, all.size());
    EXPECT_FALSE(all[0].maybe_int.has_value());
    EXPECT_FALSE(all[0].maybe_text.has_value());
}

TEST_F(NullableTest, IsNullAndIsNotNullSelectTheRightRows) {
    const auto db = Database::Instance();

    NullableRecord unset;
    unset.label = L"unset";
    unset.id = 1;
    db->Save(unset);

    NullableRecord set;
    set.label = L"set";
    set.maybe_int = static_cast<int64_t>(7);
    set.id = 2;
    db->Save(set);

    const auto is_null = IsNull(&NullableRecord::maybe_int);
    const auto null_rows = db->Fetch<NullableRecord>(&is_null);
    ASSERT_EQ(1, null_rows.size());
    EXPECT_EQ(1, null_rows[0].id);

    const auto is_not_null = IsNotNull(&NullableRecord::maybe_int);
    const auto not_null_rows = db->Fetch<NullableRecord>(&is_not_null);
    ASSERT_EQ(1, not_null_rows.size());
    EXPECT_EQ(2, not_null_rows[0].id);

    // The nullness predicates survive Clone() inside And()/Or() compounds, like every
    // other predicate
    const auto combined = IsNull(&NullableRecord::maybe_int).And(Equal(&NullableRecord::label, L"unset"));
    const auto combined_rows = db->Fetch<NullableRecord>(&combined);
    ASSERT_EQ(1, combined_rows.size());
    EXPECT_EQ(1, combined_rows[0].id);
}

TEST_F(NullableTest, ValuePredicatesOperateOnTheContainedValue) {
    const auto db = Database::Instance();

    NullableRecord set;
    set.label = L"set";
    set.maybe_int = static_cast<int64_t>(7);
    set.id = 1;
    db->Save(set);

    NullableRecord unset;
    unset.label = L"unset";
    unset.id = 2;
    db->Save(unset);

    // A value predicate on a nullable member compares against the contained value
    const auto equal_seven = Equal(&NullableRecord::maybe_int, fcpp::optional_t<int64_t>(static_cast<int64_t>(7)));
    const auto rows = db->Fetch<NullableRecord>(&equal_seven);
    ASSERT_EQ(1, rows.size());
    EXPECT_EQ(1, rows[0].id);

    // Handing a value predicate an empty optional is a misuse ("= NULL" never matches in
    // SQL); it fails fast with a pointer to IsNull()/IsNotNull()
    EXPECT_THROW(Equal(&NullableRecord::maybe_int, fcpp::optional_t<int64_t>()), std::invalid_argument);
}

TEST_F(NullableTest, NonNullableRecordsRoundTripUnchanged) {
    const auto db = Database::Instance();

    // Regression: a record without any nullable members behaves exactly as before
    const Person p{L"ada", L"lovelace", 36, true, 1};
    db->Save(p);

    const auto fetched = db->Fetch<Person>(1);
    EXPECT_EQ(p.first_name, fetched.first_name);
    EXPECT_EQ(p.last_name, fetched.last_name);
    EXPECT_EQ(p.age, fetched.age);
    EXPECT_EQ(p.is_vaccinated, fetched.is_vaccinated);
}
