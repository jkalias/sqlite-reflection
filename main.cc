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

template <typename T, typename R>
int OffsetFromStart(R T::* fn) {
    auto sf = sizeof(fn);
    char bytes_f[sf];
    memcpy(bytes_f,(const char *)&fn, sf);
    auto len = strlen(bytes_f);
    int offset = 0;
    memcpy(&offset, bytes_f, len);
    return offset;
}

int main(int argc, const char* argv[]) {
    auto h = GetReflectionRegisterInstance()->records[typeid(Person).name()].members;
    auto f1 = OffsetFromStart(&Person::first_name);
    auto f2 = OffsetFromStart(&Person::age);
    
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
