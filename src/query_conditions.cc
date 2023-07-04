#include "query_conditions.h"
#include "internal/string_utilities.h"

using namespace sqlite_reflection;

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

//BinaryCondition::BinaryCondition(const QueryCondition& left, const QueryCondition& right, const std::string& symbol)
//    : left_(left), right_(right), symbol_(symbol) {}
//
//SingleCondition::SingleCondition(const std::function<std::string()>& expression)
//    : expression_(expression) {}
//
//std::string SingleCondition::Statement() const {
//    return expression_();
//}
//
//std::string BinaryCondition::Statement() const {
//    return std::string("(") + left_.Statement() + " " + symbol_ + " " + right_.Statement() + std::string(")");
//}
//
//AndCondition::AndCondition(const QueryCondition& left, const QueryCondition& right)
//    : BinaryCondition(left, right, "AND") {}
//
//OrCondition::OrCondition(const QueryCondition& left, const QueryCondition& right)
//    : BinaryCondition(left, right, "OR") {}

// ------------------------------------------------------------------------
