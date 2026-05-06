[![CMake Build Matrix](https://github.com/jkalias/sqlite-reflection/actions/workflows/cmake.yml/badge.svg)](https://github.com/jkalias/sqlite-reflection/actions/workflows/cmake.yml)
[![GitHub license](https://img.shields.io/github/license/jkalias/sqlite-reflection)](https://github.com/jkalias/sqlite-reflection/blob/main/LICENSE)

# sqlite-reflection

A C++ wrapper for SQLite that provides compile-time checked CRUD operations for reflected C++ structs.

## Table of contents

- [Motivation](#motivation)
- [Quick start](#quick-start)
- [Defining records](#defining-records)
- [Opening and closing the database](#opening-and-closing-the-database)
- [Saving records](#saving-records)
- [Fetching records](#fetching-records)
- [Updating records](#updating-records)
- [Deleting records](#deleting-records)
- [Date and time fields](#date-and-time-fields)
- [Raw SQL](#raw-sql)
- [Build instructions](#build-instructions)
- [Dependencies](#dependencies)

## Motivation

SQLite is a battle-tested, cross-platform database for local persistence. It is a good fit when an application does not need server-side storage, or when local backup and offline access are required.

SQLite exposes a C API. That API is powerful, but it can be verbose in C++ code: callers need to manage SQLite handles, prepare statements, bind values, step through results, and clean up resources correctly. Raw SQL is also easy to mistype and hard to check at compile time.

This library is inspired by how other languages and frameworks approach data persistence. In C#, [Entity Framework](https://en.wikipedia.org/wiki/Entity_Framework) lets programmers focus on modelling the domain while delegating database management, table creation and naming, member type mapping, and related concerns to the framework. In Swift, [keypaths](https://developer.apple.com/documentation/swift/keypath) enable type-safe code that can be checked at compile time. Objective-C, Swift’s predecessor, also made extensive use of key paths in [Core Data](https://developer.apple.com/documentation/coredata), Apple’s persistence framework, which commonly uses SQLite as its underlying storage engine.

`sqlite-reflection` lets you model persistent data as C++ structs and use a typed API for common database operations. Its goals are:

- a native C++ API for object persistence
- compile-time checked member references for predicates
- automatic registration of reflected record types
- simple CRUD operations without hand-written SQL for normal use
- an MIT license suitable for open-source and proprietary projects

## Quick start

```c++
#include "database.h"
#include "person.h"

using namespace sqlite_reflection;

int main() {
  // Open a persistent SQLite database file and create tables for all
  // reflected records that have been included in the program.
  Database::Initialize("people.sqlite");
  const auto& db = Database::Instance();

  // Save a strongly typed C++ object. The last constructor argument is the id
  // added by sqlite-reflection to every reflected record.
  Person person{L"John", L"Appleseed", 42, true, 1};
  db.Save(person);

  // Build a WHERE predicate with member pointers instead of SQL strings.
  const auto adults_named_john = GreaterThanOrEqual(&Person::age, 18)
      .And(Equal(&Person::first_name, L"John"));

  // Fetch all Person rows matching the predicate.
  const auto people = db.Fetch<Person>(&adults_named_john);

  // Close the SQLite connection during application shutdown.
  Database::Finalize();
}
```

## Defining records

Record types are declared with [X macros](https://en.wikipedia.org/wiki/X_Macro). The library expands the field list into a struct, adds an `int64_t id` field, and registers metadata used to create the SQLite table.

```c++
// person.h
#pragma once

#include <string>

#define REFLECTABLE Person
#define FIELDS \
MEMBER_TEXT(first_name) \
MEMBER_TEXT(last_name) \
MEMBER_INT(age) \
MEMBER_BOOL(is_vaccinated) \
FUNC(std::wstring GetFullName() const)
#include "reflection.h"

// FUNC declarations are added to the generated struct. Implement them as you
// would implement any normal C++ member function.
inline std::wstring Person::GetFullName() const {
  return first_name + L" " + last_name;
}
```

The generated type is equivalent to:

```c++
struct Person {
  std::wstring first_name;
  std::wstring last_name;
  int64_t age;
  bool is_vaccinated;
  // all records gain an id for unique identification in the database
  int64_t id;

  std::wstring GetFullName() const;
};
```

Another record can be declared in the same way:

```c++
// pet.h
#pragma once

#include <string>

#define REFLECTABLE Pet
#define FIELDS \
MEMBER_TEXT(name) \
MEMBER_REAL(weight)
#include "reflection.h"
```

Supported field macros:

| C++ field type | Macro |
| --- | --- |
| `int64_t` | `MEMBER_INT(name)` |
| `double` | `MEMBER_REAL(name)` |
| `std::wstring` | `MEMBER_TEXT(name)` |
| `bool` | `MEMBER_BOOL(name)` |
| `sqlite_reflection::TimePoint` | `MEMBER_DATETIME(name)` |
| member function declaration | `FUNC(signature)` |

Make sure each reflected record header is included by your program before `Database::Initialize()` is called. During initialization, the library creates one table for each registered record type if that table does not already exist.

## Opening and closing the database

All database access goes through the singleton `Database` wrapper.

```c++
#include "database.h"

using namespace sqlite_reflection;

void StartApplication(const std::string& db_path) {
  // Use a file path for an on-disk database.
  Database::Initialize(db_path);
}

void StopApplication() {
  // Finalize releases the singleton database connection.
  Database::Finalize();
}
```

Passing an empty path creates an in-memory database, which is useful for tests:

```c++
// An empty path creates an in-memory SQLite database.
Database::Initialize("");
```

## Saving records

Use `Save` when you want to provide the `id` yourself.

```c++
const auto& db = Database::Instance();

// Save one record with an explicit id.
Person peter{L"Peter", L"Meier", 32, true, 5};
db.Save(peter);

// Save multiple records in one call.
std::vector<Person> people;
people.push_back({L"Mary", L"Poppins", 29, false, 6});
people.push_back({L"Jane", L"Doe", 41, true, 7});
db.Save(people);
```

Use `SaveAutoIncrement` when you want the library to assign the next available `id` for the saved row. The input object is copied; its `id` member is not modified.

```c++
// Omit the id and let sqlite-reflection assign the next available value.
Person new_person{L"John", L"Doe", 28, false};
db.SaveAutoIncrement(new_person);

// The same auto-increment behavior is available for batches.
std::vector<Person> more_people;
more_people.push_back({L"Ada", L"Lovelace", 36, true});
more_people.push_back({L"Grace", L"Hopper", 85, true});
db.SaveAutoIncrement(more_people);
```

## Fetching records

Fetch all records of a type:

```c++
// SELECT * FROM Person;
const auto people = db.FetchAll<Person>();
```

Fetch one record by `id`:

```c++
// Fetch throws if no Person with id 5 exists.
const auto person = db.Fetch<Person>(5);
```

Fetch records with a predicate:

```c++
// This predicate is equivalent to:
// id >= 2 AND id < 5 AND first_name = 'John'
// The generated SQL uses placeholders and binds each value separately.
const auto predicate = GreaterThanOrEqual(&Person::id, 2)
    .And(SmallerThan(&Person::id, 5))
    .And(Equal(&Person::first_name, L"John"));

// SELECT matching Person rows and hydrate them back into C++ objects.
const auto matches = db.Fetch<Person>(&predicate);
```

Available predicate helpers include:

| Predicate | Example |
| --- | --- |
| `Equal` | `Equal(&Person::first_name, L"John")` |
| `Unequal` | `Unequal(&Person::last_name, L"Doe")` |
| `GreaterThan` | `GreaterThan(&Person::age, 18)` |
| `GreaterThanOrEqual` | `GreaterThanOrEqual(&Person::age, 18)` |
| `SmallerThan` | `SmallerThan(&Person::age, 65)` |
| `SmallerThanOrEqual` | `SmallerThanOrEqual(&Person::age, 65)` |
| `Like` | `Like(&Person::first_name, L"oh")` |
| `And` | `Equal(&Person::first_name, L"John").And(GreaterThan(&Person::age, 18))` |
| `Or` | `Equal(&Person::first_name, L"John").Or(Equal(&Person::first_name, L"Jane"))` |

Predicate values are bound with SQLite prepared statements. Text payloads are treated as values, not SQL syntax.

## Updating records

Fetch or construct a record, change its fields, and pass it to `Update`. The `id` identifies the row to update.

```c++
// The row with id 5 will be replaced with these member values.
Person person = db.Fetch<Person>(5);
person.last_name = L"Rambo";
person.age = 33;

db.Update(person);
```

Multiple records can be updated in one call:

```c++
// Update each record in the vector by its id.
std::vector<Person> people = db.FetchAll<Person>();
people[0].last_name = L"Rambo";
people[1].age = 20;

db.Update(people);
```

## Deleting records

Delete by record:

```c++
// Delete the row matching person.id.
const auto person = db.Fetch<Person>(5);
db.Delete(person);
```

Delete by `id`:

```c++
// Delete the Person row whose id is 5.
db.Delete<Person>(5);
```

Delete by predicate:

```c++
// Delete every vaccinated Person younger than 30.
const auto predicate = SmallerThan(&Person::age, 30)
    .And(Equal(&Person::is_vaccinated, true));

db.Delete<Person>(&predicate);
```

## Date and time fields

Very often, a record needs to store a date-time value, by specifying the `MEMBER_DATETIME` Macro. C++ has an excellent `std::chrono` library for time points and durations, but the most convenient calendaring and formatting features only became broadly available in C++20, and compiler support can vary. This project keeps the minimum language version at C++11, so it uses a small `TimePoint` wrapper and the [date](https://github.com/HowardHinnant/date) library by Howard Hinnant, one of the main contributors behind `std::chrono`. That keeps date-time persistence portable while still storing values in a predictable ISO 8601 representation.

```c++
// event.h
#pragma once

#include <string>

#define REFLECTABLE Event
#define FIELDS \
MEMBER_TEXT(name) \
MEMBER_DATETIME(created_at)
#include "reflection.h"
```

```c++
#include "event.h"
#include "time_point.h"

using namespace sqlite_reflection;

// TimePoint(0) represents the Unix epoch.
Event event{L"Created", TimePoint(0), 1};
db.Save(event);
```

## Raw SQL

The typed CRUD API should be preferred for normal application code. It uses prepared statements and bound values for generated queries.

For advanced cases, `UnsafeSql` executes raw SQL text directly:

```c++
const auto& db = Database::Instance();

// Executes the SQL string as-is. Use only for trusted SQL.
db.UnsafeSql("DELETE FROM Person WHERE length(first_name) <= 4");
```

Only use `UnsafeSql` with trusted SQL strings. Do not concatenate user input into raw SQL.

## Build instructions

The project uses an out-of-source CMake build. The examples below assume a build directory named `build`. If no C++ standard is specified, the project uses C++11. You can select a newer standard with `-DCMAKE_CXX_STANDARD=20`.

### macOS with Xcode

```console
cd sqlite-reflection
cmake -S . -B build -G Xcode
cmake --build build
build/tests/Debug/unit_tests
```

You can also open the generated `build/sqlite-cpp-reflection.xcodeproj` in Xcode.

### macOS or Linux with Makefiles

```console
cd sqlite-reflection
cmake -S . -B build
cmake --build build
build/tests/unit_tests
```

### macOS with Homebrew GCC

Adjust the compiler paths to match the GCC version installed on your machine. This example uses gcc-11.

```console
cd sqlite-reflection
cmake \
  -S . \
  -B build \
  -DCMAKE_C_COMPILER=/opt/homebrew/bin/gcc-11 \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/g++-11
cmake --build build
build/tests/unit_tests
```

### Windows with Visual Studio

```console
cd sqlite-reflection
cmake -S . -B build
cmake --build build --config Debug
build\bin\Debug\unit_tests.exe
```

You can also open the generated `build/sqlite-cpp-reflection.sln` in Visual Studio.

## Dependencies

- CMake 3.14 or newer
- A C++11-compatible compiler
- GoogleTest for tests, fetched by CMake
- SQLite, vendored in `src/sqlite3.c` and `src/internal/sqlite3.h`
