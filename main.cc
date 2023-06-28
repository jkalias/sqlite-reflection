//
//  main.cpp
//  Reflectable
//
//  Created by Ioannis Kaliakatsos on 24.06.2023.
//

#include <iostream>

#include "database.h"
#include "pet.h"
#include "person.h"

int main(int argc, const char * argv[]) {
	auto n1 = typeid(&Person::first_name).name();

    auto n2 = typeid(&Person::last_name).name();
    auto n3 = typeid(&Person::age).name();
    auto n4 = typeid(&Person::salary).name();

#if !defined(_WIN32) && !defined(WIN32)
    Database::Initialize("/Users/nemesis/Desktop/Reflectable/Reflectable/src/sample.db");
#else
    Database::Initialize("D:/workspace/Reflectable/src/sample.db");
#endif
    const auto persons = Database::FetchAll<Person>();
    for (const auto& p : persons) {
        std::cout << "[Person] first name: " << p.first_name.c_str() << ", last name: " << p.last_name.c_str() << ", age: " << p.age << "\n";
    }

    return 0;
}
