[![CMake Build Matrix](https://github.com/jkalias/sqlite-reflection/actions/workflows/cmake.yml/badge.svg)](https://github.com/jkalias/sqlite-reflection/actions/workflows/cmake.yml)
[![GitHub license](https://img.shields.io/github/license/jkalias/functional_cpp)](https://github.com/jkalias/sqlite-reflection/blob/main/LICENSE)
# C++ for SQLite with compile time reflection

## Motivation
A core feature of any application is persistence of user data, usually in a database. When there is no need for server storage, or even for fallback and/or backup reasons, SQLite offers a battle-tested and cross-platform solution for local database management.

However, SQLite is written in C, and even though it exposes a [C++ API](https://www.sqlite.org/cintro.html), this tends to be rather verbose and full of details. In addition, the domain expert/programmer should always do manual bookeeping and cleanup of the relevant SQLite objects, so that no memory leaks occur. Finally, all operations are eventually expressed through raw SQL queries, which at the end is rather tedious and repetitive work.

This library is inspired by the approach other languages and frameworks take against the problem of data persistence. Specifically, in C# the [Entity Framework](https://en.wikipedia.org/wiki/Entity_Framework) allows the programmer to focus on modelling the domain, while delegating the responsibility of database management, table allocation and naming, member type annotation and naming and so on, to EF. In Swift the feature of [keypaths](https://developer.apple.com/documentation/swift/keypath) allows the programmer to write safe code, which is checked at compile time. Its predecessor Objective-C has used keypaths extensively in the [Core Data](https://developer.apple.com/documentation/coredata) Framework, which is Apple's database management software stack, using primarily SQLite in the background.

There are several C++ SQLite ORM libraries out there, however with the following limitations
* fully detailed exposure of the underlying SQL syntax and database operations
* API heavily relying on strings, so no compile-time safety can be guaranteed
* inappropriate license model for closed-source or proprietary software

The primary goals of this library are
* native C++ API for object persistence, which feels "at home" for C++ programmers
* safe code, checked at compile time, without the need to write raw SQL queries
* automatic record registration for all types used in the program, without any additional setup
* safe and easy to use API for all CRUD (Create, Read, Update, Delete) operations
* MIT license to use for any kind of software; open source or closed source/commercial.

## Detailed design
### Model domain record types and their members
In order to define a domain object for persistence, just define its name and its members. For example, the following snippet declares a struct called `Person` and another struct called `Pet`, which will both be saved in the database. This is using the technique of [X Macro](https://en.wikipedia.org/wiki/X_Macro), which will prove out to be indispensable for automation of database operations, and it's the main facilitator of reflection in this library.
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

During the database initialization phase, all record types (in the example `Person` and `Pet`) will be register in the database and the corresponding tables will be created if needed. No need for manual registration, no runtime errors due to forgotten records.

The following member attributes are allowed, based on the most commonly used SQLite column types:
* int64_t -> `MEMBER_INT`
* double -> `MEMBER_REAL`
* std::wstring -> `MEMBER_TEXT`. Wide strings are used in order to allow unicode text to be saved in the database.
* int64_t -> `MEMBER_INT`
* bool -> `MEMBER_BOOL`
* timestamp -> `MEMBER_DATETIME` (read note below)
* custom functions -> `FUNC`. The corresponding function must be provided by the programmer.

Special note for timestamps. Very often one needs to save a datetime (date with time) in the database for a given record type. C++ has an excellent `std::chrono` library to deal with time and duration, however the most useful features are available only in C++20 (and not guaranteed for all compiler vendors at the time of writing...) In order to facilitate a cross-platform solution which works all the way down to C++11, all datetimes are stored in their UTC [ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) representation, by leveraging the (awesome) [date](https://github.com/HowardHinnant/date) library of Howard Hinnant, one of the main actors behind `std::chrono`.

### Creating a database object
All database interactions are funneled through the database object. Before the database is accessed, it needs to know what record types it will operate on (as defined above), so it needs to be initialized. If you pass an empty string, an in-memory database will be created (useful for unit-testing).
```c++
// initialization needs to happen at program startup
#include "database.h"

void MySetupCode(const std::string& db_path) {
  ...
  Database::Initialize(db_path);
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
### Persist records (Save)
In order to save objects in the database, you first need to get a hold of the database object and then pass it the records for persistence. You don't _have_ to pass multiple records, you can only use one if you need to.
```c++
const auto& db = Database::Instance();
std::vector<Person> persons;

// id is here given - 5
persons.push_back({L"peter", L"meier", 32, true, 5});
... // add more records for persistence

db.Save(persons);

// if you don't want to manage the id yourself, just let the database manage it
// leave the last argument empty (it's always the id)
// persons.push_back({L"peter", L"meier", 32, true});

// this will set the record id to the next available value
// db.SaveAutoIncrement(persons);
```
### Retrieve records (Read)
In order to fetch records of a given type from the database, you first need to get a hold of the database object and then call a variant of the `Fetch` operation. 
```c++
// assume that these persons have been previously saved in the database
// {L"name1", L"surname1", 13, false, 1}
// {L"john", L"surname2", 25, false, 2}
// {L"john", L"surname3", 37, false, 3}
// {L"jame", L"surname4", 45, false, 4}
// {L"name5", L"surname5", 56, false, 5}

const auto& db = Database::Instance();

// retrieve all persons stored
const auto all_persons = db.FetchAll<Person>();

// retrieve a person from a given id
const auto specific_person = db.Fetch<Person>(5);

// create a custom predicate
const auto fetch_condition = GreaterThanOrEqual(&Person::id, 2)
                             .And(SmallerThan(&Person::id, 5))
                             .And(Equal(&Person::first_name, L"john"));

// retrieve persons with custom predicate
// this will fetch only 
// Person{L"john", L"surname2", 25, false, 2}
// and
// Person{L"john", L"surname3", 37, false, 3}
const auto fetched_persons_with_predicate = db.Fetch<Person>(&fetch_condition);
```

### Update records
Updating records couldn't be simpler: just manipulate the needed members of the given records, and ship them back to the database for update.
```c++
// assume that these persons have been previously saved in the database
// {L"john", L"doe", 28, false, 3}
// {L"mary", L"poppins", 29, false, 5}

const auto& db = Database::Instance();

// retrieve all records
std::vector<Person> persons = db.FetchAll<Person>();

// update the records as needed
persons[0].last_name = L"rambo";
persons[1].age = 20;

// update the records in the database
db.Update(persons);
```

### Delete records
Deleting records can be done in three variants: with a given id, by passing the whole record, or by a custom predicate.
```c++
// assume that these persons have been previously saved in the database
// {L"παναγιώτης", L"ανδριανόπουλος", 28, true, 3}
// {L"peter", L"meier", 32, false, 5}
// {L"mary", L"poppins", 20, true, 13}

const auto& db = Database::Instance();

const auto age_match_predicate = SmallerThan(&Person::age, 30)
    .And(Equal(&Person::is_vaccinated, true));

// this will leave only {L"peter", L"meier", 32, false, 5} in the database
db.Delete<Person>(&age_match_predicate);

// this would delete the 3rd record entry of the vector
// std::vector<Person> persons = db.FetchAll<Person>();
// db.Delete(persons[2]);

// this would delete the record entry from its id
// db.Delete<Person>(persons[1].id);
// or
// db.Delete<Person>(5);
```

### Raw SQL queries
If you want the full SQL syntax power at your fingertips, you could try the string-based raw SQL API
```c++
// assume some persons have been stored in the database
const auto& db = Database::Instance();

// execute a raw SQL query; this one will delete all persons whose name is shorter or equal than 4 characters long
db.Sql("DELETE FROM Person WHERE length(first_name) <= 4");
```

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
