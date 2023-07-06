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
#include "database.h"

#include "person.h"
#include "pet.h"

using namespace sqlite_reflection;

class DatabaseTest : public ::testing::Test
{
	void SetUp() override {
		Database::Initialize("");
	}

	void TearDown() override {
		Database::Finalize();
	}
};

TEST_F(DatabaseTest, Initialization) {
	const auto& db = Database::Instance();

	const auto all_persons = db.FetchAll<Person>();
	EXPECT_EQ(0, all_persons.size());

	const auto all_pets = db.FetchAll<Pet>();
	EXPECT_EQ(0, all_pets.size());
}

TEST_F(DatabaseTest, SingleInsertion) {
	const auto& db = Database::Instance();

	Person p{1, L"παναγιώτης", L"ανδριανόπουλος", 39};
	db.Save(p);

	const auto all_persons = db.FetchAll<Person>();
	EXPECT_EQ(1, all_persons.size());

	EXPECT_EQ(p.first_name, all_persons[0].first_name);
	EXPECT_EQ(p.last_name, all_persons[0].last_name);
	EXPECT_EQ(p.age, all_persons[0].age);
	EXPECT_EQ(p.id, all_persons[0].id);
}

TEST_F(DatabaseTest, MultipleInsertions) {
	const auto& db = Database::Instance();

	std::vector<Person> persons;

	persons.push_back({3, L"παναγιώτης", L"ανδριανόπουλος", 28});
	persons.push_back({5, L"peter", L"meier", 32});

	db.Save(persons);

	const auto saved_persons = db.FetchAll<Person>();
	EXPECT_EQ(2, saved_persons.size());

	for (auto i = 0; i < saved_persons.size(); ++i) {
		EXPECT_EQ(persons[i].id, saved_persons[i].id);
		EXPECT_EQ(persons[i].first_name, saved_persons[i].first_name);
		EXPECT_EQ(persons[i].last_name, saved_persons[i].last_name);
		EXPECT_EQ(persons[i].age, saved_persons[i].age);
	}
}

TEST_F(DatabaseTest, InsertionOnOneTypeDoesNotAffectOtherType) {
	const auto& db = Database::Instance();

	Person p{1, L"παναγιώτης", L"ανδριανόπουλος", 39};
	db.Save(p);

	const auto all_pets = db.FetchAll<Pet>();
	EXPECT_EQ(0, all_pets.size());
}

TEST_F(DatabaseTest, SingleUpdate) {
	const auto& db = Database::Instance();

	Person p{1, L"παναγιώτης", L"ανδριανόπουλος", 39};
	db.Save(p);

	p.age = 23;
	p.first_name = L"max";

	db.Update(p);

	const auto all_persons = db.FetchAll<Person>();
	EXPECT_EQ(1, all_persons.size());

	EXPECT_EQ(p.first_name, all_persons[0].first_name);
	EXPECT_EQ(p.last_name, all_persons[0].last_name);
	EXPECT_EQ(p.age, all_persons[0].age);
	EXPECT_EQ(p.id, all_persons[0].id);
}

TEST_F(DatabaseTest, MultipleUpdates) {
	const auto& db = Database::Instance();

	std::vector<Person> persons;

	persons.push_back({3, L"john", L"doe", 28});
	persons.push_back({5, L"mary", L"poppins", 20});

	db.Save(persons);

	persons[0].last_name = L"rambo";
	persons[1].age = 20;

	db.Update(persons);

	const auto saved_persons = db.FetchAll<Person>();
	EXPECT_EQ(2, saved_persons.size());

	for (auto i = 0; i < saved_persons.size(); ++i) {
		EXPECT_EQ(persons[i].id, saved_persons[i].id);
		EXPECT_EQ(persons[i].first_name, saved_persons[i].first_name);
		EXPECT_EQ(persons[i].last_name, saved_persons[i].last_name);
		EXPECT_EQ(persons[i].age, saved_persons[i].age);
	}
}

TEST_F(DatabaseTest, DeleteWithRecord) {
	const auto& db = Database::Instance();

	std::vector<Person> persons;

	persons.push_back({3, L"παναγιώτης", L"ανδριανόπουλος", 28});
	persons.push_back({5, L"peter", L"meier", 32});
	persons.push_back({13, L"mary", L"poppins", 20});

	db.Save(persons);

	auto saved_persons = db.FetchAll<Person>();
	EXPECT_EQ(3, saved_persons.size());

	db.Delete(persons[1]);
	saved_persons = db.FetchAll<Person>();
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
	const auto& db = Database::Instance();

	std::vector<Person> persons;

	persons.push_back({3, L"παναγιώτης", L"ανδριανόπουλος", 28});
	persons.push_back({5, L"peter", L"meier", 32});
	persons.push_back({13, L"mary", L"poppins", 20});

	db.Save(persons);

	auto saved_persons = db.FetchAll<Person>();
	EXPECT_EQ(3, saved_persons.size());

	db.Delete<Person>(persons[1].id);
	saved_persons = db.FetchAll<Person>();
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

TEST_F(DatabaseTest, SingleFetch) {
	const auto& db = Database::Instance();

	std::vector<Person> persons;

	persons.push_back({3, L"παναγιώτης", L"ανδριανόπουλος", 28});
	persons.push_back({5, L"peter", L"meier", 32});
	persons.push_back({13, L"mary", L"poppins", 20});

	db.Save(persons);

	const auto fetched_person = db.Fetch<Person>(5);
	EXPECT_EQ(5, fetched_person.id);
	EXPECT_EQ(L"peter", fetched_person.first_name);
	EXPECT_EQ(L"meier", fetched_person.last_name);
	EXPECT_EQ(32, fetched_person.age);
}

TEST_F(DatabaseTest, SingleFetchWithoutExistingRecordExpectingException) {
	const auto& db = Database::Instance();

	std::vector<Person> persons;

	persons.push_back({3, L"παναγιώτης", L"ανδριανόπουλος", 28});
	persons.push_back({5, L"peter", L"meier", 32});
	persons.push_back({13, L"mary", L"poppins", 20});

	db.Save(persons);

	EXPECT_ANY_THROW(db.Fetch<Person>(15));
}

TEST_F(DatabaseTest, FetchWithCustomCondition) {
    const auto& db = Database::Instance();

    std::vector<Person> persons;

    persons.push_back({1, L"name1", L"surname1", 13});
    persons.push_back({2, L"john", L"surname2", 25});
    persons.push_back({3, L"john", L"surname3", 37});
    persons.push_back({4, L"jame", L"surname4", 45});
    persons.push_back({5, L"name5", L"surname5", 56});

    db.Save(persons);
    
    auto fetch_condition = GreaterThanOrEqual(&Person::id, 2)
        .And(SmallerThan(&Person::id, 5))
        .And(Equal(&Person::first_name, std::wstring(L"john")));
    
    auto fetched_persons = db.Fetch<Person>(fetch_condition);
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
