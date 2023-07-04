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

namespace sqlite_reflection {

class REFLECTION_EXPORT Condition {
public:
    template<typename T, typename R>
    Condition(R T::* fn, R value, const std::string& symbol) {
        symbol_ = symbol;
        auto record = GetRecordFromTypeId(typeid(T).name());
        auto offset = OffsetFromStart(fn);
        for (auto i = 0; i < record.members.size(); ++i) {
            if (record.members[i].offset == offset) {
                member_name_ = record.members[i].name;
                value_ = GetStringForValue((void*)&value, record.members[i].trait);
                break;
            }
        }
    }
    
    std::string Evaluate() const {
        return member_name_ + " " + symbol_ + " " + value_;
    }
    
protected:
    std::string GetStringForValue(void* v, ReflectionMemberTrait trait);
    
    std::string symbol_;
    std::string member_name_;
    std::string value_;
};

class REFLECTION_EXPORT Equal final: public Condition {
public:
    template<typename T, typename R>
    Equal(R T::* fn, R value)
    : Condition(fn, value, "=") {}
};

class REFLECTION_EXPORT Unequal final: public Condition {
public:
    template<typename T, typename R>
    Unequal(R T::* fn, R value)
    : Condition(fn, value, "!=") {}
};
}

//    class QueryCondition
//    {
//    public:
//        virtual ~QueryCondition() = default;
//        virtual std::string Statement() const = 0;
//    };
//
//    class REFLECTION_EXPORT SingleCondition final : public QueryCondition
//    {
//    public:
//        explicit SingleCondition(const std::function<std::string()>& expression);
//        std::string Statement() const override;
//
//    protected:
//        const std::function<std::string()>& expression_;
//    };
//
//    class BinaryCondition : public QueryCondition
//    {
//    public:
//        BinaryCondition(const QueryCondition& left, const QueryCondition& right, const std::string& symbol);
//        std::string Statement() const override;
//
//    protected:
//        const QueryCondition& left_;
//        const QueryCondition& right_;
//        std::string symbol_;
//    };
//
//    class REFLECTION_EXPORT AndCondition final : public BinaryCondition
//    {
//    public:
//        AndCondition(const QueryCondition& left, const QueryCondition& right);
//    };
//
//    class REFLECTION_EXPORT OrCondition final : public BinaryCondition
//    {
//    public:
//        OrCondition(const QueryCondition& left, const QueryCondition& right);
//    };

    // ------------------------------------------------------------------------
