// MIT License
//
// Copyright (c) 2023 Ioannis Kaliakatsos
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following predicates:
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

#include "reflection.h"

#include <string>
#include <memory>

namespace sqlite_reflection {

class AndPredicate;
class OrPredicate;

/// The base class of all WHERE predicates used in SQLite queries
class REFLECTION_EXPORT QueryPredicateBase {
public:
    /// Returns a textual representation of the predicate, ready to be consumed by the SELECT query
    virtual std::string Evaluate() const = 0;
    
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
class REFLECTION_EXPORT QueryPredicate: public QueryPredicateBase {
public:
    std::string Evaluate() const override;
    
protected:
    template<typename T, typename R>
    QueryPredicate(R T::* fn, R value, const std::string& symbol, std::function<std::string(void*, SqliteStorageClass)> value_retrieval) {
        symbol_ = symbol;
        auto record = GetRecordFromTypeId(typeid(T).name());
        auto offset = OffsetFromStart(fn);
        for (auto i = 0; i < record.member_metadata.size(); ++i) {
            if (record.member_metadata[i].offset == offset) {
                member_name_ = record.member_metadata[i].name;
                value_ = value_retrieval((void*)&value, record.member_metadata[i].storage_class);
                break;
            }
        }
    }
    
    template<typename T, typename R>
    QueryPredicate(R T::* fn, R value, const std::string& symbol)
    : QueryPredicate(fn, value, symbol, [&](void* v, SqliteStorageClass storage_class) {
        return GetStringForValue(v, storage_class);
    })
    {}
    
    /// Returns a textual representation of the value used for the current query, against which the
    /// struct member (defined from the pointer-to-member function) will be compared. The value needs
    /// to be type-erased, so that the header file is not bloated with unnecessary implementation details
    virtual std::string GetStringForValue(void* v, SqliteStorageClass storage_class) const;
    
    /// The symbol used for the comparison, for example "=" for equality
    std::string symbol_;
    
    /// The name of the compared member, as defined in source code, used to construct the
    /// textual representation of the evaluation string
    std::string member_name_;
    
    /// The textual representation of the comparison value, used to construct the
    /// textual representation of the evaluation string
    std::string value_;
};

/// A wrapper for an empty predicate, used to fetch all elements of an SQLite table
class REFLECTION_EXPORT EmptyPredicate final: public QueryPredicateBase {
public:
    std::string Evaluate() const override;
};

/// A wrapper for an equality predicate, for which the value of the
/// struct member is required to be equal to a given control value
class REFLECTION_EXPORT Equal final: public QueryPredicate {
public:
    template<typename T, typename R>
    Equal(R T::* fn, R value)
    : QueryPredicate(fn, value, "=") {}
};

/// A wrapper for an inequality predicate, for which the value of the
/// struct member is required to be unequal to a given control value
class REFLECTION_EXPORT Unequal final: public QueryPredicate {
public:
    template<typename T, typename R>
    Unequal(R T::* fn, R value)
    : QueryPredicate(fn, value, "!=") {}
};

/// A wrapper for a similarity predicate, for which the value of the
/// struct member is required to be similar to a given control value
class REFLECTION_EXPORT Like final: public QueryPredicate {
public:
    template<typename T, typename R>
    Like(R T::* fn, R value)
    : QueryPredicate(fn, value, "LIKE", [&](void* v, SqliteStorageClass storage_class) {
        return GetStringForValue(v, storage_class);
    })
    {}
    
protected:
    std::string GetStringForValue(void* v, SqliteStorageClass storage_class) const override;
    static std::string Remove(const std::string& source, const std::string& substring);
};

/// A wrapper for a comparison predicate, for which the value of the
/// struct member is required to be greater than a given control value
class REFLECTION_EXPORT GreaterThan final: public QueryPredicate {
public:
    template<typename T>
    GreaterThan(int64_t T::* fn, int64_t value)
    : QueryPredicate(fn, value, ">") {}
    
    template<typename T>
    GreaterThan(double T::* fn, double value)
    : QueryPredicate(fn, value, ">") {}
};

/// A wrapper for a comparison predicate, for which the value of the
/// struct member is required to be greater than or equal to a given control value
class REFLECTION_EXPORT GreaterThanOrEqual final: public QueryPredicate {
public:
    template<typename T>
    GreaterThanOrEqual(int64_t T::* fn, int64_t value)
    : QueryPredicate(fn, value, ">=") {}
    
    template<typename T>
    GreaterThanOrEqual(double T::* fn, double value)
    : QueryPredicate(fn, value, ">=") {}
};

/// A wrapper for a comparison predicate, for which the value of the
/// struct member is required to be smaller than a given control value
class REFLECTION_EXPORT SmallerThan final: public QueryPredicate {
public:
    template<typename T>
    SmallerThan(int64_t T::* fn, int64_t value)
    : QueryPredicate(fn, value, "<") {}
    
    template<typename T>
    SmallerThan(double T::* fn, double value)
    : QueryPredicate(fn, value, "<") {}
};

/// A wrapper for a comparison predicate, for which the value of the
/// struct member is required to be smaller than or equal to a given control value
class REFLECTION_EXPORT SmallerThanOrEqual final: public QueryPredicate {
public:
    template<typename T>
    SmallerThanOrEqual(int64_t T::* fn, int64_t value)
    : QueryPredicate(fn, value, "<=") {}
    
    template<typename T>
    SmallerThanOrEqual(double T::* fn, double value)
    : QueryPredicate(fn, value, "<=") {}
};

/// A wrapper of a compound predicate, which combines two other predicates,
/// allowing the construction of more complex predicates from elementary predicates
class REFLECTION_EXPORT BinaryPredicate: public QueryPredicateBase {
public:
    std::string Evaluate() const override;
    
protected:
    BinaryPredicate(const QueryPredicateBase& left, const QueryPredicateBase& right, const std::string& symbol);
    
    const QueryPredicateBase* left_;
    const QueryPredicateBase* right_;
    std::string symbol_;
};

/// A compound predicate, which requires that both predicates are valid at the same time
class REFLECTION_EXPORT AndPredicate final: public BinaryPredicate {
public:
    AndPredicate(const QueryPredicateBase& left, const QueryPredicateBase& right);
};

/// A compound predicate, which requires that either predicate is valid
class REFLECTION_EXPORT OrPredicate final: public BinaryPredicate {
public:
    OrPredicate(const QueryPredicateBase& left, const QueryPredicateBase& right);
};
}
