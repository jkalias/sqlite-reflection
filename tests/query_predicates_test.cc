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
#include "query_predicates.h"

#include "person.h"
#include "pet.h"

using namespace sqlite_reflection;

TEST(QueryPredicatesTest, EqualityInt) {
	const Equal condition(&Person::id, 65);
	const auto evalution = condition.Evaluate();
	const auto bindings = condition.Bindings();
	EXPECT_EQ(0, strcmp(evalution.data(), "id = ?"));
	ASSERT_EQ(1, bindings.size());
	EXPECT_EQ(SqliteStorageClass::kInt, bindings[0].storage_class);
	EXPECT_EQ(65, bindings[0].int_value);
}

TEST(QueryPredicatesTest, EqualityString) {
	const Equal condition(&Person::first_name, L"john");
	const auto evalution = condition.Evaluate();
	const auto bindings = condition.Bindings();
	EXPECT_EQ(0, strcmp(evalution.data(), "first_name = ?"));
	ASSERT_EQ(1, bindings.size());
	EXPECT_EQ(SqliteStorageClass::kText, bindings[0].storage_class);
	EXPECT_EQ("john", bindings[0].text_value);
}

TEST(QueryPredicatesTest, EqualityDouble) {
	const Equal condition(&Pet::weight, 32.4);
	const auto evalution = condition.Evaluate();
	const auto bindings = condition.Bindings();
	EXPECT_EQ(0, strcmp(evalution.data(), "weight = ?"));
	ASSERT_EQ(1, bindings.size());
	EXPECT_EQ(SqliteStorageClass::kReal, bindings[0].storage_class);
	EXPECT_EQ(32.4, bindings[0].real_value);
}

TEST(QueryPredicatesTest, InequalityInt) {
	const Unequal condition(&Person::id, 65);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "id != ?"));
}

TEST(QueryPredicatesTest, InequalityString) {
	const Unequal condition(&Person::first_name, L"john");
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "first_name != ?"));
}

TEST(QueryPredicatesTest, InequalityDouble) {
	const Unequal condition(&Pet::weight, 32.4);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "weight != ?"));
}

TEST(QueryPredicatesTest, GreaterThanInt) {
	const GreaterThan condition(&Person::id, 65);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "id > ?"));
}

TEST(QueryPredicatesTest, GreaterThanOrEqualInt) {
	const GreaterThanOrEqual condition(&Person::id, 65);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "id >= ?"));
}

TEST(QueryPredicatesTest, GreaterThanDouble) {
	const GreaterThan condition(&Pet::weight, 32.4);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "weight > ?"));
}

TEST(QueryPredicatesTest, GreaterThanOrEqualDouble) {
	const GreaterThanOrEqual condition(&Pet::weight, 32.4);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "weight >= ?"));
}

TEST(QueryPredicatesTest, SmallerThanInt) {
	const SmallerThan condition(&Person::id, 65);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "id < ?"));
}

TEST(QueryPredicatesTest, SmallerThanEqualInt) {
	const SmallerThanOrEqual condition(&Person::id, 65);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "id <= ?"));
}

TEST(QueryPredicatesTest, SmallerThanDouble) {
	const SmallerThan condition(&Pet::weight, 32.4);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "weight < ?"));
}

TEST(QueryPredicatesTest, SmallerThanOrEqualDouble) {
	const SmallerThanOrEqual condition(&Pet::weight, 32.4);
	const auto evalution = condition.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "weight <= ?"));
}

TEST(QueryPredicatesTest, And) {
	const Unequal c1(&Person::id, 65);
	const Equal c2(&Person::first_name, L"john");
	const AndPredicate c3(c1, c2);
	const auto evalution = c3.Evaluate();
	const auto bindings = c3.Bindings();
	EXPECT_EQ(0, strcmp(evalution.data(), "(id != ? AND first_name = ?)"));
	ASSERT_EQ(2, bindings.size());
	EXPECT_EQ(65, bindings[0].int_value);
	EXPECT_EQ("john", bindings[1].text_value);
}

TEST(QueryPredicatesTest, Or) {
	const Unequal c1(&Person::id, 65);
	const Equal c2(&Person::first_name, L"john");
	const OrPredicate c3(c1, c2);
	const auto evalution = c3.Evaluate();
	EXPECT_EQ(0, strcmp(evalution.data(), "(id != ? OR first_name = ?)"));
}

TEST(QueryPredicatesTest, PredicateChaining) {
	const auto predicate = Equal(&Person::id, 65)
	                       .Or(Equal(&Person::first_name, L"john"))
	                       .And(Unequal(&Person::last_name, L"appleseed"));

	const auto evaluation = predicate.Evaluate();

	const auto bindings = predicate.Bindings();
	EXPECT_EQ(0, strcmp(evaluation.data(), "((id = ? OR first_name = ?) AND last_name != ?)"));
	ASSERT_EQ(3, bindings.size());
	EXPECT_EQ(65, bindings[0].int_value);
	EXPECT_EQ("john", bindings[1].text_value);
	EXPECT_EQ("appleseed", bindings[2].text_value);
}

TEST(QueryPredicatesTest, StringInjectionPayloadStaysInBindings) {
	const Equal condition(&Person::first_name, L"john' OR 1=1 --");
	const auto evalution = condition.Evaluate();
	const auto bindings = condition.Bindings();

	EXPECT_EQ(0, strcmp(evalution.data(), "first_name = ?"));
	ASSERT_EQ(1, bindings.size());
	EXPECT_EQ("john' OR 1=1 --", bindings[0].text_value);
}

TEST(QueryPredicatesTest, LikePayloadStaysInBindings) {
	const Like condition(&Person::first_name, L"john%' OR 1=1 --");
	const auto evalution = condition.Evaluate();
	const auto bindings = condition.Bindings();

	EXPECT_EQ(0, strcmp(evalution.data(), "first_name LIKE ?"));
	ASSERT_EQ(1, bindings.size());
	EXPECT_EQ("%john%' OR 1=1 --%", bindings[0].text_value);
}
