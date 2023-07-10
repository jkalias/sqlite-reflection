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
	const Equal condition(&Person::id, (int64_t)65);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "id = 65"));
}

TEST(QueryPredicatesTest, EqualityString) {
	const Equal condition(&Person::first_name, std::wstring(L"john"));
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "first_name = 'john'"));
}

TEST(QueryPredicatesTest, EqualityDouble) {
	const Equal condition(&Pet::weight, 32.4);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "weight = 32.4"));
}

TEST(QueryPredicatesTest, InequalityInt) {
	const Unequal condition(&Person::id, (int64_t)65);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "id != 65"));
}

TEST(QueryPredicatesTest, InequalityString) {
	const Unequal condition(&Person::first_name, std::wstring(L"john"));
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "first_name != 'john'"));
}

TEST(QueryPredicatesTest, InequalityDouble) {
	const Unequal condition(&Pet::weight, 32.4);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "weight != 32.4"));
}

TEST(QueryPredicatesTest, GreaterThanInt) {
	const GreaterThan condition(&Person::id, 65);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "id > 65"));
}

TEST(QueryPredicatesTest, GreaterThanOrEqualInt) {
	const GreaterThanOrEqual condition(&Person::id, 65);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "id >= 65"));
}

TEST(QueryPredicatesTest, GreaterThanDouble) {
	const GreaterThan condition(&Pet::weight, 32.4);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "weight > 32.4"));
}

TEST(QueryPredicatesTest, GreaterThanOrEqualDouble) {
	const GreaterThanOrEqual condition(&Pet::weight, 32.4);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "weight >= 32.4"));
}

TEST(QueryPredicatesTest, SmallerThanInt) {
	const SmallerThan condition(&Person::id, 65);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "id < 65"));
}

TEST(QueryPredicatesTest, SmallerThanEqualInt) {
	const SmallerThanOrEqual condition(&Person::id, 65);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "id <= 65"));
}

TEST(QueryPredicatesTest, SmallerThanDouble) {
	const SmallerThan condition(&Pet::weight, 32.4);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "weight < 32.4"));
}

TEST(QueryPredicatesTest, SmallerThanOrEqualDouble) {
	const SmallerThanOrEqual condition(&Pet::weight, 32.4);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "weight <= 32.4"));
}

TEST(QueryPredicatesTest, And) {
	const Unequal c1(&Person::id, (int64_t)65);
	const Equal c2(&Person::first_name, std::wstring(L"john"));
	const AndPredicate c3(c1, c2);
	const auto evalution = c3.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "(id != 65 AND first_name = 'john')"));
}

TEST(QueryPredicatesTest, Or) {
	const Unequal c1(&Person::id, (int64_t)65);
	const Equal c2(&Person::first_name, std::wstring(L"john"));
	const OrPredicate c3(c1, c2);
	const auto evalution = c3.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "(id != 65 OR first_name = 'john')"));
}

TEST(QueryPredicatesTest, PredicateChaining) {
	const auto predicate = Equal(&Person::id, (int64_t)65)
	                       .Or(Equal(&Person::first_name, std::wstring(L"john")))
	                       .And(Unequal(&Person::last_name, std::wstring(L"appleseed")));

	const auto evaluation = predicate.Evaluate();

	EXPECT_EQ(0, strcmp(evaluation.data(), "((id = 65 OR first_name = 'john') AND last_name != 'appleseed')"));
}
