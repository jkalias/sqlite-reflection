//
//  database.hpp
//  Reflectable
//
//  Created by Ioannis Kaliakatsos on 25.06.2023.
//

#pragma once

#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include "reflection.hpp"

struct sqlite3;
struct sqlite3_stmt;

static std::map<int, std::function<void*(sqlite3_stmt *,int)>> _value_callback_mapping;

class Database
{
public:
    static void initialize(std::string path);
    static void finalize();
    
    template <typename T>
    static std::vector<T> fetchAll() {
        auto typeId = typeid(T).name();
        const auto& queryResult = fetchEntries(typeId);
        const auto& record = getRecord(typeId);
        return hydrate<T>(queryResult, record);
    }
    
private:
    static sqlite3 *db;
    static void executeQuery(std::string query);
    
    struct QueryResults {
        std::vector<std::string> columnNames;
        std::vector<std::vector<std::string>> rowValues;
    };
    
    static QueryResults fetchEntries(const std::string& typeId);
    
    template <typename T>
    static std::vector<T> hydrate(const QueryResults &queryResult, const _Reflection &record) {
        std::vector<T> models;
        for (auto i = 0; i < queryResult.rowValues.size(); i++) {
            T model;
            for (auto j = 0; j < queryResult.columnNames.size(); j++) {
                auto currentColumn = queryResult.columnNames[j];
                auto memberIndex = record.member_index_mapping.at(currentColumn);
                auto currentTrait = record.members[memberIndex].trait;
                
                switch (currentTrait) {
                    case _ReflectionMemberTrait::INT:
                    {
                        auto& v = (*(int*)((void*)_getMemberAddress(&model, record, memberIndex)));
                        v = atoi(queryResult.rowValues[i][j].data());
                        break;
                    }
                        
                    case _ReflectionMemberTrait::REAL:
                    {
                        auto& v = (*(double*)((void*)_getMemberAddress(&model, record, memberIndex)));
                        v = atof(queryResult.rowValues[i][j].data());
                        break;
                    }
                        
                    case _ReflectionMemberTrait::TEXT:
                    case _ReflectionMemberTrait::UNICODE_TEXT:
                    {
                        auto& v = (*(std::string*)((void*)_getMemberAddress(&model, record, memberIndex)));
                        v = queryResult.rowValues[i][j].data();
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
    
    static const _Reflection& getRecord(const std::string& typeId);
    
public:
    Database(Database const&)        = delete;
    void operator=(Database const&)  = delete;
};
