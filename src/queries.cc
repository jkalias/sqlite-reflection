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

#include "queries.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>

#include "internal/sqlite3.h"
#include "internal/string_utilities.h"
#include "query_results.h"

using namespace sqlite_reflection;

Query::Query(sqlite3* db, const Reflection& record)
: db_(db), record_(record)
{
}

std::string Query::JoinedRecordColumnNames() const {
    auto column_names = GetRecordColumnNames();
    return StringUtilities::Join(column_names, ", ");
}

std::vector<std::string> Query::GetRecordColumnNames() const {
    std::vector<std::string> column_names;
    std::transform(
                   record_.members.cbegin(),
                   record_.members.cend(),
                   std::back_inserter(column_names),
                   [&](const Reflection::Member& member){ return CustomizedColumnName(member.name); });
    return column_names;
}

std::string Query::CustomizedColumnName(const std::string& name) const {
    return name;
}

// ------------------------------------------------------------------------


ExecutionQuery::ExecutionQuery(sqlite3* db, const Reflection& record)
: Query(db, record)
{
}

void ExecutionQuery::Execute() {
    auto sql = PrepareSql();
    if (sqlite3_exec(db_, sql.data(), nullptr, nullptr, nullptr)) {
        throw std::domain_error((sql + ": Query could not be executed").data());
    }
}

// ------------------------------------------------------------------------


CreateTableQuery::CreateTableQuery(sqlite3* db, const Reflection& record)
: ExecutionQuery(db, record)
{    
}

std::string CreateTableQuery::PrepareSql() const {
    std::string sql("CREATE TABLE IF NOT EXISTS ");
    sql += record_.name + " (" + JoinedRecordColumnNames() + ");";
    return sql;
}

std::string CreateTableQuery::CustomizedColumnName(const std::string& name) const {
    return name.compare(std::string("id")) == 0
    ? name + " PRIMARY KEY"
    : name;
}

// ------------------------------------------------------------------------


DeleteQuery::DeleteQuery(sqlite3* db, const Reflection& record, int64_t id)
: ExecutionQuery(db, record), id_(id)
{
}

std::string DeleteQuery::PrepareSql() const {
    std::string sql("DELETE FROM ");
    sql += record_.name + " WHERE id = '" + StringUtilities::String(id_) + "';";
    return sql;
}


// ------------------------------------------------------------------------


InsertQuery::InsertQuery(sqlite3* db, const Reflection& record, void* p)
: ExecutionQuery(db, record), p_(p)
{
}

std::string InsertQuery::PrepareSql() const {
    std::string sql("INSERT INTO ");
    sql += record_.name + " (" + JoinedRecordColumnNames() + ") VALUES (";
    sql += JoinedValues() + ");";
    return sql;
}

std::string InsertQuery::JoinedValues() const {
    const auto& members = record_.members;
    std::vector<std::string> values;
    
    for (auto j = 0; j < members.size(); j++) {
        const auto current_column = members[j].name;
        const auto current_trait = members[j].trait;
        std::string content;

        switch (current_trait) {
        case ReflectionMemberTrait::kInt:
            {
                auto& value = (*(int64_t*)((void*)GetMemberAddress(p_, record_, j)));
                content = StringUtilities::String(value);
                break;
            }

        case ReflectionMemberTrait::kReal:
            {
                auto& value = (*(double*)((void*)GetMemberAddress(p_, record_, j)));
                content = StringUtilities::String(value);
                break;
            }

        case ReflectionMemberTrait::kText:
            {
                auto& value = (*(std::wstring*)((void*)GetMemberAddress(p_, record_, j)));
                content = StringUtilities::ToUtf8(value);
                break;
            }

        default:
            break;
        }
        values.emplace_back("'" + content + "'");
    }
    
    return StringUtilities::Join(values, ", ");
}

// ------------------------------------------------------------------------


UpdateQuery::UpdateQuery(sqlite3* db, const Reflection& record, void* p)
: ExecutionQuery(db, record), p_(p)
{
}

std::string UpdateQuery::PrepareSql() const {
    std::string sql("UPDATE ");
    sql += record_.name + " SET ";
    
    auto columns = GetRecordColumnNames();
    auto values = GetValues();
    
    std::vector<std::string> columns_with_values;
    columns_with_values.reserve(values.size());
    std::transform(columns.begin() + 1,
                   columns.end(),
                   values.begin() + 1,
                   std::back_inserter(columns_with_values),
                   [](const std::string& column, const std::string& value)
                   {
                       return column + "=" + value;
                   });
    
    sql += StringUtilities::Join(columns_with_values, ", ");
    sql += " WHERE " + columns[0] + "=" + values[0];
    sql += ";";
    
    return sql;
}

std::vector<std::string> UpdateQuery::GetValues() const {
    const auto& members = record_.members;
    std::vector<std::string> values;
    
    for (auto j = 0; j < members.size(); j++) {
        const auto current_column = members[j].name;
        const auto current_trait = members[j].trait;
        std::string content;

        switch (current_trait) {
        case ReflectionMemberTrait::kInt:
            {
                auto& value = (*(int64_t*)((void*)GetMemberAddress(p_, record_, j)));
                content = StringUtilities::String(value);
                break;
            }

        case ReflectionMemberTrait::kReal:
            {
                auto& value = (*(double*)((void*)GetMemberAddress(p_, record_, j)));
                content = StringUtilities::String(value);
                break;
            }

        case ReflectionMemberTrait::kText:
            {
                auto& value = (*(std::wstring*)((void*)GetMemberAddress(p_, record_, j)));
                content = StringUtilities::ToUtf8(value);
                break;
            }

        default:
            break;
        }
        values.emplace_back("'" + content + "'");
    }
    
    return values;
}

// ------------------------------------------------------------------------


ResultsQuery::ResultsQuery(sqlite3* db, const Reflection& record)
: Query(db, record), stmt_(nullptr)
{
}

ResultsQuery::~ResultsQuery() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
    }
}

QueryResults ResultsQuery::GetResults() {
    auto sql = PrepareSql();
    
    if (sqlite3_prepare_v2(db_, sql.data(), -1, &stmt_, nullptr)) {
        sqlite3_close(db_);
        throw std::runtime_error((sql + ": could not get results").data());
    }
    
    const auto column_count = sqlite3_column_count(stmt_);
    
    QueryResults results;
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

void ResultsQuery::Hydrate(void *p, const QueryResults &query_results, const Reflection &record, size_t i) {
    for (auto j = 0; j < query_results.column_names.size(); j++) {
        const auto current_column = query_results.column_names[j];
        const auto member_index = record.member_index_mapping.at(current_column);
        const auto current_trait = record.members[member_index].trait;
        const auto& content = query_results.row_values[i][j];
        if (content == L"") {
            continue;
        }

        switch (current_trait) {
        case ReflectionMemberTrait::kInt:
            {
                auto& v = (*(int64_t*)((void*)GetMemberAddress(p, record, member_index)));
                v = StringUtilities::Int(content);
                break;
            }

        case ReflectionMemberTrait::kReal:
            {
                auto& v = (*(double*)((void*)GetMemberAddress(p, record, member_index)));
                v = StringUtilities::Double(content);
                break;
            }

        case ReflectionMemberTrait::kText:
            {
                auto& v = (*(std::wstring*)((void*)GetMemberAddress(p, record, member_index)));
                v = content;
                break;
            }

        default:
            break;
        }
    }
}

std::wstring ResultsQuery::GetColumnValue(const int col) {
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
            
        case SQLITE_BLOB:
            return L"blob";
            
        default:
            return L"";
    }
}

// ------------------------------------------------------------------------

FetchRecordsQuery::FetchRecordsQuery(sqlite3* db, const Reflection& record)
: ResultsQuery(db, record)
{
}

std::string FetchRecordsQuery::PrepareSql() const {
    std::string sql("SELECT * FROM ");
    sql += record_.name + ";";
    return sql;
}


