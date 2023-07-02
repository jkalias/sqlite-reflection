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

#include "database.h"

#include <stdexcept>
#include <memory>
#include <iterator>

#include "internal/string_utilities.h"
#include "queries.h"
#include "internal/sqlite3.h"

namespace sqlite_reflection {
    int RegisterValueCallbackInt() {
        value_callback_mapping[SQLITE_INTEGER] = [](sqlite3_stmt* stmt, int col){
            return static_cast<void*>(std::make_shared<int>(sqlite3_column_int(stmt, col)).get());
        };
        return SQLITE_INTEGER;
    }

    static int int_value_mapping = RegisterValueCallbackInt();

    int RegisterValueCallbackFloat() {
        value_callback_mapping[SQLITE_FLOAT] = [](sqlite3_stmt* stmt, int col){
            return static_cast<void*>(std::make_shared<double>(sqlite3_column_double(stmt, col)).get());
        };
        return SQLITE_FLOAT;
    }

    static int float_value_mapping = RegisterValueCallbackFloat();

    int RegisterValueCallbackText() {
        value_callback_mapping[SQLITE_TEXT] = [](sqlite3_stmt* stmt, int col){
            return (void*)sqlite3_column_text(stmt, col);
        };
        return SQLITE_TEXT;
    }

    static int text_value_mapping = RegisterValueCallbackText();

    int RegisterValueCallbackBlob() {
        value_callback_mapping[SQLITE_BLOB] = [](sqlite3_stmt* stmt, int col){
            return (void*)sqlite3_column_blob(stmt, col);
        };
        return SQLITE_BLOB;
    }

    static int blob_value_mapping = RegisterValueCallbackBlob();
    const ReflectionRegister& GetReflectionRegister() {
        return *GetReflectionRegisterInstance();
    }

    // ------------------------------------------------------------------------

    Database* Database::instance_ = nullptr;
    void Database::Initialize(const std::string& path) {
        if (instance_ != nullptr) {
            throw std::invalid_argument("Database has already been initialized");
        }
        
        const auto effective_path = path != "" ? path : ":memory:";
        instance_ = new Database(effective_path.data());
    }

    void Database::Finalize() {
        if (instance_ != nullptr) {
            sqlite3_close(instance_->db_);
            delete instance_;
            instance_ = nullptr;
        }
    }

    Database::Database(const char* path)
    : db_(nullptr) {
        if (sqlite3_open(path, &db_)) {
            throw std::invalid_argument("Database could not be initialized");
        }
        
        auto& reg = GetReflectionRegister();
        for (const auto& contents : reg.records) {
            const auto& record = contents.second;
            CreateTableQuery query(db_, record);
            query.Execute();
        }
    }

    const Database& Database::Instance() {
        return *instance_;
    }

    QueryResults Database::FetchAll(const Reflection &record) const {
        FetchRecordsQuery fetch(db_, record);
        return fetch.GetResults();
    }

    const Reflection& Database::GetRecord(const std::string& type_id) {
        return GetReflectionRegister().records.at(type_id);
    }

    void Database::Save(void *p, const Reflection& record) const {
        InsertQuery insert(db_, record, p);
        insert.Execute();
    }
}
