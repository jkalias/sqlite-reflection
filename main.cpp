//
//  main.cpp
//  Reflectable
//
//  Created by Ioannis Kaliakatsos on 24.06.2023.
//

#include <iostream>
#include "database.hpp"
#include "pet.hpp"
#include "person.hpp"

int main(int argc, const char * argv[]) {
    Person p;
//    auto v = &Person::first_name;
//    p.*v = L"test";
    
    auto n1 = typeid(&Person::first_name).name();
    auto n2 = typeid(&Person::last_name).name();
    auto n3 = typeid(&Person::age).name();
    auto n4 = typeid(&Person::salary).name();
    
    Database::initialize("/Users/nemesis/Desktop/sample.db");
    auto temp = Database::fetchAll<Person>();
    std::cout << "Hello, World!\n";
    return 0;
}
