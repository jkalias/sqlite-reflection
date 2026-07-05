// MIT License
//
// Copyright (c) 2026 Ioannis Kaliakatsos
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

#include <string.h>

#include <algorithm>
#include <cstring>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "internal/sqlite3.h"
#include "internal/string_utilities.h"

using namespace sqlite_reflection;

std::string Placeholders(size_t count) {
    std::vector<std::string> placeholders(count, "?");
    return StringUtilities::Join(placeholders, ", ");
}

void BindValue(sqlite3_stmt* stmt, int index, const SqlValue& value) {
    switch (value.storage_class) {
        case SqliteStorageClass::kInt:
            sqlite3_bind_int64(stmt, index, value.int_value);
            break;

        case SqliteStorageClass::kBool:
            sqlite3_bind_int(stmt, index, value.bool_value ? 1 : 0);
            break;

        case SqliteStorageClass::kReal:
            sqlite3_bind_double(stmt, index, value.real_value);
            break;

        case SqliteStorageClass::kText:
        case SqliteStorageClass::kDateTime:
            sqlite3_bind_text(stmt, index, value.text_value.data(), static_cast<int>(value.text_value.size()),
                              SQLITE_TRANSIENT);
            break;
    }
}

void BindValues(sqlite3_stmt* stmt, const std::vector<SqlValue>& values) {
    for (auto i = 0; i < values.size(); ++i) {
        BindValue(stmt, static_cast<int>(i + 1), values[i]);
    }
}

Query::Query(sqlite3* db, const Reflection& record) : db_(db), record_(record) {}

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

ExecutionQuery::ExecutionQuery(sqlite3* db, const Reflection& record) : Query(db, record) {}

void ExecutionQuery::Execute() const {
    const auto sql = PrepareSql();
    const auto bindings = Bindings();
    if (sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr)) {
        throw std::domain_error("Fatal error in transaction start");
    }

    if (bindings.empty()) {
        if (sqlite3_exec(db_, sql.data(), nullptr, nullptr, nullptr)) {
            sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
            throw std::domain_error((sql + ": Query could not be executed").data());
        }
    } else {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql.data(), -1, &stmt, nullptr)) {
            sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
            throw std::domain_error((sql + ": Query could not be prepared").data());
        }

        BindValues(stmt, bindings);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
            throw std::domain_error((sql + ": Query could not be executed").data());
        }
        sqlite3_finalize(stmt);
    }

    if (sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr)) {
        throw std::domain_error("Fatal error in transaction commit");
    }
}

std::vector<SqlValue> ExecutionQuery::Bindings() const {
    return {};
}

std::vector<SqlValue> ExecutionQuery::GetValues(void* p, bool skip_id) const {
    const auto& members = record_.member_metadata;
    std::vector<SqlValue> values;

    // The id is always the first member (index 0); skip it when the caller asks
    for (size_t j = skip_id ? 1 : 0; j < members.size(); j++) {
        const auto current_storage_class = members[j].storage_class;
        SqlValue value;
        value.storage_class = current_storage_class;

        switch (current_storage_class) {
            case SqliteStorageClass::kInt: {
                value.int_value = *reinterpret_cast<int64_t*>(GetMemberAddress(p, record_, j));
                break;
            }

            case SqliteStorageClass::kBool: {
                value.bool_value = *reinterpret_cast<bool*>(GetMemberAddress(p, record_, j));
                break;
            }

            case SqliteStorageClass::kReal: {
                value.real_value = *reinterpret_cast<double*>(GetMemberAddress(p, record_, j));
                break;
            }

            case SqliteStorageClass::kText: {
                auto& text = *reinterpret_cast<std::wstring*>(GetMemberAddress(p, record_, j));
                value.text_value = StringUtilities::ToUtf8(text);
                break;
            }

            case SqliteStorageClass::kDateTime: {
                auto& time_point = *reinterpret_cast<TimePoint*>(GetMemberAddress(p, record_, j));
                value.text_value = StringUtilities::ToUtf8(time_point.SystemTime());
                break;
            }

            default:
                break;
        }
        values.emplace_back(value);
    }

    return values;
}

SqlQuery::SqlQuery(sqlite3* db, const std::string& sql) : ExecutionQuery(db, Reflection()), sql_(sql) {}

std::string SqlQuery::PrepareSql() const {
    return !sql_.empty() && sql_[sql_.length() - 1] != ';' ? sql_ + ";" : sql_;
}

CreateTableQuery::CreateTableQuery(sqlite3* db, const Reflection& record) : ExecutionQuery(db, record) {}

std::string CreateTableQuery::PrepareSql() const {
    std::string sql("CREATE TABLE IF NOT EXISTS ");
    sql += record_.name + " (" + JoinedRecordColumnNames() + ");";
    return sql;
}

std::string CreateTableQuery::CustomizedColumnName(size_t index) const {
    auto name = Query::CustomizedColumnName(index);
    const auto is_id = name.compare(std::string("id")) == 0;
    name += " " + record_.member_metadata[index].sqlite_column_name;

    // AUTOINCREMENT guarantees that ids are never reused, even after the row with the
    // highest id is deleted
    return is_id ? name + " PRIMARY KEY AUTOINCREMENT" : name;
}

DeleteQuery::DeleteQuery(sqlite3* db, const Reflection& record, const QueryPredicateBase* predicate)
    : ExecutionQuery(db, record), predicate_(predicate) {}

std::string DeleteQuery::PrepareSql() const {
    std::string sql("DELETE FROM ");
    sql += record_.name + " WHERE " + predicate_->Evaluate() + ";";
    return sql;
}

std::vector<SqlValue> DeleteQuery::Bindings() const {
    return predicate_->Bindings();
}

InsertQuery::InsertQuery(sqlite3* db, const Reflection& record, void* p, bool auto_increment_id)
    : ExecutionQuery(db, record), p_(p), auto_increment_id_(auto_increment_id) {}

std::string InsertQuery::PrepareSql() const {
    std::string sql("INSERT INTO ");
    sql += record_.name;

    if (auto_increment_id_) {
        // Omit the id column (index 0) so SQLite assigns the next rowid itself
        auto columns = GetRecordColumnNames();
        const std::vector<std::string> columns_without_id(columns.begin() + 1, columns.end());
        if (columns_without_id.empty()) {
            // The record has no columns besides id, so there is nothing to list;
            // let SQLite assign the rowid and use defaults for every column
            sql += " DEFAULT VALUES;";
        } else {
            sql += " (" + StringUtilities::Join(columns_without_id, ", ") + ") VALUES (";
            sql += Placeholders(columns_without_id.size()) + ");";
        }
    } else {
        sql += " (" + JoinedRecordColumnNames() + ") VALUES (";
        sql += Placeholders(record_.member_metadata.size()) + ");";
    }

    return sql;
}

std::vector<SqlValue> InsertQuery::Bindings() const {
    // For auto-increment inserts the id column is omitted from the statement, so skip the
    // id member entirely rather than reading its possibly-uninitialized value
    return GetValues(p_, auto_increment_id_);
}

UpdateQuery::UpdateQuery(sqlite3* db, const Reflection& record, void* p) : ExecutionQuery(db, record), p_(p) {}

std::string UpdateQuery::PrepareSql() const {
    std::string sql("UPDATE ");
    sql += record_.name + " SET ";

    auto columns = GetRecordColumnNames();

    std::vector<std::string> columns_with_values;
    columns_with_values.reserve(columns.size() - 1);
    std::transform(columns.begin() + 1, columns.end(), std::back_inserter(columns_with_values),
                   [](const std::string& column) { return column + "=?"; });

    sql += StringUtilities::Join(columns_with_values, ", ");
    sql += " WHERE " + columns[0] + "=?";
    sql += ";";

    return sql;
}

std::vector<SqlValue> UpdateQuery::Bindings() const {
    const auto values = GetValues(p_);
    std::vector<SqlValue> bindings;
    bindings.reserve(values.size());
    for (auto i = 1; i < values.size(); ++i) {
        bindings.emplace_back(values[i]);
    }
    bindings.emplace_back(values[0]);
    return bindings;
}

FetchRecordsQuery::FetchRecordsQuery(sqlite3* db, const Reflection& record, const QueryPredicateBase* predicate)
    : Query(db, record), stmt_(nullptr), predicate_(predicate) {}

FetchRecordsQuery::~FetchRecordsQuery() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
    }
}

bool FetchRecordsQuery::StepRow() {
    if (stmt_ == nullptr) {
        const auto sql = PrepareSql();
        if (sqlite3_prepare_v2(db_, sql.data(), -1, &stmt_, nullptr)) {
            throw std::runtime_error((sql + ": could not get results").data());
        }
        BindValues(stmt_, predicate_->Bindings());
    }
    return sqlite3_step(stmt_) != SQLITE_DONE;
}

void FetchRecordsQuery::HydrateCurrentRow(void* p, const Reflection& record) const {
    const auto column_count = record.member_metadata.size();
    for (size_t j = 0; j < column_count; j++) {
        const auto col = static_cast<int>(j);

        // A SQL NULL is left unset (member keeps its default value), regardless of the
        // member's declared storage class
        if (sqlite3_column_type(stmt_, col) == SQLITE_NULL) {
            continue;
        }

        const auto current_storage_class = record.member_metadata[j].storage_class;
        switch (current_storage_class) {
            case SqliteStorageClass::kInt: {
                auto& v = *reinterpret_cast<int64_t*>(GetMemberAddress(p, record, j));
                v = sqlite3_column_int64(stmt_, col);
                break;
            }

            case SqliteStorageClass::kBool: {
                auto& v = *reinterpret_cast<bool*>(GetMemberAddress(p, record, j));
                v = sqlite3_column_int64(stmt_, col) == 1;
                break;
            }

            case SqliteStorageClass::kReal: {
                auto& v = *reinterpret_cast<double*>(GetMemberAddress(p, record, j));
                v = sqlite3_column_double(stmt_, col);
                break;
            }

            case SqliteStorageClass::kText: {
                const auto byte_count = sqlite3_column_bytes(stmt_, col);
                if (byte_count == 0) {
                    continue;
                }
                const auto content = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, col));
                auto& v = *reinterpret_cast<std::wstring*>(GetMemberAddress(p, record, j));
                v = StringUtilities::FromUtf8(content, byte_count);
                break;
            }

            case SqliteStorageClass::kDateTime: {
                const auto byte_count = sqlite3_column_bytes(stmt_, col);
                if (byte_count == 0) {
                    continue;
                }
                const auto content = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, col));
                auto& v = *reinterpret_cast<TimePoint*>(GetMemberAddress(p, record, j));
                v = TimePoint::FromSystemTime(StringUtilities::FromUtf8(content, byte_count));
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
