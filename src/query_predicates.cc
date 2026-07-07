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

#include "internal/string_utilities.h"

using namespace sqlite_reflection;

constexpr char space[] = " ";
constexpr char percent[] = "%";

namespace {
/// Escapes backslash, % and _ in a LIKE value so it is matched literally, rather than being
/// interpreted as SQLite LIKE wildcards. Must be paired with an ESCAPE '\' clause in the
/// generated SQL (see QueryPredicate::Evaluate). Backslash is escaped as it is encountered,
/// so a backslash inserted to escape % or _ is never itself re-escaped.
std::string EscapeLikeWildcards(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char c : value) {
        if (c == '\\' || c == '%' || c == '_') {
            escaped += '\\';
        }
        escaped += c;
    }
    return escaped;
}
}  // namespace

SqlValue::SqlValue() : storage_class(SqliteStorageClass::kText), int_value(0), bool_value(false), real_value(0.0) {}

QueryPredicateBase* QueryPredicate::Clone() const {
    return new QueryPredicate(symbol_, member_name_, value_);
}

std::string EmptyPredicate::Evaluate() const {
    return "";
}

std::vector<SqlValue> EmptyPredicate::Bindings() const {
    return {};
}

QueryPredicateBase* EmptyPredicate::Clone() const {
    return new EmptyPredicate();
}

AndPredicate QueryPredicateBase::And(const QueryPredicateBase& other) const {
    return AndPredicate(*this, other);
}

OrPredicate QueryPredicateBase::Or(const QueryPredicateBase& other) const {
    return OrPredicate(*this, other);
}

std::string QueryPredicate::Evaluate() const {
    auto evaluation = member_name_ + space + symbol_ + space + "?";
    if (symbol_ == "LIKE") {
        // The escape character used by EscapeLikeWildcards on the bound value must be
        // declared here to take effect; symbol_ and the already-escaped value_ both survive
        // Clone() (unlike a Like-only Evaluate() override), so this stays correct for a Like
        // combined via And()/Or() too
        evaluation += " ESCAPE '\\'";
    }
    return evaluation;
}

std::vector<SqlValue> QueryPredicate::Bindings() const {
    return {value_};
}

SqlValue QueryPredicate::GetSqlValue(void* v, SqliteStorageClass storage_class) const {
    SqlValue result;
    result.storage_class = storage_class;
    switch (storage_class) {
        case SqliteStorageClass::kInt: {
            result.int_value = *(int64_t*)(v);
            return result;
        }
        case SqliteStorageClass::kBool: {
            result.bool_value = *(bool*)(v);
            return result;
        }
        case SqliteStorageClass::kReal: {
            result.real_value = *(double*)(v);
            return result;
        }
        case SqliteStorageClass::kText: {
            auto value = *(std::wstring*)(v);
            result.text_value = StringUtilities::ToUtf8(value);
            return result;
        }
        case SqliteStorageClass::kDateTime: {
            auto value = *(TimePoint*)(v);
            result.text_value = StringUtilities::ToUtf8(value.SystemTime());
            return result;
        }
        default:
            throw std::domain_error("Blob cannot be compared against equality");
    }
}

SqlValue Like::GetSqlValue(void* v, SqliteStorageClass storage_class) const {
    auto value = QueryPredicate::GetSqlValue(v, storage_class);
    switch (storage_class) {
        case SqliteStorageClass::kInt:
            value.storage_class = SqliteStorageClass::kText;
            value.text_value = percent + EscapeLikeWildcards(StringUtilities::FromInt(value.int_value)) + percent;
            return value;
        case SqliteStorageClass::kBool:
            value.storage_class = SqliteStorageClass::kText;
            value.text_value =
                percent + EscapeLikeWildcards(StringUtilities::FromInt(value.bool_value ? 1 : 0)) + percent;
            return value;
        case SqliteStorageClass::kReal:
            value.storage_class = SqliteStorageClass::kText;
            value.text_value = percent + EscapeLikeWildcards(StringUtilities::FromDouble(value.real_value)) + percent;
            return value;
        case SqliteStorageClass::kText:
        case SqliteStorageClass::kDateTime:
            value.text_value = percent + EscapeLikeWildcards(value.text_value) + percent;
            return value;
        default:
            throw std::domain_error("Blob cannot be compared against similarity");
    }
}

BinaryPredicate::BinaryPredicate(const QueryPredicateBase& left, const QueryPredicateBase& right,
                                 const std::string& symbol)
    : left_(left.Clone()), right_(right.Clone()), symbol_(symbol) {}

std::string BinaryPredicate::Evaluate() const {
    return "(" + left_->Evaluate() + space + symbol_ + space + right_->Evaluate() + ")";
}

std::vector<SqlValue> BinaryPredicate::Bindings() const {
    auto bindings = left_->Bindings();
    const auto right_bindings = right_->Bindings();
    bindings.insert(bindings.end(), right_bindings.begin(), right_bindings.end());
    return bindings;
}

AndPredicate::AndPredicate(const QueryPredicateBase& left, const QueryPredicateBase& right)
    : BinaryPredicate(left, right, "AND") {}

QueryPredicateBase* AndPredicate::Clone() const {
    return new AndPredicate(*left_.get(), *right_.get());
}

OrPredicate::OrPredicate(const QueryPredicateBase& left, const QueryPredicateBase& right)
    : BinaryPredicate(left, right, "OR") {}

QueryPredicateBase* OrPredicate::Clone() const {
    return new OrPredicate(*left_.get(), *right_.get());
}
