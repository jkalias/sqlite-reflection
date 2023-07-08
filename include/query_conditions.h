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

#pragma once

#include "reflection.h"

#include <string>
#include <memory>

namespace sqlite_reflection {

class AndCondition;
class OrCondition;

/// The base class of all WHERE conditions used in SQLite queries
class REFLECTION_EXPORT QueryConditionBase {
public:
    /// Returns a textual representation of the condition, ready to be consumed by the SELECT query
    virtual std::string Evaluate() const = 0;
    
    /// Returns a compound condition, which requires that both the current
    /// and the other condition are valid at the same time
    AndCondition And(const QueryConditionBase& other) const;
    
    /// Returns a compound condition, which requires that either  the current
    /// or the other condition are valid
    OrCondition Or(const QueryConditionBase& other) const;
};

/// A wrapper of a query condition, which can be constructed from
/// a pointer-to-member function of a reflectable struct, thus enabling
/// type safety and compile-time guarantees, that the query is indeed valid
class REFLECTION_EXPORT QueryCondition: public QueryConditionBase {
public:
    std::string Evaluate() const override;
    
protected:
    template<typename T, typename R>
    QueryCondition(R T::* fn, R value, const std::string& symbol) {
        symbol_ = symbol;
        auto record = GetRecordFromTypeId(typeid(T).name());
        auto offset = OffsetFromStart(fn);
        for (auto i = 0; i < record.member_metadata.size(); ++i) {
            if (record.member_metadata[i].offset == offset) {
                member_name_ = record.member_metadata[i].name;
                value_ = GetStringForValue((void*)&value, record.member_metadata[i].storage_class);
                break;
            }
        }
    }
    
    /// Returns a textual representation of the value used for the current query, against which the
    /// struct member (defined from the pointer-to-member function) will be compared. The value needs
    /// to be type-erased, so that the header file is not bloated with unnecessary implementation details
    std::string GetStringForValue(void* v, SqliteStorageClass storage_class);
    
    /// The symbol used for the comparison, for example "=" for equality
    std::string symbol_;
    
    /// The name of the compared member, as defined in source code, used to construct the
    /// textual representation of the evaluation string
    std::string member_name_;
    
    /// The textual representation of the comparison value, used to construct the
    /// textual representation of the evaluation string
    std::string value_;
};

/// A wrapper for an empty condition, used to fetch all elements of an SQLite table
class REFLECTION_EXPORT EmptyCondition final: public QueryConditionBase {
public:
    std::string Evaluate() const override;
};

/// A wrapper for an equality condition, for which the value of the
/// struct member is required to be equal to a given control value
class REFLECTION_EXPORT Equal final: public QueryCondition {
public:
    template<typename T, typename R>
    Equal(R T::* fn, R value)
    : QueryCondition(fn, value, "=") {}
};

/// A wrapper for an inequality condition, for which the value of the
/// struct member is required to be unequal to a given control value
class REFLECTION_EXPORT Unequal final: public QueryCondition {
public:
    template<typename T, typename R>
    Unequal(R T::* fn, R value)
    : QueryCondition(fn, value, "!=") {}
};

/// A wrapper for a comparison condition, for which the value of the
/// struct member is required to be greater than a given control value
class REFLECTION_EXPORT GreaterThan final: public QueryCondition {
public:
    template<typename T>
    GreaterThan(int64_t T::* fn, int64_t value)
    : QueryCondition(fn, value, ">") {}
    
    template<typename T>
    GreaterThan(double T::* fn, double value)
    : QueryCondition(fn, value, ">") {}
};

/// A wrapper for a comparison condition, for which the value of the
/// struct member is required to be greater than or equal to a given control value
class REFLECTION_EXPORT GreaterThanOrEqual final: public QueryCondition {
public:
    template<typename T>
    GreaterThanOrEqual(int64_t T::* fn, int64_t value)
    : QueryCondition(fn, value, ">=") {}
    
    template<typename T>
    GreaterThanOrEqual(double T::* fn, double value)
    : QueryCondition(fn, value, ">=") {}
};

/// A wrapper for a comparison condition, for which the value of the
/// struct member is required to be smaller than a given control value
class REFLECTION_EXPORT SmallerThan final: public QueryCondition {
public:
    template<typename T>
    SmallerThan(int64_t T::* fn, int64_t value)
    : QueryCondition(fn, value, "<") {}
    
    template<typename T>
    SmallerThan(double T::* fn, double value)
    : QueryCondition(fn, value, "<") {}
};

/// A wrapper for a comparison condition, for which the value of the
/// struct member is required to be smaller than or equal to a given control value
class REFLECTION_EXPORT SmallerThanOrEqual final: public QueryCondition {
public:
    template<typename T>
    SmallerThanOrEqual(int64_t T::* fn, int64_t value)
    : QueryCondition(fn, value, "<=") {}
    
    template<typename T>
    SmallerThanOrEqual(double T::* fn, double value)
    : QueryCondition(fn, value, "<=") {}
};

/// A wrapper of a compound condition, which combines two other conditions,
/// allowing the construction of more complex conditions from elementary conditions
class REFLECTION_EXPORT BinaryCondition: public QueryConditionBase {
public:
    std::string Evaluate() const override;
    
protected:
    BinaryCondition(const QueryConditionBase& left, const QueryConditionBase& right, const std::string& symbol);
    
    const QueryConditionBase* left_;
    const QueryConditionBase* right_;
    std::string symbol_;
};

/// A compound condition, which requires that both conditions are valid at the same time
class REFLECTION_EXPORT AndCondition final: public BinaryCondition {
public:
    AndCondition(const QueryConditionBase& left, const QueryConditionBase& right);
};

/// A compound condition, which requires that either condition is valid
class REFLECTION_EXPORT OrCondition final: public BinaryCondition {
public:
    OrCondition(const QueryConditionBase& left, const QueryConditionBase& right);
};
}
