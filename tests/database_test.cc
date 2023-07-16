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
#include "company.h"

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

	const Person p{L"παναγιώτης", L"ανδριανόπουλος", 39, 1};
	db.Save(p);

	const auto all_persons = db.FetchAll<Person>();
	EXPECT_EQ(1, all_persons.size());

	EXPECT_EQ(p.first_name, all_persons[0].first_name);
	EXPECT_EQ(p.last_name, all_persons[0].last_name);
	EXPECT_EQ(p.age, all_persons[0].age);
	EXPECT_EQ(p.id, all_persons[0].id);
}

TEST_F(DatabaseTest, SingleInsertionWithAutoIdIncrement) {
    const auto& db = Database::Instance();

    const Person p{L"παναγιώτης", L"ανδριανόπουλος", 39};
    db.SaveAutoIncrement(p);

    const auto all_persons = db.FetchAll<Person>();
    EXPECT_EQ(1, all_persons.size());

    EXPECT_EQ(p.first_name, all_persons[0].first_name);
    EXPECT_EQ(p.last_name, all_persons[0].last_name);
    EXPECT_EQ(p.age, all_persons[0].age);
    EXPECT_EQ(1, all_persons[0].id);
}

TEST_F(DatabaseTest, MultipleInsertions) {
	const auto& db = Database::Instance();

	std::vector<Person> persons;

	persons.push_back({L"παναγιώτης", L"ανδριανόπουλος", 28, false, 3});
	persons.push_back({L"peter", L"meier", 32, true, 5});

	db.Save(persons);

	const auto saved_persons = db.FetchAll<Person>();
	EXPECT_EQ(2, saved_persons.size());

	for (auto i = 0; i < saved_persons.size(); ++i) {
		EXPECT_EQ(persons[i].id, saved_persons[i].id);
		EXPECT_EQ(persons[i].first_name, saved_persons[i].first_name);
		EXPECT_EQ(persons[i].last_name, saved_persons[i].last_name);
		EXPECT_EQ(persons[i].age, saved_persons[i].age);
        EXPECT_EQ(persons[i].isVaccinated, saved_persons[i].isVaccinated);
	}
}

TEST_F(DatabaseTest, InsertionOnOneTypeDoesNotAffectOtherType) {
	const auto& db = Database::Instance();

	const Person p{L"παναγιώτης", L"ανδριανόπουλος", 39, 1};
	db.Save(p);

	const auto all_pets = db.FetchAll<Pet>();
	EXPECT_EQ(0, all_pets.size());
}

TEST_F(DatabaseTest, SingleUpdate) {
	const auto& db = Database::Instance();

	Person p{L"παναγιώτης", L"ανδριανόπουλος", 39, 1};
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

    persons.push_back({L"john", L"doe", 28, false, 3});
	persons.push_back({L"mary", L"poppins", 20, false, 5});

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

	persons.push_back({L"παναγιώτης", L"ανδριανόπουλος", 28, false, 3});
	persons.push_back({L"peter", L"meier", 32, false, 5});
	persons.push_back({L"mary", L"poppins", 20, false, 13});

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

	persons.push_back({L"παναγιώτης", L"ανδριανόπουλος", 28, false, 3});
	persons.push_back({L"peter", L"meier", 32, false, 5});
	persons.push_back({L"mary", L"poppins", 20, false, 13});

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

TEST_F(DatabaseTest, DeleteWithPredicate) {
    const auto& db = Database::Instance();

    std::vector<Person> persons;

    persons.push_back({L"παναγιώτης", L"ανδριανόπουλος", 28, true, 3});
    persons.push_back({L"peter", L"meier", 32, false, 5});
    persons.push_back({L"mary", L"poppins", 20, true, 13});

    db.Save(persons);

    const auto age_match_predicate = SmallerThan(&Person::age, 30)
        .And(Equal(&Person::isVaccinated, true));

    db.Delete<Person>(&age_match_predicate);
    const auto fetched_persons = db.FetchAll<Person>();
    EXPECT_EQ(1, fetched_persons.size());

    EXPECT_EQ(5, fetched_persons[0].id);
    EXPECT_EQ(L"peter", fetched_persons[0].first_name);
    EXPECT_EQ(L"meier", fetched_persons[0].last_name);
    EXPECT_EQ(32, fetched_persons[0].age);
    EXPECT_EQ(false, fetched_persons[0].isVaccinated);
}

TEST_F(DatabaseTest, SingleFetch) {
	const auto& db = Database::Instance();

	std::vector<Person> persons;

	persons.push_back({L"παναγιώτης", L"ανδριανόπουλος", 28, false, 3});
	persons.push_back({L"peter", L"meier", 32, false, 5});
	persons.push_back({L"mary", L"poppins", 20, false, 13});

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

	persons.push_back({L"παναγιώτης", L"ανδριανόπουλος", 28, false, 3});
	persons.push_back({L"peter", L"meier", 32, false, 5});
	persons.push_back({L"mary", L"poppins", 20, false});

	db.Save(persons);

	EXPECT_ANY_THROW(db.Fetch<Person>(15));
}

TEST_F(DatabaseTest, FetchWithSimilarPredicateString) {
	const auto& db = Database::Instance();

	std::vector<Company> company;

	company.push_back({L"Paul", 32, L"California", 20000.0, 1});
	company.push_back({L"Allen", 25, L"Texas", 15000.0, 2});
	company.push_back({L"Teddy", 23, L"Norway", 20000.0, 3});
	company.push_back({L"Mark", 25, L"Rich-Mond", 65000.0, 4});
	company.push_back({L"David", 27, L"Texas", 85000.0, 5});
	company.push_back({L"Kim", 22, L"South-Hall", 45000.0, 6});
	company.push_back({L"Janes", 24, L"Houston", 10000.0, 7});

	db.Save(company);

	auto fetch_condition = Like(&Company::address, L"-");

	auto fetched_persons = db.Fetch<Company>(&fetch_condition);
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
	const auto& db = Database::Instance();

	std::vector<Company> company;

	company.push_back({L"Paul", 32, L"California", 20000.0, 1});
	company.push_back({L"Allen", 25, L"Texas", 15000.0, 2});
	company.push_back({L"Teddy", 23, L"Norway", 20000.0, 3});
	company.push_back({L"Mark", 25, L"Rich-Mond", 65000.0, 4});
	company.push_back({L"David", 27, L"Texas", 85000.0, 5});
	company.push_back({L"Kim", 22, L"South-Hall", 45000.0, 6});
	company.push_back({L"Janes", 24, L"Houston", 10000.0, 7});

	db.Save(company);

	auto fetch_condition = Like(&Company::salary, 5000.0);

	auto fetched_persons = db.Fetch<Company>(&fetch_condition);
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
	const auto& db = Database::Instance();

	std::vector<Company> company;

	company.push_back({L"Paul", 32, L"California", 20000.0, 1});
	company.push_back({L"Allen", 25, L"Texas", 15000.0, 2});
	company.push_back({L"Teddy", 23, L"Norway", 20000.0, 3});
	company.push_back({L"Mark", 25, L"Rich-Mond", 65000.0, 4});
	company.push_back({L"David", 27, L"Texas", 85000.0, 5});
	company.push_back({L"Kim", 22, L"South-Hall", 45000.0, 6});
	company.push_back({L"Janes", 24, L"Houston", 10000.0, 7});

	db.Save(company);

	const auto fetch_condition = Like(&Company::age, 7);

	const auto fetched_persons = db.Fetch<Company>(&fetch_condition);
	EXPECT_EQ(1, fetched_persons.size());

	EXPECT_EQ(5, fetched_persons[0].id);
	EXPECT_EQ(L"David", fetched_persons[0].name);
	EXPECT_EQ(27, fetched_persons[0].age);
	EXPECT_EQ(L"Texas", fetched_persons[0].address);
	EXPECT_EQ(85000, fetched_persons[0].salary);
}

TEST_F(DatabaseTest, FetchWithPredicateChaining) {
	const auto& db = Database::Instance();

	std::vector<Person> persons;

	persons.push_back({L"name1", L"surname1", 13, false, 1});
	persons.push_back({L"john", L"surname2", 25, false, 2});
	persons.push_back({L"john", L"surname3", 37, false, 3});
	persons.push_back({L"jame", L"surname4", 45, false, 4});
	persons.push_back({L"name5", L"surname5", 56, false, 5});

	db.Save(persons);

	const auto fetch_condition = GreaterThanOrEqual(&Person::id, 2)
	                             .And(SmallerThan(&Person::id, 5))
	                             .And(Equal(&Person::first_name, L"john"));

	const auto fetched_persons = db.Fetch<Person>(&fetch_condition);
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

TEST_F(DatabaseTest, ReadMaxId) {
	const auto& db = Database::Instance();

	std::vector<Person> persons;

	persons.push_back({L"john", L"appleseed", 28, false, 54});
	persons.push_back({L"mary", L"poppins", 20, false, 156});

	db.Save(persons);

	const auto max_id_person = db.GetMaxId<Person>();
	EXPECT_EQ(156, max_id_person);

	const auto max_id_pet = db.GetMaxId<Pet>();
	EXPECT_EQ(0, max_id_pet);
}

TEST_F(DatabaseTest, RawSqlQueryForPersistedRecord) {
    const auto& db = Database::Instance();

    std::vector<Person> persons;

    persons.push_back({L"johnie", L"appleseed", 28, false, 52});
    persons.push_back({L"mary", L"poppins", 20, false, 156});

    db.Save(persons);
    db.Sql("DELETE FROM Person WHERE length(first_name) <= 4");
    
    const auto fetched_persons = db.FetchAll<Person>();
    EXPECT_EQ(1, fetched_persons.size());
    EXPECT_EQ(52, fetched_persons[0].id);
    EXPECT_EQ(L"johnie", fetched_persons[0].first_name);
}
