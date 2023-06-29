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

int main(int argc, const char* argv[]) {
    auto h = GetReflectionRegisterInstance()->records[typeid(Person).name()].members;
    auto f1 = OffsetFromStart(&Person::first_name);
    auto f2 = OffsetFromStart(&Person::salary);
    
#if !defined(_WIN32) && !defined(WIN32)
    Database::Initialize("/Users/nemesis/Desktop/Reflectable/Reflectable/src/sample.db");
#else
	Database::Initialize("D:/workspace/Reflectable/src/sample.db");
#endif
	const auto persons = Database::FetchAll<Person>();
	for (const auto& p : persons) {
		std::cout << "[Person] first name: " << StringUtilities::ToUtf8(p.first_name) << ", last name: " << StringUtilities::ToUtf8(p.last_name) << ", age: " << p.age << ", salary: " << p.salary << "\n";
	}

	return 0;
}
