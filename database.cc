//
//  database.cc
//  Reflectable
//
//  Created by Ioannis Kaliakatsos on 25.06.2023.
//

#include <stdexcept>
#include <memory>

#include "database.h"
#include "string_utilities.h"

#include "sqlite3.h"

int RegisterValueCallbackInt() {
    value_callback_mapping[SQLITE_INTEGER] = [](sqlite3_stmt *stmt,int col) {
        return static_cast<void*>(std::make_shared<int>(sqlite3_column_int(stmt, col)).get());
    };
    return SQLITE_INTEGER;
}
static int int_value_mapping = RegisterValueCallbackInt();

int RegisterValueCallbackFloat() {
    value_callback_mapping[SQLITE_FLOAT] = [](sqlite3_stmt *stmt,int col) {
        return static_cast<void*>(std::make_shared<double>(sqlite3_column_double(stmt, col)).get());
    };
    return SQLITE_FLOAT;
}
static int float_value_mapping = RegisterValueCallbackFloat();

int RegisterValueCallbackText() {
    value_callback_mapping[SQLITE_TEXT] = [](sqlite3_stmt *stmt,int col) {
        return (void*)sqlite3_column_text(stmt, col);
    };
    return SQLITE_TEXT;
}
static int text_value_mapping = RegisterValueCallbackText();

int RegisterValueCallbackBlob() {
    value_callback_mapping[SQLITE_BLOB] = [](sqlite3_stmt *stmt,int col) {
        return (void*)sqlite3_column_blob(stmt, col);
    };
    return SQLITE_BLOB;
}
static int blob_value_mapping = RegisterValueCallbackBlob();

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

sqlite3* Database::db_ = nullptr;

std::string ColumnTypeFromTrait(ReflectionMemberTrait trait);
std::wstring GetColumnValue(sqlite3_stmt* stmt, int col);
const ReflectionRegister& GetReflectionRegister();

void Database::Initialize(const std::string& path)
{
    StringUtilities::Initialize();
    if (db_ != nullptr) {
        throw std::invalid_argument("Database has already been initialized");
    }

    const auto result = sqlite3_open(path.data(), &db_);
    if (result) {
        throw std::invalid_argument("Database could not be initialized");
    }
    
    auto &reg = GetReflectionRegister();
    for (const auto& contents: reg.records) {
        const auto& model = contents.second;
        const auto& name = model.name;
        
        std::string create_table_query("CREATE TABLE IF NOT EXISTS ");
        create_table_query += name + " (";
        
        for (auto i = 0; i < model.members.size(); i++) {
            const auto& column = model.members[i];
            create_table_query += column.name + " " + column.column_type;
            if (i != model.members.size() - 1) {
                create_table_query += " , ";
            }
        }
        create_table_query += ")";
        
        ExecuteQuery(create_table_query);
    }
}
void Database::Finalize()
{
    sqlite3_close(db_);
}

void Database::ExecuteQuery(const std::string& query)
{
	const auto sql = query + ";";
    sqlite3_exec(db_, sql.data(), nullptr, nullptr, nullptr);
}

Database::QueryResults Database::FetchEntries(const std::string& type_id)
{
    const auto &record = GetRecord(type_id);
    std::string sql("SELECT * FROM ");
    sql += record.name + ";";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql.data(), -1, &stmt, nullptr)) {
        sqlite3_close(db_);
        throw std::domain_error("Could not fetch entries from table" + record.name);
    }

    const auto column_count = sqlite3_column_count(stmt);
    
    QueryResults results;
    results.column_names.reserve(column_count);
    for (auto i = 0; i < column_count; i++) {
        results.column_names.emplace_back(sqlite3_column_name(stmt, i));
    }
    
    while (sqlite3_step(stmt) != SQLITE_DONE) {
        std::vector<std::wstring> row;
        row.reserve(column_count);
        for (auto col = 0; col < column_count; col++) {
            /*if (col == 3) {
                auto p = value_callback_mapping[sqlite3_column_type(stmt, col)](stmt, col);
                auto g = record.value_serialization_mapping.at(results.column_names[col])(p);
                auto ok = false;
            }*/
            auto value = GetColumnValue(stmt, col);
            row.emplace_back(value);
        }
        results.row_values.emplace_back(row);
    }
    sqlite3_finalize(stmt);
    
    return results;
}
const Reflection& Database::GetRecord(const std::string& type_id)
{
    return GetReflectionRegister().records.at(type_id);
}

const ReflectionRegister& GetReflectionRegister()
{
    return *GetReflectionRegisterInstance();
}

std::wstring GetColumnValue(sqlite3_stmt* stmt, const int col)
{
	const int col_type = sqlite3_column_type(stmt, col);
    switch(col_type) {
        case SQLITE_INTEGER:
            return std::to_wstring(sqlite3_column_int(stmt, col));

        case SQLITE_FLOAT:
            return std::to_wstring(sqlite3_column_double(stmt, col));

    case SQLITE_TEXT:
	    {
		    const auto content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
            return StringUtilities::FromMultibyte(content);
	    }

        case SQLITE_BLOB:
            return L"blob";

        default:
            return L"null";
    }
}
