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

#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "reflection.h"

namespace sqlite_reflection {
class AndPredicate;
class OrPredicate;

struct REFLECTION_EXPORT SqlValue {
    SqlValue();

    SqliteStorageClass storage_class;
    int64_t int_value;
    bool bool_value;
    double real_value;
    std::string text_value;
    bool is_null;
};

/// The base class of all WHERE predicates used in SQLite queries
class REFLECTION_EXPORT QueryPredicateBase {
public:
    virtual ~QueryPredicateBase() = default;

    /// Returns a textual representation of the predicate with placeholders for bound values
    virtual std::string Evaluate() const = 0;

    /// Returns values that need to be bound to the predicate placeholders
    virtual std::vector<SqlValue> Bindings() const = 0;

    /// Creates a clone for compounding predicates
    virtual QueryPredicateBase* Clone() const = 0;

    /// Returns a compound predicate, which requires that both the current
    /// and the other predicate are valid at the same time
    AndPredicate And(const QueryPredicateBase& other) const;

    /// Returns a compound predicate, which requires that either the current
    /// or the other predicate are valid
    OrPredicate Or(const QueryPredicateBase& other) const;
};

/// A wrapper of a query predicate, which can be constructed from
/// a pointer-to-member function of a reflectable struct, thus enabling
/// type safety and compile-time guarantees, that the query is indeed valid
class REFLECTION_EXPORT QueryPredicate : public QueryPredicateBase {
public:
    std::string Evaluate() const override;
    std::vector<SqlValue> Bindings() const override;
    QueryPredicateBase* Clone() const override;

protected:
    template <typename T, typename R>
    QueryPredicate(R T::* fn, R value, const std::string& symbol,
                   std::function<SqlValue(void*, SqliteStorageClass)> value_retrieval)
        : symbol_(symbol) {
        auto record = GetRecordFromTypeId(typeid(T).name());
        auto offset = OffsetFromStart(fn);
        auto found = false;
        for (auto i = 0; i < record.member_metadata.size(); ++i) {
            if (record.member_metadata[i].offset == offset) {
                member_name_ = record.member_metadata[i].name;
                value_ = value_retrieval((void*)&value, record.member_metadata[i].storage_class);
                found = true;
                break;
            }
        }
        if (!found) {
            throw std::runtime_error("No registered member of '" + record.name +
                                      "' matches the given pointer-to-member (type id: " + typeid(T).name() + ")");
        }
    }

    template <typename T, typename R>
    QueryPredicate(R T::* fn, R value, const std::string& symbol)
        : QueryPredicate(fn, value, symbol,
                         [&](void* v, SqliteStorageClass storage_class) { return GetSqlValue(v, storage_class); }) {}

    template <typename T, typename R>
    QueryPredicate(fcpp::optional_t<R> T::* fn, R value, const std::string& symbol)
        : QueryPredicate(fn, fcpp::optional_t<R>(value), symbol, [&](void* v, SqliteStorageClass storage_class) {
              return GetOptionalSqlValue(v, storage_class);
          }) {}

    QueryPredicate(const std::string& symbol, const std::string& member_name, const SqlValue& value)
        : symbol_(symbol), member_name_(member_name), value_(value) {}

    /// Returns the value used for the current query, against which the struct member
    /// (defined from the pointer-to-member function) will be compared. The value needs
    /// to be type-erased, so that the header file is not bloated with unnecessary implementation details
    virtual SqlValue GetSqlValue(void* v, SqliteStorageClass storage_class) const;
    virtual SqlValue GetOptionalSqlValue(void* v, SqliteStorageClass storage_class) const;

    /// The symbol used for the comparison, for example "=" for equality
    std::string symbol_;

    /// The name of the compared member, as defined in source code, used to construct the
    /// textual representation of the evaluation string
    std::string member_name_;

    /// The comparison value that will be bound to the generated SQL placeholder
    SqlValue value_;
};

/// A wrapper for an empty predicate, used to fetch all elements of an SQLite table
class REFLECTION_EXPORT NullPredicate : public QueryPredicateBase {
public:
    template <typename T, typename R>
    NullPredicate(R T::* fn, const std::string& symbol) : symbol_(symbol) {
        auto record = GetRecordFromTypeId(typeid(T).name());
        auto offset = OffsetFromStart(fn);
        for (auto i = 0; i < record.member_metadata.size(); ++i) {
            if (record.member_metadata[i].offset == offset) {
                member_name_ = record.member_metadata[i].name;
                return;
            }
        }
        throw std::runtime_error("No registered member of '" + record.name +
                                 "' matches the given pointer-to-member (type id: " + typeid(T).name() + ")");
    }

    std::string Evaluate() const override;
    std::vector<SqlValue> Bindings() const override;
    QueryPredicateBase* Clone() const override;

protected:
    NullPredicate(const std::string& symbol, const std::string& member_name)
        : symbol_(symbol), member_name_(member_name) {}

    std::string symbol_;
    std::string member_name_;
};

class REFLECTION_EXPORT IsNull final : public NullPredicate {
public:
    template <typename T, typename R>
    explicit IsNull(R T::* fn) : NullPredicate(fn, "IS NULL") {}
};

class REFLECTION_EXPORT IsNotNull final : public NullPredicate {
public:
    template <typename T, typename R>
    explicit IsNotNull(R T::* fn) : NullPredicate(fn, "IS NOT NULL") {}
};

/// A wrapper for an empty predicate, used to fetch all elements of an SQLite table
class REFLECTION_EXPORT EmptyPredicate final : public QueryPredicateBase {
public:
    std::string Evaluate() const override;
    std::vector<SqlValue> Bindings() const override;
    QueryPredicateBase* Clone() const override;
};

/// A wrapper for an equality predicate, for which the value of the
/// struct member is required to be equal to a given control value
class REFLECTION_EXPORT Equal final : public QueryPredicate {
public:
    template <typename T, typename R>
    explicit Equal(R T::* fn, R value) : QueryPredicate(fn, value, "=") {}

    template <typename T, typename R>
    explicit Equal(fcpp::optional_t<R> T::* fn, R value) : QueryPredicate(fn, value, "=") {}

    template <typename T>
    explicit Equal(int64_t T::* fn, int value) : Equal(fn, (int64_t)value) {}

    template <typename T>
    explicit Equal(std::wstring T::* fn, const wchar_t* value) : Equal(fn, std::wstring(value)) {}
};

/// A wrapper for an inequality predicate, for which the value of the
/// struct member is required to be unequal to a given control value
class REFLECTION_EXPORT Unequal final : public QueryPredicate {
public:
    template <typename T, typename R>
    explicit Unequal(R T::* fn, R value) : QueryPredicate(fn, value, "!=") {}

    template <typename T, typename R>
    explicit Unequal(fcpp::optional_t<R> T::* fn, R value) : QueryPredicate(fn, value, "!=") {}

    template <typename T>
    explicit Unequal(int64_t T::* fn, int value) : Unequal(fn, (int64_t)value) {}

    template <typename T>
    explicit Unequal(std::wstring T::* fn, const wchar_t* value) : Unequal(fn, std::wstring(value)) {}
};

/// A wrapper for a similarity predicate, for which the value of the
/// struct member is required to be similar to a given control value
class REFLECTION_EXPORT Like final : public QueryPredicate {
public:
    template <typename T, typename R>
    explicit Like(R T::* fn, R value)
        : QueryPredicate(fn, value, "LIKE",
                         [&](void* v, SqliteStorageClass storage_class) { return GetSqlValue(v, storage_class); }) {}

    template <typename T, typename R>
    explicit Like(fcpp::optional_t<R> T::* fn, R value)
        : QueryPredicate(fn, fcpp::optional_t<R>(value), "LIKE", [&](void* v, SqliteStorageClass storage_class) {
              return GetOptionalSqlValue(v, storage_class);
          }) {}

    template <typename T>
    explicit Like(int64_t T::* fn, int value) : Like(fn, (int64_t)value) {}

    template <typename T>
    explicit Like(std::wstring T::* fn, const wchar_t* value) : Like(fn, std::wstring(value)) {}

protected:
    SqlValue GetSqlValue(void* v, SqliteStorageClass storage_class) const override;
};

/// A wrapper for a comparison predicate, for which the value of the
/// struct member is required to be greater than a given control value
class REFLECTION_EXPORT GreaterThan final : public QueryPredicate {
public:
    template <typename T>
    explicit GreaterThan(int64_t T::* fn, int64_t value) : QueryPredicate(fn, value, ">") {}

    template <typename T>
    explicit GreaterThan(int64_t T::* fn, int value) : QueryPredicate(fn, (int64_t)value, ">") {}

    template <typename T>
    explicit GreaterThan(double T::* fn, double value) : QueryPredicate(fn, value, ">") {}

    template <typename T>
    explicit GreaterThan(fcpp::optional_t<int64_t> T::* fn, int64_t value) : QueryPredicate(fn, value, ">") {}

    template <typename T>
    explicit GreaterThan(fcpp::optional_t<double> T::* fn, double value) : QueryPredicate(fn, value, ">") {}
};

/// A wrapper for a comparison predicate, for which the value of the
/// struct member is required to be greater than or equal to a given control value
class REFLECTION_EXPORT GreaterThanOrEqual final : public QueryPredicate {
public:
    template <typename T>
    explicit GreaterThanOrEqual(int64_t T::* fn, int64_t value) : QueryPredicate(fn, value, ">=") {}

    template <typename T>
    explicit GreaterThanOrEqual(int64_t T::* fn, int value) : QueryPredicate(fn, (int64_t)value, ">=") {}

    template <typename T>
    explicit GreaterThanOrEqual(double T::* fn, double value) : QueryPredicate(fn, value, ">=") {}

    template <typename T>
    explicit GreaterThanOrEqual(fcpp::optional_t<int64_t> T::* fn, int64_t value) : QueryPredicate(fn, value, ">=") {}

    template <typename T>
    explicit GreaterThanOrEqual(fcpp::optional_t<double> T::* fn, double value) : QueryPredicate(fn, value, ">=") {}
};

/// A wrapper for a comparison predicate, for which the value of the
/// struct member is required to be smaller than a given control value
class REFLECTION_EXPORT SmallerThan final : public QueryPredicate {
public:
    template <typename T>
    explicit SmallerThan(int64_t T::* fn, int64_t value) : QueryPredicate(fn, value, "<") {}

    template <typename T>
    explicit SmallerThan(int64_t T::* fn, int value) : QueryPredicate(fn, (int64_t)value, "<") {}

    template <typename T>
    explicit SmallerThan(double T::* fn, double value) : QueryPredicate(fn, value, "<") {}

    template <typename T>
    explicit SmallerThan(fcpp::optional_t<int64_t> T::* fn, int64_t value) : QueryPredicate(fn, value, "<") {}

    template <typename T>
    explicit SmallerThan(fcpp::optional_t<double> T::* fn, double value) : QueryPredicate(fn, value, "<") {}
};

/// A wrapper for a comparison predicate, for which the value of the
/// struct member is required to be smaller than or equal to a given control value
class REFLECTION_EXPORT SmallerThanOrEqual final : public QueryPredicate {
public:
    template <typename T>
    explicit SmallerThanOrEqual(int64_t T::* fn, int64_t value) : QueryPredicate(fn, value, "<=") {}

    template <typename T>
    explicit SmallerThanOrEqual(int64_t T::* fn, int value) : QueryPredicate(fn, (int64_t)value, "<=") {}

    template <typename T>
    explicit SmallerThanOrEqual(double T::* fn, double value) : QueryPredicate(fn, value, "<=") {}

    template <typename T>
    explicit SmallerThanOrEqual(fcpp::optional_t<int64_t> T::* fn, int64_t value) : QueryPredicate(fn, value, "<=") {}

    template <typename T>
    explicit SmallerThanOrEqual(fcpp::optional_t<double> T::* fn, double value) : QueryPredicate(fn, value, "<=") {}
};

/// A wrapper of a compound predicate, which combines two other predicates,
/// allowing the construction of more complex predicates from elementary predicates
class REFLECTION_EXPORT BinaryPredicate : public QueryPredicateBase {
public:
    std::string Evaluate() const override;
    std::vector<SqlValue> Bindings() const override;

protected:
    BinaryPredicate(const QueryPredicateBase& left, const QueryPredicateBase& right, const std::string& symbol);

    std::unique_ptr<QueryPredicateBase> left_;
    std::unique_ptr<QueryPredicateBase> right_;
    std::string symbol_;
};

/// A compound predicate, which requires that both predicates are valid at the same time
class REFLECTION_EXPORT AndPredicate final : public BinaryPredicate {
public:
    AndPredicate(const QueryPredicateBase& left, const QueryPredicateBase& right);
    QueryPredicateBase* Clone() const override;
};

/// A compound predicate, which requires that either predicate is valid
class REFLECTION_EXPORT OrPredicate final : public BinaryPredicate {
public:
    OrPredicate(const QueryPredicateBase& left, const QueryPredicateBase& right);
    QueryPredicateBase* Clone() const override;
};
}  // namespace sqlite_reflection
