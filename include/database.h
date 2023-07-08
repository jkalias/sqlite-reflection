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
#include "fetch_query_results.h"
#include "query_conditions.h"
#include "queries.h"

struct sqlite3;
struct sqlite3_stmt;
static std::map<int, std::function<void*(sqlite3_stmt*, int)>> value_callback_mapping;

namespace sqlite_reflection {
	class REFLECTION_EXPORT Database
	{
	public:
		static void Initialize(const std::string& path = "");
		static void Finalize();
		static const Database& Instance();

		Database(Database const&) = delete;
		void operator=(Database const&) = delete;

		template <typename T>
		std::vector<T> FetchAll() const {
			const auto type_id = typeid(T).name();
			const auto& record = GetRecord(type_id);
            EmptyCondition empty;
            const auto& query_result = Fetch(record, empty);
			return Hydrate<T>(query_result, record);
		}
        
        template <typename T>
        std::vector<T> Fetch(const ConditionBase& fetch_condition) const {
            const auto type_id = typeid(T).name();
            const auto& record = GetRecord(type_id);
            const auto& query_result = Fetch(record, fetch_condition);
            return Hydrate<T>(query_result, record);
        }

		template <typename T>
		T Fetch(int64_t id) const {
			const auto type_id = typeid(T).name();
			const auto& record = GetRecord(type_id);
            Equal equal_id_condition(&T::id, id);
			const auto& query_result = Fetch(record, equal_id_condition);
			if (query_result.row_values.size() != 1) {
				throw std::runtime_error("No record with this id found");
			}
			return Hydrate<T>(query_result, record)[0];
		}

		template <typename T>
		void Save(const T& model) const {
			const auto type_id = typeid(T).name();
			const auto& record = GetRecord(type_id);
			Save((void*)&model, record);
		}

		template <typename T>
		void Save(const std::vector<T>& models) const {
			const auto type_id = typeid(T).name();
			const auto& record = GetRecord(type_id);
			for (const auto& model : models) {
				Save((void*)&model, record);
			}
		}

		template <typename T>
		void Update(const T& model) const {
			const auto type_id = typeid(T).name();
			const auto& record = GetRecord(type_id);
			Update((void*)&model, record);
		}

		template <typename T>
		void Update(const std::vector<T>& models) const {
			const auto type_id = typeid(T).name();
			const auto& record = GetRecord(type_id);
			for (const auto& model : models) {
				Update((void*)&model, record);
			}
		}

		template <typename T>
		void Delete(const T& model) const {
			const auto type_id = typeid(T).name();
			const auto& record = GetRecord(type_id);
			Delete(model.id, record);
		}

		template <typename T>
		void Delete(int64_t id) const {
			const auto type_id = typeid(T).name();
			const auto& record = GetRecord(type_id);
			Delete(id, record);
		}

	private:
		static Database* instance_;
		sqlite3* db_;

		explicit Database(const char* path);

		FetchQueryResults Fetch(const Reflection& record, const ConditionBase& query_condition) const;

		static const Reflection& GetRecord(const std::string& type_id);

		template <typename T>
		std::vector<T> Hydrate(const FetchQueryResults& query_results, const Reflection& record) const {
			std::vector<T> models;
			for (auto i = 0; i < query_results.row_values.size(); i++) {
				T model;
				FetchRecordsQuery::Hydrate((void*)&model, query_results, record, i);
				models.emplace_back(model);
			}
			return models;
		}

		void Save(void* p, const Reflection& record) const;

		void Update(void* p, const Reflection& record) const;

		void Delete(int64_t id, const Reflection& record) const;
	};
}
