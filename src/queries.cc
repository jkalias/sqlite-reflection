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

#include "sqlite3.h"
#include "string_utilities.h"
#include "query_results.h"

using namespace sqlite_reflection;

Query::Query(sqlite3* db, const Reflection& record)
: db_(db), record_(record)
{
}


// ------------------------------------------------------------------------


CreateTableQuery::CreateTableQuery(sqlite3* db, const Reflection& record)
: Query(db, record)
{    
}

void CreateTableQuery::Execute() {
    auto sql = PrepareSql();
    if (sqlite3_exec(db_, sql.data(), nullptr, nullptr, nullptr)) {
        throw std::domain_error((sql + ": Query could not be executed").data());
    }
}

std::string CreateTableQuery::PrepareSql() const {
    std::string sql("CREATE TABLE IF NOT EXISTS ");
    sql += record_.name + " (";
    
    std::vector<std::string> column_names;
    std::transform(
                   record_.members.cbegin(),
                   record_.members.cend(),
                   std::back_inserter(column_names),
                   [](const Reflection::Member& member){ return member.name; });
    
    sql += StringUtilities::Join(column_names, ", ");
    sql += ");";
    
    return sql;
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


