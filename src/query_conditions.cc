#include "query_conditions.h"
#include "internal/string_utilities.h"

using namespace sqlite_reflection;

std::string EmptyCondition::Evaluate() const {
    return "";
}

AndCondition ConditionBase::And(const ConditionBase& other) const {
    return AndCondition(*this, other);
}

OrCondition ConditionBase::Or(const ConditionBase& other) const {
    return OrCondition(*this, other);
}

std::string Condition::Evaluate() const {
    return member_name_ + " " + symbol_ + " " + value_;
}

std::string Condition::GetStringForValue(void *v, ReflectionMemberTrait trait) {
    switch (trait) {
        case ReflectionMemberTrait::kInt:
        {
            auto value = *(int64_t*)(v);
            return StringUtilities::String(value);
        }
        case ReflectionMemberTrait::kReal:
        {
            auto value = *(double*)(v);
            return StringUtilities::String(value);
        }
        case ReflectionMemberTrait::kText:
        {
            auto value = *(std::wstring*)(v);
            return "'" + StringUtilities::ToUtf8(value) + "'";
        }
        default:
            throw std::domain_error("Blob cannot be compared against equality");
            break;
    }
}

BinaryCondition::BinaryCondition(const ConditionBase& left, const ConditionBase& right, const std::string& symbol)
: left_(&left), right_(&right), symbol_(symbol)
{
}

std::string BinaryCondition::Evaluate() const {
    return "(" + left_->Evaluate() + " " + symbol_ + " " + right_->Evaluate() + ")";
}

AndCondition::AndCondition(const ConditionBase& left, const ConditionBase& right)
: BinaryCondition(left, right, "AND")
{}

OrCondition::OrCondition(const ConditionBase& left, const ConditionBase& right)
: BinaryCondition(left, right, "OR")
{}
