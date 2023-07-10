#include "query_predicates.h"
#include "internal/string_utilities.h"

using namespace sqlite_reflection;

const std::string single_quote("'");
const std::string space(" ");
const std::string percent("%");

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
	return member_name_ + space + symbol_ + space + value_;
}

std::string QueryPredicate::GetStringForValue(void* v, SqliteStorageClass storage_class) const {
	switch (storage_class) {
	case SqliteStorageClass::kInt:
		{
			auto value = *(int64_t*)(v);
			return StringUtilities::FromInt(value);
		}
	case SqliteStorageClass::kReal:
		{
			auto value = *(double*)(v);
			return StringUtilities::FromDouble(value);
		}
	case SqliteStorageClass::kText:
		{
			auto value = *(std::wstring*)(v);
			return single_quote + StringUtilities::ToUtf8(value) + single_quote;
		}
	case SqliteStorageClass::kDateTime:
		{
			auto value = *(std::time_t*)(v);
			return single_quote + StringUtilities::FromTime(value) + single_quote;
		}
	default:
		throw std::domain_error("Blob cannot be compared against equality");
		break;
	}
}

std::string Like::GetStringForValue(void* v, SqliteStorageClass storage_class) const {
	auto value = QueryPredicate::GetStringForValue(v, storage_class);
	value = Remove(value, single_quote);
	return single_quote + percent + value + percent + single_quote;
}

std::string Like::Remove(const std::string& source, const std::string& substring) {
	auto copy = source;
	auto it = copy.find(substring);
	while (it != std::string::npos) {
		copy.erase(it, substring.length());
		it = copy.find(substring);
	}
	return copy;
}

BinaryPredicate::BinaryPredicate(const QueryPredicateBase& left, const QueryPredicateBase& right, const std::string& symbol)
	: left_(&left), right_(&right), symbol_(symbol) {}

std::string BinaryPredicate::Evaluate() const {
	return "(" + left_->Evaluate() + space + symbol_ + space + right_->Evaluate() + ")";
}

AndPredicate::AndPredicate(const QueryPredicateBase& left, const QueryPredicateBase& right)
	: BinaryPredicate(left, right, "AND") {}

OrPredicate::OrPredicate(const QueryPredicateBase& left, const QueryPredicateBase& right)
	: BinaryPredicate(left, right, "OR") {}
