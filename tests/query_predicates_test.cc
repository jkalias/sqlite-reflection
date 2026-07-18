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

#include "query_predicates.h"

#include <gtest/gtest.h>

#include <string>
#include <utility>

#include "nullable_record.h"
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
    const auto predicate =
        Equal(&Person::id, 65).Or(Equal(&Person::first_name, L"john")).And(Unequal(&Person::last_name, L"appleseed"));

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

    EXPECT_EQ(0, strcmp(evalution.data(), R"(first_name LIKE ? ESCAPE '\')"));
    ASSERT_EQ(1, bindings.size());
    // The literal '%' in the payload is escaped, since it is caller-supplied text, not an
    // intentional wildcard
    EXPECT_EQ(R"(%john\%' OR 1=1 --%)", bindings[0].text_value);
}

TEST(QueryPredicatesTest, LikeEscapesWildcardsAndEmitsEscapeClause) {
    const Like condition(&Person::first_name, L"50%_a\\b");
    const auto evaluation = condition.Evaluate();
    const auto bindings = condition.Bindings();

    EXPECT_EQ(0, strcmp(evaluation.data(), R"(first_name LIKE ? ESCAPE '\')"));
    ASSERT_EQ(1, bindings.size());
    // %, _ and \ in the caller's value are each escaped with a backslash before the outer
    // "contains" wildcards are added
    EXPECT_EQ(R"(%50\%\_a\\b%)", bindings[0].text_value);
}

TEST(QueryPredicatesTest, LikeInsideAndSurvivesCloneWithEscapeClause) {
    // BinaryPredicate stores Clone()d operands, and QueryPredicate::Clone() returns a base
    // QueryPredicate rather than a Like - the ESCAPE clause must therefore come from
    // QueryPredicate::Evaluate() itself (keyed on symbol_ == "LIKE") to survive this, not from
    // a Like-only Evaluate() override
    const auto condition = Like(&Person::first_name, L"50%").And(Equal(&Person::age, 30));
    const auto evaluation = condition.Evaluate();
    const auto bindings = condition.Bindings();

    EXPECT_EQ(0, strcmp(evaluation.data(), R"((first_name LIKE ? ESCAPE '\' AND age = ?))"));
    ASSERT_EQ(2, bindings.size());
    EXPECT_EQ(R"(%50\%%)", bindings[0].text_value);
    EXPECT_EQ(30, bindings[1].int_value);
}

namespace {
// A plain struct that is deliberately never run through the REFLECTABLE/FIELDS registration
// macros, so its type id never appears in the reflection registry.
struct UnregisteredRecord {
    int64_t id;
    int64_t value;
};

// A plain struct that is also never run through the registration macros, but is manually and
// incompletely registered below (name only, no member metadata) to exercise the
// registered-but-offset-mismatch guard, as distinct from the unregistered-type guard above.
struct MismatchedRecord {
    int64_t id;
    int64_t value;
};

// Erases a hand-inserted entry from the process-wide reflection registry on scope exit. Without
// this, a MismatchedRecord-shaped entry with no member metadata would linger in the registry for
// the rest of the test binary: Database::Database iterates every registered record and would
// generate "CREATE TABLE IF NOT EXISTS MismatchedRecord ();" (empty column list) for it, failing
// every later Database::Initialize() call in this process.
class ScopedRegistryCleanup {
public:
    explicit ScopedRegistryCleanup(std::string type_id) : type_id_(std::move(type_id)) {}
    ~ScopedRegistryCleanup() {
        GetReflectionRegisterInstance()->records.erase(type_id_);
    }

private:
    std::string type_id_;
};
}  // namespace

TEST(QueryPredicatesTest, PredicateConstructionThrowsForUnregisteredType) {
    // #23: GetRecordFromTypeId must fail fast for a type that was never registered, instead of
    // std::map::operator[] silently default-inserting an empty Reflection (empty table name, no
    // columns), which would otherwise surface later as an opaque SQLite prepare error
    EXPECT_THROW(Equal(&UnregisteredRecord::value, 42), std::runtime_error);
}

TEST(QueryPredicatesTest, PredicateConstructionThrowsWhenNoMemberMatches) {
    // #22: even for a registered type, if no member_metadata entry's offset matches the
    // pointer-to-member (here because the type was registered by hand with no members at all,
    // rather than via the FIELDS macro), the QueryPredicate constructor must fail fast instead
    // of silently leaving member_name_ empty and emitting malformed SQL like " = ?"
    const std::string type_id = typeid(MismatchedRecord).name();
    auto& instance = *GetReflectionRegisterInstance();
    instance.records[type_id].name = "MismatchedRecord";
    const ScopedRegistryCleanup cleanup(type_id);

    EXPECT_THROW(Equal(&MismatchedRecord::value, 42), std::runtime_error);
}


TEST(QueryPredicatesTest, NullableValuePredicatesUseContainedValue) {
    const Equal condition(&NullableRecord::optional_int, int64_t{42});
    EXPECT_EQ(0, strcmp(condition.Evaluate().data(), "optional_int = ?"));
    const auto bindings = condition.Bindings();
    ASSERT_EQ(1, bindings.size());
    EXPECT_EQ(SqliteStorageClass::kInt, bindings[0].storage_class);
    EXPECT_EQ(42, bindings[0].int_value);
}

TEST(QueryPredicatesTest, NullPredicatesHaveNoBindings) {
    const IsNull is_null(&NullableRecord::optional_text);
    EXPECT_EQ(0, strcmp(is_null.Evaluate().data(), "optional_text IS NULL"));
    EXPECT_TRUE(is_null.Bindings().empty());

    const IsNotNull is_not_null(&NullableRecord::optional_text);
    EXPECT_EQ(0, strcmp(is_not_null.Evaluate().data(), "optional_text IS NOT NULL"));
    EXPECT_TRUE(is_not_null.Bindings().empty());
}
