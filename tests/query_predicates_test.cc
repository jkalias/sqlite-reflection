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
#include "query_predicates.h"

#include "person.h"
#include "pet.h"

using namespace sqlite_reflection;

TEST(QueryPredicatesTest, EqualityInt) {
    Equal condition(&Person::id, (int64_t)65);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "id = 65"));
}

TEST(QueryPredicatesTest, EqualityString) {
    Equal condition(&Person::first_name, std::wstring(L"john"));
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "first_name = 'john'"));
}

TEST(QueryPredicatesTest, EqualityDouble) {
    Equal condition(&Pet::weight, 32.4);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "weight = 32.4"));
}

TEST(QueryPredicatesTest, InequalityInt) {
    Unequal condition(&Person::id, (int64_t)65);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "id != 65"));
}

TEST(QueryPredicatesTest, InequalityString) {
    Unequal condition(&Person::first_name, std::wstring(L"john"));
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "first_name != 'john'"));
}

TEST(QueryPredicatesTest, InequalityDouble) {
    Unequal condition(&Pet::weight, 32.4);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "weight != 32.4"));
}

TEST(QueryPredicatesTest, GreaterThanInt) {
    GreaterThan condition(&Person::id, (int64_t)65);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "id > 65"));
}

TEST(QueryPredicatesTest, GreaterThanOrEqualInt) {
    GreaterThanOrEqual condition(&Person::id, (int64_t)65);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "id >= 65"));
}

TEST(QueryPredicatesTest, GreaterThanDouble) {
    GreaterThan condition(&Pet::weight, 32.4);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "weight > 32.4"));
}

TEST(QueryPredicatesTest, GreaterThanOrEqualDouble) {
    GreaterThanOrEqual condition(&Pet::weight, 32.4);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "weight >= 32.4"));
}

TEST(QueryPredicatesTest, SmallerThanInt) {
    SmallerThan condition(&Person::id, (int64_t)65);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "id < 65"));
}

TEST(QueryPredicatesTest, SmallerThanEqualInt) {
    SmallerThanOrEqual condition(&Person::id, (int64_t)65);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "id <= 65"));
}

TEST(QueryPredicatesTest, SmallerThanDouble) {
    SmallerThan condition(&Pet::weight, 32.4);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "weight < 32.4"));
}

TEST(QueryPredicatesTest, SmallerThanOrEqualDouble) {
    SmallerThanOrEqual condition(&Pet::weight, 32.4);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "weight <= 32.4"));
}

TEST(QueryPredicatesTest, And) {
    Unequal c1(&Person::id, (int64_t)65);
    Equal c2(&Person::first_name, std::wstring(L"john"));
    AndPredicate c3(c1, c2);
    auto evalution = c3.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "(id != 65 AND first_name = 'john')"));
}

TEST(QueryPredicatesTest, Or) {
    Unequal c1(&Person::id, (int64_t)65);
    Equal c2(&Person::first_name, std::wstring(L"john"));
    OrPredicate c3(c1, c2);
    auto evalution = c3.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "(id != 65 OR first_name = 'john')"));
}

TEST(QueryPredicatesTest, ConditionChaining) {
    auto evaluation = Equal(&Person::id, (int64_t)65)
        .Or(Equal(&Person::first_name, std::wstring(L"john")))
        .And(Unequal(&Person::last_name, std::wstring(L"appleseed")))
        .Evaluate();
    
    EXPECT_EQ(0, strcmp(evaluation.data(), "((id = 65 OR first_name = 'john') AND last_name != 'appleseed')"));
}