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

#include <string>

#include "query.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>

#include "sqlite3.h"
#include "string_utilities.h"
#include "query_results.h"

using namespace sqlite_reflection;

std::wstring GetColumnValue(sqlite3_stmt* stmt, int col);

Query::Query() {}

CreateTableQuery::CreateTableQuery(const Reflection& record)
	: record_(record) { }

std::string CreateTableQuery::Evaluate() const {
	std::string create_table_query("CREATE TABLE IF NOT EXISTS ");
	create_table_query += record_.name + " (";

	std::vector<std::string> column_names;
	std::transform(
		record_.members.cbegin(),
		record_.members.cend(),
		std::back_inserter(column_names),
		[](const Reflection::Member& member){ return member.name; });

	create_table_query += StringUtilities::Join(column_names, ", ");

	create_table_query += ")";
	return create_table_query;
}

FetchRecordQuery::FetchRecordQuery(const Reflection& record, sqlite3* db)
	: record_(record), db_(db) { }

std::string FetchRecordQuery::Evaluate() const {
	std::string sql("SELECT * FROM ");
	sql += record_.name + ";";
	sqlite3_stmt* stmt;
	if (sqlite3_prepare_v2(db_, sql.data(), -1, &stmt, nullptr)) {
		sqlite3_close(db_);
		throw std::domain_error("Could not fetch entries from table" + record_.name);
	}

	const auto column_count = sqlite3_column_count(stmt);

	QueryResults2 results;
	results.column_names.reserve(column_count);
	for (auto i = 0; i < column_count; i++) {
		results.column_names.emplace_back(sqlite3_column_name(stmt, i));
	}

	while (sqlite3_step(stmt) != SQLITE_DONE) {
		std::vector<std::wstring> row;
		row.reserve(column_count);
		for (auto col = 0; col < column_count; col++) {
			/*if (col == 3) {
				auto p = value_callback_mapping[sqlite3_column_type(stmt, col)](stmt, col);
				auto g = record_.value_serialization_mapping.at(results.column_names[col])(p);
				auto ok = false;
			}*/
			auto value = GetColumnValue(stmt, col);
			row.emplace_back(value);
		}
		results.row_values.emplace_back(row);
	}
	sqlite3_finalize(stmt);
}

std::wstring GetColumnValue(sqlite3_stmt* stmt, const int col) {
	const int col_type = sqlite3_column_type(stmt, col);
	switch (col_type) {
	case SQLITE_INTEGER:
		return std::to_wstring(sqlite3_column_int(stmt, col));

	case SQLITE_FLOAT:
		return std::to_wstring(sqlite3_column_double(stmt, col));

	case SQLITE_TEXT:
	{
		const auto content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
		return StringUtilities::FromUtf8(content);
	}

	case SQLITE_BLOB:
		return L"blob";

	default:
		return L"null";
	}
}