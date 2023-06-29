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

#include "string_utilities.h"

using namespace sqlite_reflection;

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
