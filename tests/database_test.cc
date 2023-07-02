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

class DatabaseTest: public ::testing::Test {
    void SetUp() override {
        Database::Initialize("");
    }
    
    void TearDown() override {
        Database::Finalize();
    }
};

TEST_F(DatabaseTest, Initialization)
{
    const auto &db = Database::Instance();
    
    const auto all_persons = db.FetchAll<Person>();
    EXPECT_EQ(0, all_persons.size());
    
    const auto all_pets = db.FetchAll<Pet>();
    EXPECT_EQ(0, all_pets.size());
}

TEST_F(DatabaseTest, SingleInsertion)
{
    const auto &db = Database::Instance();
    
    Person p {1, L"παναγιώτης", L"ανδριανόπουλος", 39};
    db.Save(p);
    
    const auto all_persons = db.FetchAll<Person>();
    EXPECT_EQ(1, all_persons.size());
    
    EXPECT_EQ(p.first_name, all_persons[0].first_name);
    EXPECT_EQ(p.last_name, all_persons[0].last_name);
    EXPECT_EQ(p.age, all_persons[0].age);
    EXPECT_EQ(p.id, all_persons[0].id);
}

TEST_F(DatabaseTest, MultipleInsertions)
{
    const auto &db = Database::Instance();
    
    std::vector<Person> persons;
    
    persons.push_back({ 3, L"παναγιώτης", L"ανδριανόπουλος", 28 });
    persons.push_back({ 5, L"peter", L"meier", 32} );
    
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
