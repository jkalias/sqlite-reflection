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
#if !defined(_WIN32) && !defined(WIN32)
    Database::Initialize("/Users/nemesis/Desktop/Reflectable/Reflectable/src/sample.db");
#else
	Database::Initialize("D:/workspace/Reflectable/src/sample.db");
#endif
	const auto persons = Database::FetchAll<Person>();
	for (const auto& p : persons) {
		std::cout << L"[Person] first name: " << p.first_name.c_str() << L", last name: " << p.last_name.c_str() << L", age: " << p.age << "\n";
	}

	return 0;
}
