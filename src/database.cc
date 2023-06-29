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

#include "database.h"

#include <stdexcept>
#include <memory>
#include <algorithm>
#include <iterator>

#include "string_utilities.h"
#include "sqlite3.h"

int RegisterValueCallbackInt() {
	value_callback_mapping[SQLITE_INTEGER] = [](sqlite3_stmt* stmt, int col){
		return static_cast<void*>(std::make_shared<int>(sqlite3_column_int(stmt, col)).get());
	};
	return SQLITE_INTEGER;
}

static int int_value_mapping = RegisterValueCallbackInt();

int RegisterValueCallbackFloat() {
	value_callback_mapping[SQLITE_FLOAT] = [](sqlite3_stmt* stmt, int col){
		return static_cast<void*>(std::make_shared<double>(sqlite3_column_double(stmt, col)).get());
	};
	return SQLITE_FLOAT;
}

static int float_value_mapping = RegisterValueCallbackFloat();

int RegisterValueCallbackText() {
	value_callback_mapping[SQLITE_TEXT] = [](sqlite3_stmt* stmt, int col){
		return (void*)sqlite3_column_text(stmt, col);
	};
	return SQLITE_TEXT;
}

static int text_value_mapping = RegisterValueCallbackText();

int RegisterValueCallbackBlob() {
	value_callback_mapping[SQLITE_BLOB] = [](sqlite3_stmt* stmt, int col){
		return (void*)sqlite3_column_blob(stmt, col);
	};
	return SQLITE_BLOB;
}

static int blob_value_mapping = RegisterValueCallbackBlob();

//void test(Person &model, std::wstring Person::* key_path, std::function<std::wstring(void)> value) {
//
//    std::map<std::string, std::function<void(void*, void*)>> key_value_update_mapping;
//    key_value_update_mapping["first_name"] = [](void *p, void *v) {
//        Person& model = (*(Person*)(p));
//        std::wstring& value = (*(std::wstring*)(v));
//        model.*(&Person::first_name) = value;
//    };
//
//    model.*key_path = value();
//}
//
//void update(void *p, sqlite3_stmt *stmt, int i) {
//    Person& record = (*(Person*)(p));
//    auto keyPath = &Person::first_name;
//    test(record, keyPath, []() { return L"hallo"; });
//    auto int_value = sqlite3_column_int(stmt, i);
//}

sqlite3* Database::db_ = nullptr;

std::string ColumnTypeFromTrait(ReflectionMemberTrait trait);
std::wstring GetColumnValue(sqlite3_stmt* stmt, int col);
const ReflectionRegister& GetReflectionRegister();

void Database::Initialize(const std::string& path) {
	if (db_ != nullptr) {
		throw std::invalid_argument("Database has already been initialized");
	}

	const auto effective_path = path != ""
		? path
		: ":memory:";

	const auto result = sqlite3_open(effective_path.data(), &db_);
	if (result) {
		throw std::invalid_argument("Database could not be initialized");
	}

	auto& reg = GetReflectionRegister();
	for (const auto& contents : reg.records) {
		const auto& model = contents.second;
		const auto& name = model.name;

		std::string create_table_query("CREATE TABLE IF NOT EXISTS ");
		create_table_query += name + " (";

		std::vector<std::string> column_names;
		std::transform(
			model.members.cbegin(),
			model.members.cend(),
			std::back_inserter(column_names),
			[](const Reflection::Member& member){ return member.name; });

		create_table_query += StringUtilities::Join(column_names, ", ");

		create_table_query += ")";

		ExecuteQuery(create_table_query);
	}
}

void Database::Finalize() {
	sqlite3_close(db_);
}

void Database::ExecuteQuery(const std::string& query) {
	const auto sql = query + ";";
	sqlite3_exec(db_, sql.data(), nullptr, nullptr, nullptr);
}

Database::QueryResults Database::FetchEntries(const std::string& type_id) {
	const auto& record = GetRecord(type_id);
	std::string sql("SELECT * FROM ");
	sql += record.name + ";";
	sqlite3_stmt* stmt;
	if (sqlite3_prepare_v2(db_, sql.data(), -1, &stmt, nullptr)) {
		sqlite3_close(db_);
		throw std::domain_error("Could not fetch entries from table" + record.name);
	}

	const auto column_count = sqlite3_column_count(stmt);

	QueryResults results;
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
			    auto g = record.value_serialization_mapping.at(results.column_names[col])(p);
			    auto ok = false;
			}*/
			auto value = GetColumnValue(stmt, col);
			row.emplace_back(value);
		}
		results.row_values.emplace_back(row);
	}
	sqlite3_finalize(stmt);

	return results;
}

const Reflection& Database::GetRecord(const std::string& type_id) {
	return GetReflectionRegister().records.at(type_id);
}

const ReflectionRegister& GetReflectionRegister() {
	return *GetReflectionRegisterInstance();
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
