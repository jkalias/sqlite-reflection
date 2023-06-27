//
//  database.cpp
//  Reflectable
//
//  Created by Ioannis Kaliakatsos on 25.06.2023.
//

#include <stdio.h>
#include <stdexcept>
#include <numeric>
#include "database.hpp"
#include "sqlite3.h"

int register_int_value_callback() {
    _value_callback_mapping[SQLITE_INTEGER] = [](sqlite3_stmt *stmt,int col) {
        return (void*)std::make_shared<int>(sqlite3_column_int(stmt, col)).get();
    };
    return SQLITE_INTEGER;
}
static int int_value_mapping = register_int_value_callback();

int register_float_value_callback() {
    _value_callback_mapping[SQLITE_FLOAT] = [](sqlite3_stmt *stmt,int col) {
        return (void*)(std::make_shared<double>(sqlite3_column_double(stmt, col)).get());
    };
    return SQLITE_FLOAT;
}
static int float_value_mapping = register_float_value_callback();

int register_text_value_callback() {
    _value_callback_mapping[SQLITE_TEXT] = [](sqlite3_stmt *stmt,int col) {
        return (void*)sqlite3_column_text(stmt, col);
    };
    return SQLITE_TEXT;
}
static int text_value_mapping = register_text_value_callback();

int register_blob_value_callback() {
    _value_callback_mapping[SQLITE_BLOB] = [](sqlite3_stmt *stmt,int col) {
        return (void*)sqlite3_column_blob(stmt, col);
    };
    return SQLITE_BLOB;
}
static int blob_value_mapping = register_blob_value_callback();

//
//
//
//void test(Person &model, std::wstring Person::* key_path, std::function<std::wstring(void)> value) {
//
//    std::map<std::string, std::function<void(void*, void*)>> key_value_update_mapping;
//    key_value_update_mapping["first_name"] = [](void *p, void *v) {
//        Person& model = (*(Person*)(p));
//        std::wstring& value = (*(std::wstring*)(v));
//        model.*(&Person::first_name) = value;
//    };
//
//    model.*key_path = value();
//}
//
//void update(void *p, sqlite3_stmt *stmt, int i) {
//    Person& record = (*(Person*)(p));
//    auto keyPath = &Person::first_name;
//    test(record, keyPath, []() { return L"hallo"; });
//    auto int_value = sqlite3_column_int(stmt, i);
//}

sqlite3* Database::db = nullptr;

std::string columnTypeFromTrait(_ReflectionMemberTrait trait);
std::string getColumnValue(sqlite3_stmt* stmt, int col);
const _ReflectionRegister& getReflectionRegister();

void Database::initialize(std::string path)
{
    if (db != nullptr) {
        throw std::invalid_argument("Database has already been initialized");
    }
    
    auto result = sqlite3_open(path.data(), &db);
    if (result) {
        throw std::invalid_argument("Database could not be initialized");
    }
    
    auto &reg = getReflectionRegister();
    for (const auto& contents: reg.records) {
        const auto& name = contents.first;
        const auto& model = contents.second;
        
        std::string createTableQuery("CREATE TABLE IF NOT EXISTS ");
        createTableQuery += name + " (";
        
        for (auto i = 0; i < model.members.size(); i++) {
            const auto& column = model.members[i];
            createTableQuery += column.name + " " + column.columnType;
            if (i != model.members.size() - 1) {
                createTableQuery += " , ";
            }
        }
        createTableQuery += ")";
        
        executeQuery(createTableQuery);
    }
}
void Database::finalize()
{
    sqlite3_close(db);
}

void Database::executeQuery(std::string query)
{
    auto sql = query + ";";
    sqlite3_exec(db, sql.data(), nullptr, 0, nullptr);
}

Database::QueryResults Database::fetchEntries(const std::string& typeId)
{
    const auto &record = getRecord(typeId);
    std::string sql("SELECT * FROM ");
    sql += record.name + ";";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql.data(), -1, &stmt, NULL)) {
        sqlite3_close(db);
        throw std::domain_error("Could not fetch entries from table" + record.name);
    }
    
    auto columnCount = sqlite3_column_count(stmt);
    
    Database::QueryResults results;
    results.columnNames.reserve(columnCount);
    for (auto i = 0; i < columnCount; i++) {
        results.columnNames.emplace_back(sqlite3_column_name(stmt, i));
    }
    
    while (sqlite3_step(stmt) != SQLITE_DONE) {
        std::vector<std::string> row;
        row.reserve(columnCount);
        for (auto col = 0; col < columnCount; col++) {
            if (col == 3) {
                auto p = _value_callback_mapping[sqlite3_column_type(stmt, col)](stmt, col);
                auto g = record.value_serialization_mapping.at(results.columnNames[col])(p);
                auto ok = false;
            }
            auto value = getColumnValue(stmt, col);
            row.emplace_back(value);
        }
        results.rowValues.emplace_back(row);
    }
    sqlite3_finalize(stmt);
    
    return results;
}
const _Reflection& Database::getRecord(const std::string& typeId)
{
    return getReflectionRegister().records.at(typeId);
}

const _ReflectionRegister& getReflectionRegister()
{
    return *_getReflectionRegisterInstance();
}

std::string getColumnValue(sqlite3_stmt* stmt, int col)
{
    int colType = sqlite3_column_type(stmt, col);
    switch(colType) {
        case SQLITE_INTEGER:
            return std::to_string(sqlite3_column_int(stmt, col));
            break;
            
        case SQLITE_FLOAT:
            return std::to_string(sqlite3_column_double(stmt, col));
            break;
            
        case SQLITE_TEXT:
            return std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, col)));
            break;
            
        case SQLITE_BLOB:
            return "blob";
            break;
    }
    return "null";
}
