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
#include <functional>

#include "reflection.h"
#include "query_predicates.h"

struct sqlite3;
struct sqlite3_stmt;

namespace sqlite_reflection {
    /// A wrapper of all SQLite queries, encapsulating the preparation and
    /// execution of queries against the SQLite database
	class REFLECTION_EXPORT Query
	{
	public:
		virtual ~Query() = default;

	protected:
		Query(sqlite3* db, const Reflection& record);
        
        /// A textual representation of the given query, in SQL language
		virtual std::string PrepareSql() const = 0;
        
        /// Returns a comma-separated string of all struct member names
		std::string JoinedRecordColumnNames() const;
        
        /// Returns all struct member names in textual format
		std::vector<std::string> GetRecordColumnNames() const;
        
        /// Hook for customization of a given table column name, used primarily
        /// for marking the column corresponding to id as PRIMARY KEY
		virtual std::string CustomizedColumnName(size_t index) const;
        
        sqlite3* db_;
        const Reflection& record_;
	};

    /// A query for which no results are expected, such as
    /// CREATE TABLE
    /// UPDATE
    /// INSERT
    /// DELETE
	class REFLECTION_EXPORT ExecutionQuery : public Query
	{
	public:
		~ExecutionQuery() override = default;
		explicit ExecutionQuery(sqlite3* db, const Reflection& record);
		void Execute() const;

	protected:
		std::vector<std::string> GetValues(void* p) const;
	};

    /// A query for creating a table for a given reflectable struct. When the database
    /// is initialized, a table for every registered reflectable struct is created
    /// This maps to CREATE TABLE IF NOT EXISTS  in SQL
	class REFLECTION_EXPORT CreateTableQuery final : public ExecutionQuery
	{
	public:
		~CreateTableQuery() override = default;
		explicit CreateTableQuery(sqlite3* db, const Reflection& record);

	protected:
		std::string PrepareSql() const override;
		std::string CustomizedColumnName(size_t index) const override;
	};

    /// A query to delete a given record from the database, by means of its id
    /// This maps to DELETE in SQL
	class REFLECTION_EXPORT DeleteQuery final : public ExecutionQuery
	{
	public:
		~DeleteQuery() override = default;
		explicit DeleteQuery(sqlite3* db, const Reflection& record, int64_t id);

	protected:
		std::string PrepareSql() const override;
		int64_t id_;
	};

    /// A query to insert a given record to the database, by supplying a given type-erased struct instance
    /// This maps to INSERT INTO in SQL
	class REFLECTION_EXPORT InsertQuery final : public ExecutionQuery
	{
	public:
		~InsertQuery() override = default;
		explicit InsertQuery(sqlite3* db, const Reflection& record, void* p);

	protected:
		std::string PrepareSql() const override;
		std::string JoinedValues() const;
		void* p_;
	};

    /// A query to update a given record to the database, by supplying a given type-erased struct instance
    /// This maps to UPDATE in SQL
	class REFLECTION_EXPORT UpdateQuery final : public ExecutionQuery
	{
	public:
		~UpdateQuery() override = default;
		explicit UpdateQuery(sqlite3* db, const Reflection& record, void* p);

	protected:
		std::string PrepareSql() const override;
		void* p_;
	};


    struct FetchQueryResults;

    /// A query for retrieving all records from the database, which match a given predicate condition
    /// This maps to SELECT * in SQL
	class REFLECTION_EXPORT FetchRecordsQuery final : public Query
	{
	public:
		explicit FetchRecordsQuery(sqlite3* db, const Reflection& record, const QueryPredicateBase& predicate);
        ~FetchRecordsQuery() override;
        
        /// Returns a textual representation of the results of the query
        FetchQueryResults GetResults();
        
        /// Reconstructs all record member values based on their concrete type,
        /// based on the textual representation of the corresponding row result
        /// of the fetch query
        static void Hydrate(void* p, const FetchQueryResults& query_results, const Reflection& record, size_t i);

	protected:
		std::string PrepareSql() const override;
        std::wstring GetColumnValue(int col) const;
        
        sqlite3_stmt* stmt_;
        const QueryPredicateBase& predicate_;
	};
}
