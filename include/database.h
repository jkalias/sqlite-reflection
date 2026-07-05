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

#pragma once

#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

#include "queries.h"
#include "query_predicates.h"
#include "reflection.h"

struct sqlite3;

namespace sqlite_reflection {
/// A wrapper of an SQLite database, enabling type-safe and compile-time CRUD operations,
/// encapsulating the C-based API of the underlying SQLite engine
class REFLECTION_EXPORT Database {
public:
    /// Initializes the SQLite database singleton at a given file path.
    /// This function should be called before any operation is performed on the database.
    /// During initialization all reflectable structs/records are registered and their corresponding tables are created
    /// in the database. If the path is empty, an in-memory database is created.
    static void Initialize(const std::string& path = "");

    /// Releases the database singleton. The underlying connection is closed once the last
    /// holder of an Instance() handle has released it, so a concurrent in-flight operation
    /// keeps the connection alive until it completes.
    static void Finalize();

    /// Retrieves the database singleton wrapper for further operations. The returned shared
    /// handle keeps the database alive for as long as the caller holds it, so an operation in
    /// progress is never torn down by a concurrent Finalize().
    static std::shared_ptr<const Database> Instance();

    ~Database();

    Database(Database const&) = delete;
    Database(Database&&) = delete;
    Database& operator=(const Database&) = delete;
    Database& operator=(Database&&) = delete;

    /// Retrieves all entries of a given record from the database.
    /// This corresponds to a SELECT query in the SQL syntax
    template <typename T>
    std::vector<T> FetchAll() const {
        const auto type_id = typeid(T).name();
        const auto& record = GetRecord(type_id);
        EmptyPredicate empty;
        return FetchRecords<T>(record, &empty);
    }

    /// Retrieves all entries of a given record from the database, which match a given predicate.
    /// This corresponds to a SELECT query in the SQL syntax
    template <typename T>
    std::vector<T> Fetch(const QueryPredicateBase* predicate) const {
        const auto type_id = typeid(T).name();
        const auto& record = GetRecord(type_id);
        return FetchRecords<T>(record, predicate);
    }

    /// Retrieves a single entry of a given record from the database, which matches a given id.
    /// This corresponds to a SELECT query in the SQL syntax
    template <typename T>
    T Fetch(int64_t id) const {
        const auto type_id = typeid(T).name();
        const auto& record = GetRecord(type_id);
        Equal equal_id_condition(&T::id, id);
        auto models = FetchRecords<T>(record, &equal_id_condition);
        if (models.size() != 1) {
            throw std::runtime_error("No record with this id found");
        }
        return models[0];
    }

    /// Saves a given record in the database.
    /// This corresponds to an INSERT query in the SQL syntax
    template <typename T>
    void Save(const T& model) const {
        const auto type_id = typeid(T).name();
        const auto& record = GetRecord(type_id);
        Save((void*)&model, record);
    }

    /// Saves a given record in the database, letting the database assign its id.
    /// The newly generated id is written back into the passed-in model.
    /// This corresponds to an INSERT query in the SQL syntax
    template <typename T>
    void SaveAutoIncrement(T& model) const {
        const auto type_id = typeid(T).name();
        const auto& record = GetRecord(type_id);
        model.id = SaveAutoIncrement((void*)&model, record);
    }

    /// Saves multiple records iteratively in the database.
    /// This corresponds to an INSERT query in the SQL syntax
    template <typename T>
    void Save(const std::vector<T>& models) const {
        const auto type_id = typeid(T).name();
        const auto& record = GetRecord(type_id);
        for (const auto& model : models) {
            Save((void*)&model, record);
        }
    }

    /// Saves multiple records iteratively in the database, letting the database assign their ids.
    /// The newly generated ids are written back into the passed-in models.
    /// This corresponds to an INSERT query in the SQL syntax
    template <typename T>
    void SaveAutoIncrement(std::vector<T>& models) const {
        const auto type_id = typeid(T).name();
        const auto& record = GetRecord(type_id);
        for (auto& model : models) {
            model.id = SaveAutoIncrement((void*)&model, record);
        }
    }

    /// Updates a given record in the database.
    /// This corresponds to an UPDATE query in the SQL syntax
    template <typename T>
    void Update(const T& model) const {
        const auto type_id = typeid(T).name();
        const auto& record = GetRecord(type_id);
        Update((void*)&model, record);
    }

    /// Updates multiple records iteratively in the database.
    /// This corresponds to an UPDATE query in the SQL syntax
    template <typename T>
    void Update(const std::vector<T>& models) const {
        const auto type_id = typeid(T).name();
        const auto& record = GetRecord(type_id);
        for (const auto& model : models) {
            Update((void*)&model, record);
        }
    }

    /// Deletes a given record from the database.
    /// This corresponds to an DELETE query in the SQL syntax
    template <typename T>
    void Delete(const T& model) const {
        const auto type_id = typeid(T).name();
        const auto& record = GetRecord(type_id);
        const auto equal_id_predicate = Equal(&T::id, model.id);
        Delete(record, &equal_id_predicate);
    }

    /// Deletes a given record from the database, which matches a given id.
    /// This corresponds to an DELETE query in the SQL syntax
    template <typename T>
    void Delete(int64_t id) const {
        const auto type_id = typeid(T).name();
        const auto& record = GetRecord(type_id);
        const auto equal_id_predicate = Equal(&T::id, id);
        Delete(record, &equal_id_predicate);
    }

    /// Deletes multiple records of a given type from the database, which match a given predicate.
    /// This corresponds to an DELETE query in the SQL syntax, with an additional WHERE clause
    template <typename T>
    void Delete(const QueryPredicateBase* predicate) const {
        const auto type_id = typeid(T).name();
        const auto& record = GetRecord(type_id);
        Delete(record, predicate);
    }

    /// Executes a raw SQL query. A trailing semicolon is added if needed.
    /// Prefer the type-safe CRUD APIs for user-provided values.
    void UnsafeSql(const std::string& raw_sql_query) const;

private:
    explicit Database(const char* path);

    /// Returns a record type from its type information, retrieved from typeid(...).name()
    static const Reflection& GetRecord(const std::string& type_id);

    /// Executes a fetch query (SELECT) for a given record with a given predicate, streaming
    /// each matching row directly from the prepared statement into a newly constructed T -
    /// no intermediate string materialization of the result set
    template <typename T>
    std::vector<T> FetchRecords(const Reflection& record, const QueryPredicateBase* predicate) const {
        std::lock_guard<std::mutex> lock(db_mutex_);
        FetchRecordsQuery query(db_, record, predicate);
        std::vector<T> models;
        while (query.StepRow()) {
            T model;
            query.HydrateCurrentRow((void*)&model, record);
            models.emplace_back(std::move(model));
        }
        return models;
    }

    /// Saves a single record in the database
    void Save(void* p, const Reflection& record) const;

    /// Saves a single record in the database, letting SQLite assign its id,
    /// and returns the id that was generated for the inserted row
    int64_t SaveAutoIncrement(void* p, const Reflection& record) const;

    /// Updates a single record in the database
    void Update(void* p, const Reflection& record) const;

    /// Deletes a single record from the database
    void Delete(const Reflection& record, const QueryPredicateBase* predicate) const;

    static std::shared_ptr<Database> instance_;

    /// Observes the most recently retired instance. After Finalize() drops the singleton's
    /// own reference, this stays non-expired for as long as any outstanding Instance() handle
    /// keeps the previous database alive, which blocks reinitialization until those handles
    /// are released (so two live databases/connections can never coexist).
    static std::weak_ptr<Database> retired_;

    /// Guards the singleton lifecycle (Initialize / Finalize / Instance)
    static std::mutex instance_mutex_;

    sqlite3* db_;

    /// Serializes all access to the shared connection db_, so that the per-operation
    /// BEGIN/COMMIT transaction is never interleaved across threads
    mutable std::mutex db_mutex_;
};
}  // namespace sqlite_reflection
