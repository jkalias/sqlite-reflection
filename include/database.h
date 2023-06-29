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
#include <vector>
#include <map>

#include "reflection.h"
#include "string_utilities.h"

struct sqlite3;
struct sqlite3_stmt;

static std::map<int, std::function<void*(sqlite3_stmt*, int)>> value_callback_mapping;

class Database
{
public:
	Database(Database const&) = delete;
	void operator=(Database const&) = delete;
	static void Initialize(const std::string& path);
	static void Finalize();

	template <typename T>
	static std::vector<T> FetchAll() {
		const auto type_id = typeid(T).name();
		const auto& query_result = FetchEntries(type_id);
		const auto& record = GetRecord(type_id);
		return Hydrate<T>(query_result, record);
	}

private:
	struct QueryResults
	{
		std::vector<std::string> column_names;
		std::vector<std::vector<std::wstring>> row_values;
	};

	static sqlite3* db_;
	static void ExecuteQuery(const std::string& query);
	static QueryResults FetchEntries(const std::string& type_id);
	static const Reflection& GetRecord(const std::string& type_id);

	template <typename T>
	static std::vector<T> Hydrate(const QueryResults& query_result, const Reflection& record) {
		std::vector<T> models;
		for (auto i = 0; i < query_result.row_values.size(); i++) {
			T model;
			for (auto j = 0; j < query_result.column_names.size(); j++) {
				const auto current_column = query_result.column_names[j];
				const auto member_index = record.member_index_mapping.at(current_column);
				const auto current_trait = record.members[member_index].trait;
				const auto& content = query_result.row_values[i][j];
				if (content == L"null") {
					continue;
				}

				switch (current_trait) {
				case ReflectionMemberTrait::kInt:
					{
						auto& v = (*(int*)((void*)GetMemberAddress(&model, record, member_index)));
						v = StringUtilities::Int(content);
						break;
					}

				case ReflectionMemberTrait::kReal:
					{
						auto& v = (*(double*)((void*)GetMemberAddress(&model, record, member_index)));
						v = StringUtilities::Double(content);
						break;
					}

				case ReflectionMemberTrait::kText:
					{
						auto& v = (*(std::wstring*)((void*)GetMemberAddress(&model, record, member_index)));
						v = content;
						break;
					}

				default:
					break;
				}
			}
			models.emplace_back(model);
		}
		return models;
	}
};
