//
//  database.h
//  Reflectable
//
//  Created by Ioannis Kaliakatsos on 25.06.2023.
//

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

				case ReflectionMemberTrait::kUnicodeText:
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