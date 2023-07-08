#include "query_predicates.h"
#include "internal/string_utilities.h"

using namespace sqlite_reflection;

std::string EmptyPredicate::Evaluate() const {
    return "";
}

AndPredicate QueryPredicateBase::And(const QueryPredicateBase& other) const {
    return AndPredicate(*this, other);
}

OrPredicate QueryPredicateBase::Or(const QueryPredicateBase& other) const {
    return OrPredicate(*this, other);
}

std::string QueryPredicate::Evaluate() const {
    return member_name_ + " " + symbol_ + " " + value_;
}

std::string QueryPredicate::GetStringForValue(void *v, SqliteStorageClass storage_class) {
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

BinaryPredicate::BinaryPredicate(const QueryPredicateBase& left, const QueryPredicateBase& right, const std::string& symbol)
: left_(&left), right_(&right), symbol_(symbol)
{
}

std::string BinaryPredicate::Evaluate() const {
    return "(" + left_->Evaluate() + " " + symbol_ + " " + right_->Evaluate() + ")";
}

AndPredicate::AndPredicate(const QueryPredicateBase& left, const QueryPredicateBase& right)
: BinaryPredicate(left, right, "AND")
{}

OrPredicate::OrPredicate(const QueryPredicateBase& left, const QueryPredicateBase& right)
: BinaryPredicate(left, right, "OR")
{}
