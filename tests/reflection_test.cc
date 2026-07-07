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

#include "reflection.h"

#include <gtest/gtest.h>

#include <type_traits>

#include "company.h"
#include "datetime_container.h"
#include "person.h"
#include "pet.h"

using namespace sqlite_reflection;

// Reflectable records must not be polymorphic (see include/reflection.h's static_assert next to
// the REFLECTABLE struct definition). These compile-time checks pin that guarantee for every
// storage class the test records exercise (TEXT/wstring, INT, REAL, BOOL, DATETIME/TimePoint),
// so a regression that reintroduces a virtual function/inheritance into the macro-generated
// struct fails the build here rather than silently corrupting member offsets at runtime.
static_assert(!std::is_polymorphic<Person>::value, "Person must not be polymorphic");
static_assert(!std::is_polymorphic<Pet>::value, "Pet must not be polymorphic");
static_assert(!std::is_polymorphic<Company>::value, "Company must not be polymorphic");
static_assert(!std::is_polymorphic<DatetimeContainer>::value, "DatetimeContainer must not be polymorphic");

// Mirrors the static_asserts above as ordinary runtime expectations, so the guarantee is also
// visible in normal test output rather than only enforced silently at compile time.
TEST(ReflectionTest, ReflectableRecordsAreNotPolymorphic) {
    EXPECT_FALSE(std::is_polymorphic<Person>::value);
    EXPECT_FALSE(std::is_polymorphic<Pet>::value);
    EXPECT_FALSE(std::is_polymorphic<Company>::value);
    EXPECT_FALSE(std::is_polymorphic<DatetimeContainer>::value);
}

// Mirrors the stronger is_standard_layout static_assert in include/reflection.h (attempted
// separately from the polymorphic guard, since it's only verified on libstdc++/Linux so far -
// see #21). If that assert is ever removed because a CI platform can't satisfy it, this test
// should be removed alongside it rather than weakened to keep passing.
TEST(ReflectionTest, ReflectableRecordsAreStandardLayout) {
    EXPECT_TRUE(std::is_standard_layout<Person>::value);
    EXPECT_TRUE(std::is_standard_layout<Pet>::value);
    EXPECT_TRUE(std::is_standard_layout<Company>::value);
    EXPECT_TRUE(std::is_standard_layout<DatetimeContainer>::value);
}
