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
#include "queries.h"

#include "person.h"
#include "pet.h"

using namespace sqlite_reflection;

TEST(ConditionsTest, EqualityInt) {
    Equal condition(&Person::id, (int64_t)65);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "id = 65"));
}

TEST(ConditionsTest, EqualityString) {
    Equal condition(&Person::first_name, std::wstring(L"john"));
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "first_name = 'john'"));
}

TEST(ConditionsTest, EqualityDouble) {
    Equal condition(&Pet::weight, 32.4);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "weight = 32.400000"));
}

TEST(ConditionsTest, InequalityInt) {
    Unequal condition(&Person::id, (int64_t)65);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "id != 65"));
}

TEST(ConditionsTest, InequalityString) {
    Unequal condition(&Person::first_name, std::wstring(L"john"));
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "first_name != 'john'"));
}

TEST(ConditionsTest, InequalityDouble) {
    Unequal condition(&Pet::weight, 32.4);
    auto evalution = condition.Evaluate();
    EXPECT_EQ(0, strcmp(evalution.data(), "weight != 32.400000"));
}
