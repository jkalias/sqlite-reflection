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

#include "database.h"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>
#include <utility>

#include "company.h"
#include "id_only_record.h"
#include "person.h"
#include "pet.h"

using namespace sqlite_reflection;

namespace {
// C++11 stand-in for std::void_t (C++17)
template <typename...>
struct MakeVoid {
    typedef void type;
};

// Detects whether Save(const T&) is invocable through a const Database
template <typename T, typename = void>
struct CanSaveThroughConst : std::false_type {};

template <typename T>
struct CanSaveThroughConst<
    T, typename MakeVoid<decltype(std::declval<const Database&>().Save(std::declval<const T&>()))>::type>
    : std::true_type {};

// Detects whether FetchAll<T>() is invocable through a const Database
template <typename T, typename = void>
struct CanFetchAllThroughConst : std::false_type {};

template <typename T>
struct CanFetchAllThroughConst<T, typename MakeVoid<decltype(std::declval<const Database&>().FetchAll<T>())>::type>
    : std::true_type {};
}  // namespace

// The const-correctness contract (#26): const means read-only. A handle to a const Database
// can fetch but not mutate; the write methods are only invocable through a non-const handle.
static_assert(!CanSaveThroughConst<Person>::value,
              "Save must not be invocable through a const Database - write methods are non-const");
static_assert(CanFetchAllThroughConst<Person>::value,
              "FetchAll must remain invocable through a const Database - read methods are const");

class DatabaseTest : public ::testing::Test {
    void SetUp() override {
        Database::Initialize("");
    }

    void TearDown() override {
        Database::Finalize();
    }
};

TEST_F(DatabaseTest, ReadOnlyHandleCanStillFetch) {
    const auto db = Database::Instance();
    const Person p{L"ada", L"lovelace", 36, true, 1};
    db->Save(p);

    // The implicit shared_ptr<Database> -> shared_ptr<const Database> conversion yields a
    // read-only view, through which the const fetch methods keep working
    std::shared_ptr<const Database> reader = Database::Instance();
    const auto all = reader->FetchAll<Person>();
    ASSERT_EQ(1, all.size());
    EXPECT_EQ(p.first_name, all[0].first_name);
}

TEST_F(DatabaseTest, Initialization) {
    const auto db = Database::Instance();

    const auto all_persons = db->FetchAll<Person>();
    EXPECT_EQ(0, all_persons.size());

    const auto all_pets = db->FetchAll<Pet>();
    EXPECT_EQ(0, all_pets.size());
}

TEST_F(DatabaseTest, SingleInsertion) {
    const auto db = Database::Instance();

    const Person p{L"παναγιώτης", L"ανδριανόπουλος", 39, 1};
    db->Save(p);

    const auto all_persons = db->FetchAll<Person>();
    EXPECT_EQ(1, all_persons.size());

    EXPECT_EQ(p.first_name, all_persons[0].first_name);
    EXPECT_EQ(p.last_name, all_persons[0].last_name);
    EXPECT_EQ(p.age, all_persons[0].age);
    EXPECT_EQ(p.id, all_persons[0].id);
}

TEST_F(DatabaseTest, SingleInsertionWithAutoIdIncrement) {
    const auto db = Database::Instance();

    Person p{L"παναγιώτης", L"ανδριανόπουλος", 39};
    db->SaveAutoIncrement(p);

    // The generated id is written back into the passed-in record
    EXPECT_EQ(1, p.id);

    const auto all_persons = db->FetchAll<Person>();
    EXPECT_EQ(1, all_persons.size());

    EXPECT_EQ(p.first_name, all_persons[0].first_name);
    EXPECT_EQ(p.last_name, all_persons[0].last_name);
    EXPECT_EQ(p.age, all_persons[0].age);
    EXPECT_EQ(1, all_persons[0].id);
}

TEST_F(DatabaseTest, MultipleInsertionsWithAutoIdIncrement) {
    const auto db = Database::Instance();

    std::vector<Person> persons;
    persons.push_back({L"παναγιώτης", L"ανδριανόπουλος", 28});
    persons.push_back({L"peter", L"meier", 32});

    db->SaveAutoIncrement(persons);

    // The generated ids are written back into the passed-in records
    EXPECT_EQ(1, persons[0].id);
    EXPECT_EQ(2, persons[1].id);

    const auto saved_persons = db->FetchAll<Person>();
    EXPECT_EQ(2, saved_persons.size());

    for (auto i = 0; i < saved_persons.size(); ++i) {
        EXPECT_EQ(persons[i].id, saved_persons[i].id);
        EXPECT_EQ(persons[i].first_name, saved_persons[i].first_name);
        EXPECT_EQ(persons[i].last_name, saved_persons[i].last_name);
        EXPECT_EQ(persons[i].age, saved_persons[i].age);
    }
}

TEST_F(DatabaseTest, AutoIdIncrementContinuesFromExistingMaxId) {
    const auto db = Database::Instance();

    const Person existing{L"ada", L"lovelace", 36, true, 10};
    db->Save(existing);

    Person p{L"grace", L"hopper", 85};
    db->SaveAutoIncrement(p);

    // The new id continues from the current maximum id in the table
    EXPECT_EQ(11, p.id);
}

TEST_F(DatabaseTest, AutoIdIsNotReusedAfterDeletingHighestRow) {
    const auto db = Database::Instance();

    Person first{L"ada", L"lovelace", 36};
    db->SaveAutoIncrement(first);
    EXPECT_EQ(1, first.id);

    Person second{L"grace", L"hopper", 85};
    db->SaveAutoIncrement(second);
    EXPECT_EQ(2, second.id);

    // Remove the row with the highest id
    db->Delete(second);

    // A subsequent auto-increment save must not reuse the freed id
    Person third{L"john", L"doe", 28};
    db->SaveAutoIncrement(third);
    EXPECT_EQ(3, third.id);
}

TEST_F(DatabaseTest, AutoIncrementInsertForIdOnlyRecord) {
    const auto db = Database::Instance();

    // A record whose only column is the implicit id must still insert via the
    // auto-increment path (INSERT INTO ... DEFAULT VALUES) and get an assigned id
    IdOnlyRecord first;
    db->SaveAutoIncrement(first);
    EXPECT_EQ(1, first.id);

    IdOnlyRecord second;
    db->SaveAutoIncrement(second);
    EXPECT_EQ(2, second.id);

    const auto all = db->FetchAll<IdOnlyRecord>();
    EXPECT_EQ(2, all.size());
    EXPECT_EQ(1, all[0].id);
    EXPECT_EQ(2, all[1].id);
}

TEST_F(DatabaseTest, MultipleInsertions) {
    const auto db = Database::Instance();

    std::vector<Person> persons;

    persons.push_back({L"παναγιώτης", L"ανδριανόπουλος", 28, false, 3});
    persons.push_back({L"peter", L"meier", 32, true, 5});

    db->Save(persons);

    const auto saved_persons = db->FetchAll<Person>();
    EXPECT_EQ(2, saved_persons.size());

    for (auto i = 0; i < saved_persons.size(); ++i) {
        EXPECT_EQ(persons[i].id, saved_persons[i].id);
        EXPECT_EQ(persons[i].first_name, saved_persons[i].first_name);
        EXPECT_EQ(persons[i].last_name, saved_persons[i].last_name);
        EXPECT_EQ(persons[i].age, saved_persons[i].age);
        EXPECT_EQ(persons[i].is_vaccinated, saved_persons[i].is_vaccinated);
    }
}

TEST_F(DatabaseTest, InsertionOnOneTypeDoesNotAffectOtherType) {
    const auto db = Database::Instance();

    const Person p{L"παναγιώτης", L"ανδριανόπουλος", 39, 1};
    db->Save(p);

    const auto all_pets = db->FetchAll<Pet>();
    EXPECT_EQ(0, all_pets.size());
}

TEST_F(DatabaseTest, SingleUpdate) {
    const auto db = Database::Instance();

    Person p{L"παναγιώτης", L"ανδριανόπουλος", 39, 1};
    db->Save(p);

    p.age = 23;
    p.first_name = L"max";

    db->Update(p);

    const auto all_persons = db->FetchAll<Person>();
    EXPECT_EQ(1, all_persons.size());

    EXPECT_EQ(p.first_name, all_persons[0].first_name);
    EXPECT_EQ(p.last_name, all_persons[0].last_name);
    EXPECT_EQ(p.age, all_persons[0].age);
    EXPECT_EQ(p.id, all_persons[0].id);
}

TEST_F(DatabaseTest, MultipleUpdates) {
    const auto db = Database::Instance();

    std::vector<Person> persons;

    persons.push_back({L"john", L"doe", 28, false, 3});
    persons.push_back({L"mary", L"poppins", 29, false, 5});

    db->Save(persons);

    persons[0].last_name = L"rambo";
    persons[1].age = 20;

    db->Update(persons);

    const auto saved_persons = db->FetchAll<Person>();
    EXPECT_EQ(2, saved_persons.size());

    for (auto i = 0; i < saved_persons.size(); ++i) {
        EXPECT_EQ(persons[i].id, saved_persons[i].id);
        EXPECT_EQ(persons[i].first_name, saved_persons[i].first_name);
        EXPECT_EQ(persons[i].last_name, saved_persons[i].last_name);
        EXPECT_EQ(persons[i].age, saved_persons[i].age);
    }
}

TEST_F(DatabaseTest, InsertedTextSqlPayloadIsPersistedLiterally) {
    const auto db = Database::Instance();

    const Person p{L"john'); DROP TABLE Person; --", L"payload", 39, false, 1};
    db->Save(p);

    const auto all_persons = db->FetchAll<Person>();
    EXPECT_EQ(1, all_persons.size());
    EXPECT_EQ(p.first_name, all_persons[0].first_name);
    EXPECT_EQ(p.last_name, all_persons[0].last_name);
    EXPECT_EQ(p.age, all_persons[0].age);
    EXPECT_EQ(p.id, all_persons[0].id);
}

TEST_F(DatabaseTest, InsertedTextWithEmbeddedNulIsPersistedLiterally) {
    const auto db = Database::Instance();

    const Person p{std::wstring(L"a\0b", 3), L"payload", 39, false, 1};
    db->Save(p);

    const auto all_persons = db->FetchAll<Person>();
    ASSERT_EQ(1, all_persons.size());
    EXPECT_EQ(p.first_name, all_persons[0].first_name);
    EXPECT_EQ(3, all_persons[0].first_name.size());

    const auto predicate = Equal(&Person::first_name, std::wstring(L"a\0b", 3));
    const auto matching_persons = db->Fetch<Person>(&predicate);
    ASSERT_EQ(1, matching_persons.size());
    EXPECT_EQ(p.first_name, matching_persons[0].first_name);
}

TEST_F(DatabaseTest, UpdatedTextSqlPayloadIsPersistedLiterally) {
    const auto db = Database::Instance();

    Person p{L"john", L"payload", 39, false, 1};
    db->Save(p);

    p.first_name = L"updated'); DELETE FROM Person; --";
    db->Update(p);

    const auto all_persons = db->FetchAll<Person>();
    EXPECT_EQ(1, all_persons.size());
    EXPECT_EQ(p.first_name, all_persons[0].first_name);
    EXPECT_EQ(p.last_name, all_persons[0].last_name);
    EXPECT_EQ(p.age, all_persons[0].age);
    EXPECT_EQ(p.id, all_persons[0].id);
}

TEST_F(DatabaseTest, DeleteWithRecord) {
    const auto db = Database::Instance();

    std::vector<Person> persons;

    persons.push_back({L"παναγιώτης", L"ανδριανόπουλος", 28, false, 3});
    persons.push_back({L"peter", L"meier", 32, false, 5});
    persons.push_back({L"mary", L"poppins", 20, false, 13});

    db->Save(persons);

    auto saved_persons = db->FetchAll<Person>();
    EXPECT_EQ(3, saved_persons.size());

    db->Delete(persons[1]);
    saved_persons = db->FetchAll<Person>();
    EXPECT_EQ(2, saved_persons.size());

    auto i = 0;
    EXPECT_EQ(3, saved_persons[i].id);
    EXPECT_EQ(L"παναγιώτης", saved_persons[i].first_name);
    EXPECT_EQ(L"ανδριανόπουλος", saved_persons[i].last_name);
    EXPECT_EQ(28, saved_persons[i].age);

    i++;
    EXPECT_EQ(13, saved_persons[i].id);
    EXPECT_EQ(L"mary", saved_persons[i].first_name);
    EXPECT_EQ(L"poppins", saved_persons[i].last_name);
    EXPECT_EQ(20, saved_persons[i].age);
}

TEST_F(DatabaseTest, DeleteWithId) {
    const auto db = Database::Instance();

    std::vector<Person> persons;

    persons.push_back({L"παναγιώτης", L"ανδριανόπουλος", 28, false, 3});
    persons.push_back({L"peter", L"meier", 32, false, 5});
    persons.push_back({L"mary", L"poppins", 20, false, 13});

    db->Save(persons);

    auto saved_persons = db->FetchAll<Person>();
    EXPECT_EQ(3, saved_persons.size());

    db->Delete<Person>(persons[1].id);
    saved_persons = db->FetchAll<Person>();
    EXPECT_EQ(2, saved_persons.size());

    auto i = 0;
    EXPECT_EQ(3, saved_persons[i].id);
    EXPECT_EQ(L"παναγιώτης", saved_persons[i].first_name);
    EXPECT_EQ(L"ανδριανόπουλος", saved_persons[i].last_name);
    EXPECT_EQ(28, saved_persons[i].age);

    i++;
    EXPECT_EQ(13, saved_persons[i].id);
    EXPECT_EQ(L"mary", saved_persons[i].first_name);
    EXPECT_EQ(L"poppins", saved_persons[i].last_name);
    EXPECT_EQ(20, saved_persons[i].age);
}

TEST_F(DatabaseTest, DeleteWithPredicate) {
    const auto db = Database::Instance();

    std::vector<Person> persons;

    persons.push_back({L"παναγιώτης", L"ανδριανόπουλος", 28, true, 3});
    persons.push_back({L"peter", L"meier", 32, false, 5});
    persons.push_back({L"mary", L"poppins", 20, true, 13});

    db->Save(persons);

    const auto age_match_predicate = SmallerThan(&Person::age, 30).And(Equal(&Person::is_vaccinated, true));

    db->Delete<Person>(&age_match_predicate);
    const auto fetched_persons = db->FetchAll<Person>();
    EXPECT_EQ(1, fetched_persons.size());

    EXPECT_EQ(5, fetched_persons[0].id);
    EXPECT_EQ(L"peter", fetched_persons[0].first_name);
    EXPECT_EQ(L"meier", fetched_persons[0].last_name);
    EXPECT_EQ(32, fetched_persons[0].age);
    EXPECT_EQ(false, fetched_persons[0].is_vaccinated);
}

TEST_F(DatabaseTest, DeleteWithInjectedPredicateDoesNotDeleteRows) {
    const auto db = Database::Instance();

    std::vector<Person> persons;

    persons.push_back({L"john", L"doe", 28, false, 3});
    persons.push_back({L"mary", L"poppins", 20, false, 5});

    db->Save(persons);

    const auto injected_predicate = Equal(&Person::first_name, L"nobody' OR 1=1 --");
    db->Delete<Person>(&injected_predicate);

    const auto fetched_persons = db->FetchAll<Person>();
    EXPECT_EQ(2, fetched_persons.size());
}

TEST_F(DatabaseTest, SingleFetch) {
    const auto db = Database::Instance();

    std::vector<Person> persons;

    persons.push_back({L"παναγιώτης", L"ανδριανόπουλος", 28, false, 3});
    persons.push_back({L"peter", L"meier", 32, false, 5});
    persons.push_back({L"mary", L"poppins", 20, false, 13});

    db->Save(persons);

    const auto fetched_person = db->Fetch<Person>(5);
    EXPECT_EQ(5, fetched_person.id);
    EXPECT_EQ(L"peter", fetched_person.first_name);
    EXPECT_EQ(L"meier", fetched_person.last_name);
    EXPECT_EQ(32, fetched_person.age);
}

TEST_F(DatabaseTest, SingleFetchWithoutExistingRecordExpectingException) {
    const auto db = Database::Instance();

    std::vector<Person> persons;

    persons.push_back({L"παναγιώτης", L"ανδριανόπουλος", 28, false, 3});
    persons.push_back({L"peter", L"meier", 32, false, 5});
    persons.push_back({L"mary", L"poppins", 20, false});

    db->Save(persons);

    EXPECT_ANY_THROW(db->Fetch<Person>(15));
}

TEST_F(DatabaseTest, FetchWithInjectedPredicateDoesNotMatchAllRows) {
    const auto db = Database::Instance();

    std::vector<Person> persons;

    persons.push_back({L"john", L"doe", 28, false, 3});
    persons.push_back({L"mary", L"poppins", 20, false, 5});

    db->Save(persons);

    const auto injected_predicate = Equal(&Person::first_name, L"john' OR 1=1 --");
    const auto fetched_persons = db->Fetch<Person>(&injected_predicate);
    EXPECT_EQ(0, fetched_persons.size());
}

TEST_F(DatabaseTest, FetchWithSimilarPredicateString) {
    const auto db = Database::Instance();

    std::vector<Company> company;

    company.push_back({L"Paul", 32, L"California", 20000.0, 1});
    company.push_back({L"Allen", 25, L"Texas", 15000.0, 2});
    company.push_back({L"Teddy", 23, L"Norway", 20000.0, 3});
    company.push_back({L"Mark", 25, L"Rich-Mond", 65000.0, 4});
    company.push_back({L"David", 27, L"Texas", 85000.0, 5});
    company.push_back({L"Kim", 22, L"South-Hall", 45000.0, 6});
    company.push_back({L"Janes", 24, L"Houston", 10000.0, 7});

    db->Save(company);

    auto fetch_condition = Like(&Company::address, L"-");

    auto fetched_persons = db->Fetch<Company>(&fetch_condition);
    EXPECT_EQ(2, fetched_persons.size());

    EXPECT_EQ(4, fetched_persons[0].id);
    EXPECT_EQ(L"Mark", fetched_persons[0].name);
    EXPECT_EQ(25, fetched_persons[0].age);
    EXPECT_EQ(L"Rich-Mond", fetched_persons[0].address);
    EXPECT_EQ(65000.0, fetched_persons[0].salary);

    EXPECT_EQ(6, fetched_persons[1].id);
    EXPECT_EQ(L"Kim", fetched_persons[1].name);
    EXPECT_EQ(22, fetched_persons[1].age);
    EXPECT_EQ(L"South-Hall", fetched_persons[1].address);
    EXPECT_EQ(45000.0, fetched_persons[1].salary);
}

TEST_F(DatabaseTest, FetchWithSimilarPredicateDouble) {
    const auto db = Database::Instance();

    std::vector<Company> company;

    company.push_back({L"Paul", 32, L"California", 20000.0, 1});
    company.push_back({L"Allen", 25, L"Texas", 15000.0, 2});
    company.push_back({L"Teddy", 23, L"Norway", 20000.0, 3});
    company.push_back({L"Mark", 25, L"Rich-Mond", 65000.0, 4});
    company.push_back({L"David", 27, L"Texas", 85000.0, 5});
    company.push_back({L"Kim", 22, L"South-Hall", 45000.0, 6});
    company.push_back({L"Janes", 24, L"Houston", 10000.0, 7});

    db->Save(company);

    auto fetch_condition = Like(&Company::salary, 5000.0);

    auto fetched_persons = db->Fetch<Company>(&fetch_condition);
    EXPECT_EQ(4, fetched_persons.size());

    EXPECT_EQ(2, fetched_persons[0].id);
    EXPECT_EQ(L"Allen", fetched_persons[0].name);
    EXPECT_EQ(25, fetched_persons[0].age);
    EXPECT_EQ(L"Texas", fetched_persons[0].address);
    EXPECT_EQ(15000, fetched_persons[0].salary);

    EXPECT_EQ(4, fetched_persons[1].id);
    EXPECT_EQ(L"Mark", fetched_persons[1].name);
    EXPECT_EQ(25, fetched_persons[1].age);
    EXPECT_EQ(L"Rich-Mond", fetched_persons[1].address);
    EXPECT_EQ(65000, fetched_persons[1].salary);

    EXPECT_EQ(5, fetched_persons[2].id);
    EXPECT_EQ(L"David", fetched_persons[2].name);
    EXPECT_EQ(27, fetched_persons[2].age);
    EXPECT_EQ(L"Texas", fetched_persons[2].address);
    EXPECT_EQ(85000, fetched_persons[2].salary);

    EXPECT_EQ(6, fetched_persons[3].id);
    EXPECT_EQ(L"Kim", fetched_persons[3].name);
    EXPECT_EQ(22, fetched_persons[3].age);
    EXPECT_EQ(L"South-Hall", fetched_persons[3].address);
    EXPECT_EQ(45000.0, fetched_persons[3].salary);
}

TEST_F(DatabaseTest, FetchWithSimilarPredicateInt) {
    const auto db = Database::Instance();

    std::vector<Company> company;

    company.push_back({L"Paul", 32, L"California", 20000.0, 1});
    company.push_back({L"Allen", 25, L"Texas", 15000.0, 2});
    company.push_back({L"Teddy", 23, L"Norway", 20000.0, 3});
    company.push_back({L"Mark", 25, L"Rich-Mond", 65000.0, 4});
    company.push_back({L"David", 27, L"Texas", 85000.0, 5});
    company.push_back({L"Kim", 22, L"South-Hall", 45000.0, 6});
    company.push_back({L"Janes", 24, L"Houston", 10000.0, 7});

    db->Save(company);

    const auto fetch_condition = Like(&Company::age, 7);

    const auto fetched_persons = db->Fetch<Company>(&fetch_condition);
    EXPECT_EQ(1, fetched_persons.size());

    EXPECT_EQ(5, fetched_persons[0].id);
    EXPECT_EQ(L"David", fetched_persons[0].name);
    EXPECT_EQ(27, fetched_persons[0].age);
    EXPECT_EQ(L"Texas", fetched_persons[0].address);
    EXPECT_EQ(85000, fetched_persons[0].salary);
}

TEST_F(DatabaseTest, FetchWithPredicateChaining) {
    const auto db = Database::Instance();

    std::vector<Person> persons;

    persons.push_back({L"name1", L"surname1", 13, false, 1});
    persons.push_back({L"john", L"surname2", 25, false, 2});
    persons.push_back({L"john", L"surname3", 37, false, 3});
    persons.push_back({L"jame", L"surname4", 45, false, 4});
    persons.push_back({L"name5", L"surname5", 56, false, 5});

    db->Save(persons);

    const auto fetch_condition =
        GreaterThanOrEqual(&Person::id, 2).And(SmallerThan(&Person::id, 5)).And(Equal(&Person::first_name, L"john"));

    const auto fetched_persons = db->Fetch<Person>(&fetch_condition);
    EXPECT_EQ(2, fetched_persons.size());

    EXPECT_EQ(2, fetched_persons[0].id);
    EXPECT_EQ(L"john", fetched_persons[0].first_name);
    EXPECT_EQ(L"surname2", fetched_persons[0].last_name);
    EXPECT_EQ(25, fetched_persons[0].age);

    EXPECT_EQ(3, fetched_persons[1].id);
    EXPECT_EQ(L"john", fetched_persons[1].first_name);
    EXPECT_EQ(L"surname3", fetched_persons[1].last_name);
    EXPECT_EQ(37, fetched_persons[1].age);
}

TEST_F(DatabaseTest, LikeMatchesPercentWildcardLiterally) {
    const auto db = Database::Instance();

    std::vector<Person> persons;
    persons.push_back({L"50% off", L"doe", 30, false, 1});
    persons.push_back({L"5000 off", L"doe", 30, false, 2});
    db->Save(persons);

    // A literal '%' in the search value must not act as a SQLite LIKE wildcard
    const auto fetch_condition = Like(&Person::first_name, L"50%");
    const auto fetched = db->Fetch<Person>(&fetch_condition);

    ASSERT_EQ(1, fetched.size());
    EXPECT_EQ(1, fetched[0].id);
    EXPECT_EQ(L"50% off", fetched[0].first_name);
}

TEST_F(DatabaseTest, LikeMatchesUnderscoreWildcardLiterally) {
    const auto db = Database::Instance();

    std::vector<Person> persons;
    persons.push_back({L"a_b", L"doe", 30, false, 1});
    persons.push_back({L"axb", L"doe", 30, false, 2});
    db->Save(persons);

    // A literal '_' in the search value must not act as a SQLite LIKE any-single-char wildcard
    const auto fetch_condition = Like(&Person::first_name, L"a_b");
    const auto fetched = db->Fetch<Person>(&fetch_condition);

    ASSERT_EQ(1, fetched.size());
    EXPECT_EQ(1, fetched[0].id);
    EXPECT_EQ(L"a_b", fetched[0].first_name);
}

TEST_F(DatabaseTest, LikeMatchesBackslashLiterally) {
    const auto db = Database::Instance();

    std::vector<Person> persons;
    persons.push_back({L"a\\b", L"doe", 30, false, 1});
    persons.push_back({L"axb", L"doe", 30, false, 2});
    db->Save(persons);

    // A literal backslash in the search value must match literally, not be misinterpreted as
    // (or interfere with) the ESCAPE character
    const auto fetch_condition = Like(&Person::first_name, L"a\\b");
    const auto fetched = db->Fetch<Person>(&fetch_condition);

    ASSERT_EQ(1, fetched.size());
    EXPECT_EQ(1, fetched[0].id);
    EXPECT_EQ(L"a\\b", fetched[0].first_name);
}

TEST_F(DatabaseTest, LikeInsideAndCombinationStillMatchesWildcardsLiterally) {
    const auto db = Database::Instance();

    std::vector<Person> persons;
    persons.push_back({L"50% off", L"doe", 30, false, 1});
    persons.push_back({L"5000 off", L"doe", 30, false, 2});
    persons.push_back({L"50% off", L"roe", 40, false, 3});
    db->Save(persons);

    // Combining Like via And() clones it into a base QueryPredicate (see
    // QueryPredicate::Clone()); the ESCAPE clause and the already-escaped bound value must
    // both survive that clone for the match to stay literal here
    const auto fetch_condition = Like(&Person::first_name, L"50%").And(Equal(&Person::age, 30));
    const auto fetched = db->Fetch<Person>(&fetch_condition);

    ASSERT_EQ(1, fetched.size());
    EXPECT_EQ(1, fetched[0].id);
}

TEST_F(DatabaseTest, LikeWithoutWildcardsStillMatchesSubstring) {
    const auto db = Database::Instance();

    std::vector<Person> persons;
    persons.push_back({L"hello world", L"doe", 30, false, 1});
    persons.push_back({L"goodbye", L"doe", 30, false, 2});
    db->Save(persons);

    // Regression: a value with no %/_/\ must still behave as a plain substring/contains match
    const auto fetch_condition = Like(&Person::first_name, L"hello");
    const auto fetched = db->Fetch<Person>(&fetch_condition);

    ASSERT_EQ(1, fetched.size());
    EXPECT_EQ(1, fetched[0].id);
    EXPECT_EQ(L"hello world", fetched[0].first_name);
}

TEST_F(DatabaseTest, FetchPreservesInt64ValuesBeyondInt32Range) {
    const auto db = Database::Instance();

    // Values above INT32_MAX must survive a round-trip; sqlite3_column_int would
    // truncate/wrap these to 32 bits
    const int64_t above_int32_max = 5000000000LL;
    const int64_t near_int64_max = 9000000000000000000LL;

    std::vector<Company> company;
    company.push_back({L"Big Corp", above_int32_max, L"Nowhere", 1.0, 1});
    company.push_back({L"Huge Corp", near_int64_max, L"Nowhere", 1.0, 2});

    db->Save(company);

    const auto fetched_first = db->Fetch<Company>(1);
    EXPECT_EQ(above_int32_max, fetched_first.age);

    const auto fetched_second = db->Fetch<Company>(2);
    EXPECT_EQ(near_int64_max, fetched_second.age);
}

TEST_F(DatabaseTest, FetchPreservesHighPrecisionDoubleValues) {
    const auto db = Database::Instance();

    // std::to_wstring(double) formats with a fixed 6 decimal places, which would
    // silently drop precision here
    const double high_precision = 0.12345678901234567;
    const double large_magnitude = 123456789012345.67;

    std::vector<Company> company;
    company.push_back({L"Precision Inc", 1, L"Nowhere", high_precision, 1});
    company.push_back({L"Large Corp", 2, L"Nowhere", large_magnitude, 2});

    db->Save(company);

    const auto fetched_first = db->Fetch<Company>(1);
    EXPECT_DOUBLE_EQ(high_precision, fetched_first.salary);

    const auto fetched_second = db->Fetch<Company>(2);
    EXPECT_DOUBLE_EQ(large_magnitude, fetched_second.salary);
}

TEST_F(DatabaseTest, FetchPreservesNullAndEmptyStringSkipSemantics) {
    const auto db = Database::Instance();

    // Columns omitted from a raw INSERT are stored as SQL NULL. For NON-nullable members,
    // direct hydration must skip assignment for a NULL column, and must also skip assignment
    // for a TEXT column holding a genuine empty string. Both are only observable here for
    // wstring members: std::wstring's default constructor deterministically produces an empty
    // string regardless of whether it was assigned, whereas a skipped scalar member (e.g. an
    // omitted INTEGER column) keeps whatever indeterminate value T's default construction
    // happens to leave it at, so that case isn't asserted on here. Nullable members hydrate
    // NULL as an empty optional instead - see nullable_test.cc.
    //
    // Freshly created tables now carry NOT NULL on non-nullable columns, so a NULL can no
    // longer be inserted into them through this library's own schema. Recreate the table the
    // way a legacy database file (created before NOT NULL was emitted) or a foreign file
    // would look, which is exactly the scenario the skip semantics protect.
    db->UnsafeSql("DROP TABLE Company");
    db->UnsafeSql(
        "CREATE TABLE Company (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, age INTEGER, "
        "address TEXT, salary REAL)");
    db->UnsafeSql("INSERT INTO Company (id, name, age, salary) VALUES (1, '', 30, 50000.0)");
    db->UnsafeSql("INSERT INTO Company (id, age, address, salary) VALUES (2, 31, 'Nowhere', 60000.0)");

    const auto first = db->Fetch<Company>(1);
    EXPECT_EQ(L"", first.name);     // explicit empty string
    EXPECT_EQ(L"", first.address);  // omitted column -> SQL NULL
    EXPECT_EQ(30, first.age);
    EXPECT_EQ(50000.0, first.salary);

    const auto second = db->Fetch<Company>(2);
    EXPECT_EQ(L"", second.name);  // omitted column -> SQL NULL
    EXPECT_EQ(L"Nowhere", second.address);
    EXPECT_EQ(31, second.age);
    EXPECT_EQ(60000.0, second.salary);
}

TEST_F(DatabaseTest, FetchSkipsBlobColumnsWithoutThrowing) {
    const auto db = Database::Instance();

    // A BLOB value (reachable via raw SQL despite the column's declared affinity - here an
    // invalid UTF-8 byte) must be skipped exactly like a NULL, not fed into the typed
    // accessor for the member's declared storage class. The old text-based path returned an
    // empty string for any column whose runtime type wasn't INTEGER/FLOAT/TEXT (i.e. NULL or
    // BLOB) and Hydrate skipped assignment on that; direct hydration must replicate this or
    // else invalid UTF-8 blob bytes would throw out of FromUtf8 during a TEXT member's
    // hydration, where the old path silently tolerated the row.
    db->UnsafeSql("INSERT INTO Company (id, name, age, address, salary) VALUES (1, X'FF', 30, 'Nowhere', 50000.0)");

    Company fetched;
    EXPECT_NO_THROW(fetched = db->Fetch<Company>(1));
    EXPECT_EQ(L"", fetched.name);
    EXPECT_EQ(L"Nowhere", fetched.address);
    EXPECT_EQ(30, fetched.age);
    EXPECT_EQ(50000.0, fetched.salary);
}

TEST_F(DatabaseTest, FetchToleratesTableWithFewerColumnsThanCurrentStruct) {
    const auto db = Database::Instance();

    // Initialize() only ever runs CREATE TABLE IF NOT EXISTS, so a table created by an older
    // version of a reflected struct is never migrated to add newly introduced columns.
    // Simulate that drift directly (rather than needing a pre-existing file) by dropping
    // columns after the row is saved: SELECT * then returns fewer columns than
    // record.member_metadata.size(), which must not read past the statement's real column
    // count when hydrating.
    db->Save(Company{L"Old Corp", 40, L"Nowhere", 12345.0, 1});
    db->UnsafeSql("ALTER TABLE Company DROP COLUMN address");
    db->UnsafeSql("ALTER TABLE Company DROP COLUMN salary");

    Company fetched;
    EXPECT_NO_THROW(fetched = db->Fetch<Company>(1));
    EXPECT_EQ(L"Old Corp", fetched.name);
    EXPECT_EQ(40, fetched.age);
}

TEST_F(DatabaseTest, FetchAllRoundTripsLargeBatchExactly) {
    const auto db = Database::Instance();

    // Correctness-at-scale: a large FetchAll must still hydrate every row exactly, now that
    // hydration reads directly from the prepared statement instead of materializing the
    // whole result set as strings first
    constexpr int kRowCount = 50000;
    std::vector<Company> companies;
    companies.reserve(kRowCount);
    for (int i = 0; i < kRowCount; ++i) {
        Company c;
        c.id = i + 1;
        c.name = L"company_" + std::to_wstring(i);
        c.age = 5000000000LL + i;  // beyond INT32_MAX for every row
        c.address = L"address_" + std::to_wstring(i);
        c.salary = 0.1 + static_cast<double>(i) * 1e-9;  // needs full double precision
        companies.push_back(c);
    }

    db->Save(companies);

    const auto fetched = db->FetchAll<Company>();
    ASSERT_EQ(static_cast<size_t>(kRowCount), fetched.size());

    for (int i = 0; i < kRowCount; ++i) {
        EXPECT_EQ(companies[i].id, fetched[i].id);
        EXPECT_EQ(companies[i].name, fetched[i].name);
        EXPECT_EQ(companies[i].age, fetched[i].age);
        EXPECT_EQ(companies[i].address, fetched[i].address);
        EXPECT_DOUBLE_EQ(companies[i].salary, fetched[i].salary);
    }
}

TEST_F(DatabaseTest, RawSqlQueryForPersistedRecord) {
    const auto db = Database::Instance();

    std::vector<Person> persons;

    persons.push_back({L"johnie", L"appleseed", 28, false, 52});
    persons.push_back({L"mary", L"poppins", 20, false, 156});

    db->Save(persons);
    db->UnsafeSql("DELETE FROM Person WHERE length(first_name) <= 4");

    const auto fetched_persons = db->FetchAll<Person>();
    EXPECT_EQ(1, fetched_persons.size());
    EXPECT_EQ(52, fetched_persons[0].id);
    EXPECT_EQ(L"johnie", fetched_persons[0].first_name);
}
