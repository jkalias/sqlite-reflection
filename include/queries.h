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
        
        sqlite3* db_;
        const Reflection &record_;
    };


// ------------------------------------------------------------------------


	class REFLECTION_EXPORT CreateTableQuery final : public Query
	{
	public:
		explicit CreateTableQuery(sqlite3* db, const Reflection& record);
		void Execute();

    protected:
        std::string PrepareSql() const override;
	};


// ------------------------------------------------------------------------
    struct QueryResults;

    class REFLECTION_EXPORT ResultsQuery : public Query
    {
    public:
        explicit ResultsQuery(sqlite3* db, const Reflection& record);
        virtual ~ResultsQuery();
        
        QueryResults GetResults();
        
    protected:
        sqlite3_stmt* stmt_;

    private:
        std::wstring GetColumnValue(const int col);
    };


// ------------------------------------------------------------------------


	class REFLECTION_EXPORT FetchRecordsQuery final : public ResultsQuery
	{
	public:
		explicit FetchRecordsQuery(sqlite3* db, const Reflection& record);
        
    protected:
        std::string PrepareSql() const override;
	};

// todo: InsertQuery
// todo: UpdateQuery
// todo: DeleteQuery
}