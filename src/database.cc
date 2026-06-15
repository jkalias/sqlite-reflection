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

#include "database.h"

#include <memory>
#include <mutex>
#include <stdexcept>

#include "internal/sqlite3.h"
#include "queries.h"

namespace sqlite_reflection {
std::shared_ptr<Database> Database::instance_ = nullptr;
std::weak_ptr<Database> Database::retired_;
std::mutex Database::instance_mutex_;

const ReflectionRegister& GetReflectionRegister() {
    return *GetReflectionRegisterInstance();
}

void Database::Initialize(const std::string& path) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (instance_ != nullptr) {
        throw std::invalid_argument("Database has already been initialized");
    }
    if (!retired_.expired()) {
        // A previous database is still kept alive by outstanding Instance() handles.
        // Creating a new one now would leave two live databases/connections that do not
        // share the same db_mutex_; refuse until those handles are released.
        throw std::runtime_error(
            "Database cannot be reinitialized while handles to the previous database are still in use");
    }

    const auto effective_path = !path.empty() ? path : ":memory:";
    instance_ = std::shared_ptr<Database>(new Database(effective_path.data()));
}

void Database::Finalize() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    // Drop the singleton's own reference. The connection is closed by ~Database once the
    // last outstanding Instance() handle is released, so an in-flight operation on another
    // thread keeps the database alive until it finishes. Track the retiring instance so a
    // subsequent Initialize() is rejected until those handles are gone.
    retired_ = instance_;
    instance_.reset();
}

Database::~Database() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
    }
}

Database::Database(const char* path) : db_(nullptr) {
    // Open in serialized threading mode so the shared connection is safe to use from
    // multiple threads; access is additionally serialized through db_mutex_.
    const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    if (sqlite3_open_v2(path, &db_, flags, nullptr)) {
        throw std::invalid_argument("Database could not be initialized");
    }

    auto& reg = GetReflectionRegister();
    for (const auto& contents : reg.records) {
        const auto& record = contents.second;
        CreateTableQuery query(db_, record);
        query.Execute();
    }
}

std::shared_ptr<const Database> Database::Instance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (instance_ == nullptr) {
        throw std::runtime_error("Database has not been initialized; call Database::Initialize() first");
    }
    return instance_;
}

FetchQueryResults Database::Fetch(const Reflection& record, const QueryPredicateBase* predicate) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    FetchRecordsQuery query(db_, record, predicate);
    return query.GetResults();
}

const Reflection& Database::GetRecord(const std::string& type_id) {
    return GetReflectionRegister().records.at(type_id);
}

int64_t Database::GetMaxId(const Reflection& record) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    FetchMaxIdQuery query(db_, record);
    return query.GetMaxId();
}

void Database::Save(void* p, const Reflection& record) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    InsertQuery query(db_, record, p);
    query.Execute();
}

int64_t Database::SaveAutoIncrement(void* p, const Reflection& record) const {
    // The lock spans both the insert and the rowid read so that, on the shared connection,
    // sqlite3_last_insert_rowid() reflects this insert and not one from another thread.
    std::lock_guard<std::mutex> lock(db_mutex_);
    InsertQuery query(db_, record, p, true);
    query.Execute();
    return sqlite3_last_insert_rowid(db_);
}

void Database::Update(void* p, const Reflection& record) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    UpdateQuery query(db_, record, p);
    query.Execute();
}

void Database::Delete(const Reflection& record, const QueryPredicateBase* predicate) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    DeleteQuery query(db_, record, predicate);
    query.Execute();
}

void Database::UnsafeSql(const std::string& raw_sql_query) const {
    std::lock_guard<std::mutex> lock(db_mutex_);
    SqlQuery sql(db_, raw_sql_query);
    sql.Execute();
}
}  // namespace sqlite_reflection
