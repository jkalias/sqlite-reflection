[![CMake Build Matrix](https://github.com/jkalias/sqlite-reflection/actions/workflows/cmake.yml/badge.svg)](https://github.com/jkalias/sqlite-reflection/actions/workflows/cmake.yml)
[![GitHub license](https://img.shields.io/github/license/jkalias/functional_cpp)](https://github.com/jkalias/sqlite-reflection/blob/main/LICENSE)
# SQLite C++ frontend with reflection

## Introduction
A core feature of any application is persistence of user data, usually in a database. When there is no need for server storage, or even for fallback and/or backup reasons, SQLite offers a battle-tested and cross-platform solution for local database management.

However, SQLite is written in C, and even though it exposes a [C++ API](https://www.sqlite.org/cintro.html), this tends to be rather verbose and full of details. In addition, the domain expert/programmer should always do manual bookeeping and cleanup of the relevant SQLite objects, so that no memory leaks occur. Finally, all operations are eventually expressed through raw SQL queries, which at the end is rather tedious and repetitive work.

This library is inspired by the approach other languages and frameworks take against the problem of data persistence. Specifically, in C# the [Entity Framework](https://en.wikipedia.org/wiki/Entity_Framework) allows the programmer to focus on modelling the domain, while delegating the responsibility of database management, table allocation and naming, member type annotation and naming and so on, to EF. In Swift the feature of [keypaths](https://developer.apple.com/documentation/swift/keypath) allows the programmer to write safe code, which is checked at compile time. Its predecessor Objective-C has used keypaths extensively in the [Core Data](https://developer.apple.com/documentation/coredata) Framework, which is Apple's database management software stack, using primarily SQLite in the background.

The primary goals of this library are
* a native C++ API for object persistence, which feels "at home" for C++ programmers
* safe code, checked at compile time, without the need to write raw SQL queries
* automatic record registration for all types used in the program, without any additional setup

## Detailed design
### Creating a database object
All database interactions are funneled through the database object. Before the database is accessed, it needs to know what record types it will operate on (more on that later), so it needs to be initialized. If you pass an empty string, an in-memory database will be created (useful for unit-testing).
```c++
// initialization needs to happen at program startup
#include "database.h"

void MySetupCode() {
  std::string path_where_database_should_be_saved;
  ...
  Database::Initialize(path_where_database_should_be_saved);
  ...
}
```

Even though it's not strictly necessary, you are encouraged to finalize the database at program shutdown
```c++
// good practice during program shutdown
#include "database.h"

void MyTearDownCode() {
  ...
  Database::Finalize();
  ...
}
```

### Defining the record types and their members
In order to define a type for persistence, just define its name and its members. For example, the following snippet declares a struct called `Person` and another struct called `Pet`. This is using the technique of [X Macro](https://en.wikipedia.org/wiki/X_Macro), which will prove out to be indispensable for automation of database operations, and it's the main facilitator of reflection in this library.
```c++
// in Person.h
#include <string>

#define REFLECTABLE Person
#define FIELDS \
MEMBER_TEXT(first_name) \
MEMBER_TEXT(last_name) \
MEMBER_INT(age) \
MEMBER_BOOL(is_vaccinated) \
FUNC(std::wstring GetFullName() const)
#include "reflection.h"

// either inline in the header or in a separate Person.cc file
std::wstring Person::GetFullName() const {
  return first_name + L" " + last_name;
}

// equivalent to
//struct Person {
//  std::wstring first_name;
//  std::wstring last_name;
//  int64_t age;
//  bool is_vaccinated;
//  int64_t id;  <--- all records gain an id for unique identification in the database
//
//  std::wstring GetFullName() const;
//}

// in Pet.h
#include <string>

#define REFLECTABLE Pet
#define FIELDS \
MEMBER_TEXT(name) \
MEMBER_REAL(weight)
#include "reflection.h"

// equivalent to
// struct Pet {
//  std::wstring name;
//  double weight;
//  int64_t id;  <--- id for database
//}
```

During the database initialization phase, all record types (here `Person` and `Pet`) will be register in the database and the corresponding tables will be created if needed. No need for manual registration, no runtime errors due to forgotten records, which did not get registered.

The following member attributes are allowed:
* int64_t -> `MEMBER_INT`
* double -> `MEMBER_REAL`
* std::wstring -> `MEMBER_TEXT`. Wide strings are used in order to allow unicode strings to be saved in the database
* int64_t -> `MEMBER_INT`
* bool -> `MEMBER_BOOL`
* custom functions -> `FUNC`. The corresponding function must be provided by the programmer.

## Compilation (Cmake)
### Dependencies
* CMake >= 3.14

### Minimum C++ version
* C++11

An out-of-source build strategy is used. All following examples assume an output build folder named ```build```. If no additional argument is passed to CMake, C++11 is used. Otherwise, you can pass ```-DCMAKE_CXX_STANDARD=20``` and it will use C++20 for example.
### macOS (Xcode)
```console
cd sqlite-reflection
cmake -S . -B build -G Xcode
```
Then open the generated ```sqlite-reflection.xcodeproj``` in the ```build``` folder.

### macOS (Makefiles/clang)
```console
cd sqlite-reflection
cmake -S . -B build
cmake --build build
build/tests/unit_tests
```

### macOS (Makefiles/g++)
Assuming you have installed Homebrew, you can then use the gcc and g++ compilers by doing the following (this example uses version gcc 11)
```console
cd sqlite-reflection
cmake \
    -S . \
    -B build \
    -DCMAKE_C_COMPILER=/opt/homebrew/Cellar/gcc/11.2.0/bin/gcc-11 \
    -DCMAKE_CXX_COMPILER=/opt/homebrew/Cellar/gcc/11.2.0/bin/g++-11
cmake --build build
build/tests/unit_tests
```

### Linux (Makefiles)
```console
cd sqlite-reflection
cmake -S . -B build
cmake --build build
build/tests/unit_tests
```

### Windows (Visual Studio)
```console
cd sqlite-reflection
cmake -S . -B build
```
Then open the generated ```sqlite-reflection.sln``` in the ```build``` folder.
