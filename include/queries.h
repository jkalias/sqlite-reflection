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

#include "reflection.h"
struct sqlite3;
struct sqlite3_stmt;

namespace sqlite_reflection {
    class REFLECTION_EXPORT Query
    {
    public:
        virtual ~Query() = default;

    protected:
        Query(sqlite3* db, const Reflection& record);
        virtual std::string PrepareSql() const = 0;
        std::string JoinedRecordColumnNames() const;
        std::vector<std::string> GetRecordColumnNames() const;
        virtual std::string CustomizedColumnName(const std::string& name) const;
        
        sqlite3* db_;
        const Reflection &record_;
    };


// ------------------------------------------------------------------------


	class REFLECTION_EXPORT ExecutionQuery : public Query
	{
	public:
        virtual ~ExecutionQuery() = default;
		explicit ExecutionQuery(sqlite3* db, const Reflection& record);
		void Execute();
        
    protected:
        std::vector<std::string> GetValues(void* p) const;
	};

// ------------------------------------------------------------------------


    class REFLECTION_EXPORT CreateTableQuery final : public ExecutionQuery
    {
    public:
        ~CreateTableQuery() = default;
        explicit CreateTableQuery(sqlite3* db, const Reflection& record);

    protected:
        std::string PrepareSql() const override;
        std::string CustomizedColumnName(const std::string& name) const override;
    };


// ------------------------------------------------------------------------


    class REFLECTION_EXPORT DeleteQuery final : public ExecutionQuery
    {
    public:
        ~DeleteQuery() = default;
        explicit DeleteQuery(sqlite3* db, const Reflection& record, int64_t id);

    protected:
        std::string PrepareSql() const override;
        int64_t id_;
    };


// ------------------------------------------------------------------------


    class REFLECTION_EXPORT InsertQuery final : public ExecutionQuery
    {
    public:
        ~InsertQuery() = default;
        explicit InsertQuery(sqlite3* db, const Reflection& record, void* p);

    protected:
        std::string PrepareSql() const override;
        std::string JoinedValues() const;
        void* p_;
    };


// ------------------------------------------------------------------------


    class REFLECTION_EXPORT UpdateQuery final : public ExecutionQuery
    {
    public:
        ~UpdateQuery() = default;
        explicit UpdateQuery(sqlite3* db, const Reflection& record, void* p);

    protected:
        std::string PrepareSql() const override;
        void* p_;
    };


// ------------------------------------------------------------------------
    struct QueryResults;

    class REFLECTION_EXPORT ResultsQuery : public Query
    {
    public:
        explicit ResultsQuery(sqlite3* db, const Reflection& record);
        virtual ~ResultsQuery();
        
        QueryResults GetResults();
        static void Hydrate(void *p, const QueryResults& query_results, const Reflection& record, size_t i);
        
    protected:
        sqlite3_stmt* stmt_;

    private:
        std::wstring GetColumnValue(const int col);
    };


// ------------------------------------------------------------------------


	class REFLECTION_EXPORT FetchRecordsQuery final : public ResultsQuery
	{
	public:
        explicit FetchRecordsQuery(sqlite3* db, const Reflection& record, int64_t id);
		explicit FetchRecordsQuery(sqlite3* db, const Reflection& record);
        
    protected:
        std::string PrepareSql() const override;
        std::string id_;
	};
}
