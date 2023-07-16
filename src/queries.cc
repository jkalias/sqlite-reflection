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

#include "queries.h"

#include <string>
#include <cstring>
#include <string.h>
#include <algorithm>
#include <iterator>
#include <stdexcept>

#include "internal/sqlite3.h"
#include "internal/string_utilities.h"
#include "fetch_query_results.h"

using namespace sqlite_reflection;

Query::Query(sqlite3* db, const Reflection& record)
	: db_(db), record_(record) {}

std::string Query::JoinedRecordColumnNames() const {
	const auto column_names = GetRecordColumnNames();
	return StringUtilities::Join(column_names, ", ");
}

std::vector<std::string> Query::GetRecordColumnNames() const {
	std::vector<std::string> column_names;
	column_names.reserve(record_.member_metadata.size());
	for (auto j = 0; j < record_.member_metadata.size(); ++j) {
		column_names.emplace_back(CustomizedColumnName(j));
	}
	return column_names;
}

std::string Query::CustomizedColumnName(size_t index) const {
	return record_.member_metadata[index].name;
}

ExecutionQuery::ExecutionQuery(sqlite3* db, const Reflection& record)
	: Query(db, record) {}

void ExecutionQuery::Execute() const {
	const auto sql = PrepareSql();
	if (sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr)) {
		throw std::domain_error("Fatal error in transaction start");
	}
	if (sqlite3_exec(db_, sql.data(), nullptr, nullptr, nullptr)) {
		sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
		throw std::domain_error((sql + ": Query could not be executed").data());
	}
	if (sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr)) {
		throw std::domain_error("Fatal error in transaction commit");
	}
}

std::vector<std::string> ExecutionQuery::GetValues(void* p) const {
	const auto& members = record_.member_metadata;
	std::vector<std::string> values;

	for (auto j = 0; j < members.size(); j++) {
		const auto current_column = members[j].name;
		const auto current_storage_class = members[j].storage_class;
		std::string content;

		switch (current_storage_class) {
		case SqliteStorageClass::kInt:
			{
				const auto& value = (*(int64_t*)((void*)GetMemberAddress(p, record_, j)));
				content = StringUtilities::FromInt(value);
				break;
			}
                
        case SqliteStorageClass::kBool:
            {
                const auto& value = (*(bool*)((void*)GetMemberAddress(p, record_, j)));
                content = StringUtilities::FromInt(value ? 1 : 0);
                break;
            }

		case SqliteStorageClass::kReal:
			{
				const auto& value = (*(double*)((void*)GetMemberAddress(p, record_, j)));
				content = StringUtilities::FromDouble(value);
				break;
			}

		case SqliteStorageClass::kText:
			{
				auto& value = (*(std::wstring*)((void*)GetMemberAddress(p, record_, j)));
				content = StringUtilities::ToUtf8(value);
				break;
			}

		case SqliteStorageClass::kDateTime:
			{
				auto& value = (*(TimePoint*)((void*)GetMemberAddress(p, record_, j)));
				content = StringUtilities::ToUtf8(value.SystemTime());
				break;
			}

		default:
			break;
		}
		values.emplace_back("'" + content + "'");
	}

	return values;
}

CreateTableQuery::CreateTableQuery(sqlite3* db, const Reflection& record)
	: ExecutionQuery(db, record) { }

std::string CreateTableQuery::PrepareSql() const {
	std::string sql("CREATE TABLE IF NOT EXISTS ");
	sql += record_.name + " (" + JoinedRecordColumnNames() + ");";
	return sql;
}

std::string CreateTableQuery::CustomizedColumnName(size_t index) const {
	auto name = Query::CustomizedColumnName(index);
	const auto is_id = name.compare(std::string("id")) == 0;
	name += " " + record_.member_metadata[index].sqlite_column_name;

	return is_id
		       ? name + " PRIMARY KEY"
		       : name;
}

DeleteQuery::DeleteQuery(sqlite3* db, const Reflection& record, const QueryPredicateBase* predicate)
	: ExecutionQuery(db, record), predicate_(predicate) {}

std::string DeleteQuery::PrepareSql() const {
	std::string sql("DELETE FROM ");
	sql += record_.name + " WHERE " + predicate_->Evaluate() + ";";
	return sql;
}

InsertQuery::InsertQuery(sqlite3* db, const Reflection& record, void* p)
	: ExecutionQuery(db, record), p_(p) {}

std::string InsertQuery::PrepareSql() const {
	std::string sql("INSERT INTO ");
	sql += record_.name + " (" + JoinedRecordColumnNames() + ") VALUES (";
	sql += JoinedValues() + ");";
	return sql;
}

std::string InsertQuery::JoinedValues() const {
	const auto values = GetValues(p_);
	return StringUtilities::Join(values, ", ");
}

UpdateQuery::UpdateQuery(sqlite3* db, const Reflection& record, void* p)
	: ExecutionQuery(db, record), p_(p) {}

std::string UpdateQuery::PrepareSql() const {
	std::string sql("UPDATE ");
	sql += record_.name + " SET ";

	auto columns = GetRecordColumnNames();
	const auto values = GetValues(p_);

	std::vector<std::string> columns_with_values;
	columns_with_values.reserve(values.size());
	std::transform(columns.begin() + 1,
	               columns.end(),
	               values.begin() + 1,
	               std::back_inserter(columns_with_values),
	               [](const std::string& column, const std::string& value){
		               return column + "=" + value;
	               });

	sql += StringUtilities::Join(columns_with_values, ", ");
	sql += " WHERE " + columns[0] + "=" + values[0];
	sql += ";";

	return sql;
}

FetchMaxIdQuery::FetchMaxIdQuery(sqlite3* db, const Reflection& record)
	: Query(db, record), stmt_(nullptr) {}

FetchMaxIdQuery::~FetchMaxIdQuery() {
	if (stmt_) {
		sqlite3_finalize(stmt_);
	}
}

std::string FetchMaxIdQuery::PrepareSql() const {
	return "SELECT MAX(id) FROM " + record_.name + ";";
}

int64_t FetchMaxIdQuery::GetMaxId() {
	const auto sql = PrepareSql();

	if (sqlite3_prepare_v2(db_, sql.data(), -1, &stmt_, nullptr)) {
		sqlite3_close(db_);
		throw std::runtime_error("Could not retrieve max id for table " + record_.name);
	}

	const auto column_count = sqlite3_column_count(stmt_);
	if (column_count != 1) {
		throw std::runtime_error("Number of columns for max id is wrong for table " + record_.name);
	}

	if (sqlite3_step(stmt_) != SQLITE_ROW) {
		throw std::runtime_error("Row result could not be read for max id of table " + record_.name);
	}

	const auto max_id = sqlite3_column_int(stmt_, 0);
	return max_id;
}

FetchRecordsQuery::FetchRecordsQuery(sqlite3* db, const Reflection& record, const QueryPredicateBase* predicate)
	: Query(db, record), stmt_(nullptr), predicate_(predicate) {}

FetchRecordsQuery::~FetchRecordsQuery() {
	if (stmt_) {
		sqlite3_finalize(stmt_);
	}
}

FetchQueryResults FetchRecordsQuery::GetResults() {
	const auto sql = PrepareSql();

	if (sqlite3_prepare_v2(db_, sql.data(), -1, &stmt_, nullptr)) {
		sqlite3_close(db_);
		throw std::runtime_error((sql + ": could not get results").data());
	}

	const auto column_count = sqlite3_column_count(stmt_);

	FetchQueryResults results;
	results.column_names.reserve(column_count);
	for (auto i = 0; i < column_count; i++) {
		results.column_names.emplace_back(sqlite3_column_name(stmt_, i));
	}

	while (sqlite3_step(stmt_) != SQLITE_DONE) {
		std::vector<std::wstring> row;
		row.reserve(column_count);
		for (auto col = 0; col < column_count; col++) {
			auto value = GetColumnValue(col);
			row.emplace_back(value);
		}
		results.row_values.emplace_back(row);
	}

	return results;
}

void FetchRecordsQuery::Hydrate(void* p, const FetchQueryResults& query_results, const Reflection& record, size_t i) {
	for (auto j = 0; j < query_results.column_names.size(); j++) {
		const auto current_column = query_results.column_names[j];
		const auto current_storage_class = record.member_metadata[j].storage_class;
		const auto& content = query_results.row_values[i][j];
		if (content == L"") {
			continue;
		}

		switch (current_storage_class) {
		case SqliteStorageClass::kInt:
			{
				auto& v = (*(int64_t*)((void*)GetMemberAddress(p, record, j)));
				v = StringUtilities::ToInt(content);
				break;
			}
                
        case SqliteStorageClass::kBool:
            {
                auto& v = (*(bool*)((void*)GetMemberAddress(p, record, j)));
                v = StringUtilities::ToInt(content) == 1;
                break;
            }

		case SqliteStorageClass::kReal:
			{
				auto& v = (*(double*)((void*)GetMemberAddress(p, record, j)));
				v = StringUtilities::ToDouble(content);
				break;
			}

		case SqliteStorageClass::kText:
			{
				auto& v = (*(std::wstring*)((void*)GetMemberAddress(p, record, j)));
				v = content;
				break;
			}

		case SqliteStorageClass::kDateTime:
			{
				auto& v = (*(TimePoint*)((void*)GetMemberAddress(p, record, j)));
				v = TimePoint::FromSystemTime(content);
				break;
			}

		default:
			break;
		}
	}
}

std::string FetchRecordsQuery::PrepareSql() const {
	std::string sql("SELECT * FROM ");
	sql += record_.name;
	const auto condition_evaluation = predicate_->Evaluate();
	if (strcmp(condition_evaluation.data(), "") != 0) {
		sql += " WHERE " + condition_evaluation;
	}
	return sql + ";";
}

std::wstring FetchRecordsQuery::GetColumnValue(const int col) const {
	const int col_type = sqlite3_column_type(stmt_, col);
	switch (col_type) {
	case SQLITE_INTEGER:
		return std::to_wstring(sqlite3_column_int(stmt_, col));

	case SQLITE_FLOAT:
		return std::to_wstring(sqlite3_column_double(stmt_, col));

	case SQLITE_TEXT:
		{
			const auto content = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, col));
			return StringUtilities::FromUtf8(content);
		}

	default:
		return L"";
	}
}
