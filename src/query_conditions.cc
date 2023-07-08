#include "query_conditions.h"
#include "internal/string_utilities.h"

using namespace sqlite_reflection;

std::string EmptyCondition::Evaluate() const {
    return "";
}

AndCondition QueryConditionBase::And(const QueryConditionBase& other) const {
    return AndCondition(*this, other);
}

OrCondition QueryConditionBase::Or(const QueryConditionBase& other) const {
    return OrCondition(*this, other);
}

std::string QueryCondition::Evaluate() const {
    return member_name_ + " " + symbol_ + " " + value_;
}

std::string QueryCondition::GetStringForValue(void *v, SqliteStorageClass storage_class) {
    switch (storage_class) {
        case SqliteStorageClass::kInt:
        {
            auto value = *(int64_t*)(v);
            return StringUtilities::String(value);
        }
        case SqliteStorageClass::kReal:
        {
            auto value = *(double*)(v);
            return StringUtilities::String(value);
        }
        case SqliteStorageClass::kText:
        {
            auto value = *(std::wstring*)(v);
            return "'" + StringUtilities::ToUtf8(value) + "'";
        }
        default:
            throw std::domain_error("Blob cannot be compared against equality");
            break;
    }
}

BinaryCondition::BinaryCondition(const QueryConditionBase& left, const QueryConditionBase& right, const std::string& symbol)
: left_(&left), right_(&right), symbol_(symbol)
{
}

std::string BinaryCondition::Evaluate() const {
    return "(" + left_->Evaluate() + " " + symbol_ + " " + right_->Evaluate() + ")";
}

AndCondition::AndCondition(const QueryConditionBase& left, const QueryConditionBase& right)
: BinaryCondition(left, right, "AND")
{}

OrCondition::OrCondition(const QueryConditionBase& left, const QueryConditionBase& right)
: BinaryCondition(left, right, "OR")
{}
