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

#include "query_predicates.h"
#include "internal/string_utilities.h"

using namespace sqlite_reflection;

const std::string single_quote("'");
const std::string space(" ");
const std::string percent("%");

QueryPredicateBase* QueryPredicate::Clone() const {
	return new QueryPredicate(symbol_, member_name_, value_);
}

std::string EmptyPredicate::Evaluate() const {
	return "";
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
			auto value = *(TimePoint*)(v);
			return single_quote + StringUtilities::ToUtf8(value.SystemTime()) + single_quote;
		}
	default:
		throw std::domain_error("Blob cannot be compared against equality");
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
	: left_(left.Clone()), right_(right.Clone()), symbol_(symbol) {}

std::string BinaryPredicate::Evaluate() const {
	return "(" + left_->Evaluate() + space + symbol_ + space + right_->Evaluate() + ")";
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