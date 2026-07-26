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

#include <string>
#include <vector>

#include "database.h"
#include "person.h"
#include "vector.h"

using namespace sqlite_reflection;

namespace {
class FunctionalVectorTest : public ::testing::Test {
    void SetUp() override {
        Database::Initialize("");
    }

    void TearDown() override {
        Database::Finalize();
    }
};

// The whole point of the migration: a fetch result is an fcpp::vector, so functional operations
// can be chained directly on it, without the caller wrapping the std::vector themselves.
TEST_F(FunctionalVectorTest, FetchResultChainsFilterAndMap) {
    const auto db = Database::Instance();
    db->Save(std::vector<Person>{
        {L"ada", L"lovelace", 36, true, 1},
        {L"grace", L"hopper", 17, true, 2},
        {L"alan", L"turing", 41, true, 3},
    });

    // filter() (mutating, returns vector&) then map<U>() (non-mutating) chained on the temporary
    // returned by FetchAll(), exactly as advertised in the README.
    const auto adult_names = db->FetchAll<Person>()
                                 .filter([](const Person& p) { return p.age >= 18; })
                                 .map<std::wstring>([](const Person& p) { return p.GetFullName(); });

    ASSERT_EQ(2u, adult_names.size());
    EXPECT_EQ(L"ada lovelace", adult_names[0]);
    EXPECT_EQ(L"alan turing", adult_names[1]);
}

TEST_F(FunctionalVectorTest, FetchResultSupportsAllOfAnyOf) {
    const auto db = Database::Instance();
    db->Save(std::vector<Person>{
        {L"ada", L"lovelace", 36, true, 1},
        {L"grace", L"hopper", 17, true, 2},
    });

    const auto people = db->FetchAll<Person>();
    EXPECT_TRUE(people.all_of([](const Person& p) { return p.age > 0; }));
    EXPECT_FALSE(people.all_of([](const Person& p) { return p.age >= 18; }));
    EXPECT_TRUE(people.any_of([](const Person& p) { return p.age < 18; }));
    EXPECT_FALSE(people.any_of([](const Person& p) { return p.age > 100; }));
}

// range-for and indexing over a fetch result keep working as with a std::vector
TEST_F(FunctionalVectorTest, FetchResultSupportsRangeForAndIndexing) {
    const auto db = Database::Instance();
    db->Save(std::vector<Person>{
        {L"ada", L"lovelace", 36, true, 1},
        {L"alan", L"turing", 41, true, 2},
    });

    const auto people = db->FetchAll<Person>();

    int64_t total_age = 0;
    for (const auto& person : people) {
        total_age += person.age;
    }
    EXPECT_EQ(77, total_age);
    EXPECT_EQ(L"ada", people[0].first_name);
    EXPECT_EQ(L"alan", people[1].first_name);
}

// Batch Save/Update accept an fcpp::vector directly.
TEST_F(FunctionalVectorTest, BatchSaveAndUpdateAcceptFcppVector) {
    const auto db = Database::Instance();

    fcpp::vector<Person> people;
    people.insert_back({L"ada", L"lovelace", 36, true, 1}).insert_back({L"alan", L"turing", 41, true, 2});

    db->Save(people);

    auto fetched = db->FetchAll<Person>();
    ASSERT_EQ(2u, fetched.size());
    EXPECT_EQ(36, fetched[0].age);

    people[0].age = 37;
    db->Update(people);

    fetched = db->FetchAll<Person>();
    ASSERT_EQ(2u, fetched.size());
    EXPECT_EQ(37, fetched[0].age);
}

// SaveAutoIncrement writes the generated ids back into the passed-in fcpp::vector.
TEST_F(FunctionalVectorTest, SaveAutoIncrementWritesIdsBackIntoFcppVector) {
    const auto db = Database::Instance();

    fcpp::vector<Person> people;
    people.insert_back({L"ada", L"lovelace", 36, true}).insert_back({L"alan", L"turing", 41, true});

    db->SaveAutoIncrement(people);

    EXPECT_EQ(1, people[0].id);
    EXPECT_EQ(2, people[1].id);
    EXPECT_EQ(2u, db->FetchAll<Person>().size());
}

// The std::vector compatibility overloads keep working for callers that still pass a std::vector.
TEST_F(FunctionalVectorTest, StdVectorCompatibilityOverloadsStillWork) {
    const auto db = Database::Instance();

    std::vector<Person> people;
    people.push_back({L"ada", L"lovelace", 36, true, 1});
    people.push_back({L"alan", L"turing", 41, true, 2});

    db->Save(people);
    EXPECT_EQ(2u, db->FetchAll<Person>().size());

    people[0].age = 37;
    db->Update(people);
    EXPECT_EQ(37, db->Fetch<Person>(1).age);
}

// The SaveAutoIncrement std::vector wrapper writes the generated ids back into the caller's
// std::vector, just like the fcpp::vector overload.
TEST_F(FunctionalVectorTest, SaveAutoIncrementStdVectorWrapperWritesIdsBack) {
    const auto db = Database::Instance();

    std::vector<Person> people;
    people.push_back({L"ada", L"lovelace", 36, true});
    people.push_back({L"alan", L"turing", 41, true});

    db->SaveAutoIncrement(people);

    EXPECT_EQ(1, people[0].id);
    EXPECT_EQ(2, people[1].id);
    EXPECT_EQ(2u, db->FetchAll<Person>().size());
}
}  // namespace
