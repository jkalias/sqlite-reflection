# Contributing

## Code style

This project uses LLVM tooling for formatting and lightweight linting:

- `clang-format` reads `.clang-format`.
- `clang-tidy` reads `.clang-tidy`.

The formatting style is based on Google C++ style with a 4-space indent and a
120-column limit. New and modified C++ code should follow the local naming
patterns already used by the public API:

- types and public API classes: `PascalCase`
- public member functions: `PascalCase`
- free functions used as internal helpers: `PascalCase` when matching existing
  code, otherwise descriptive lower-case names are preferred for new private
  helpers
- variables, data members, and parameters: `snake_case`
- private data members: trailing underscore, for example `db_`
- constants and enum values: existing project style, for example `kText`
- macros: `UPPER_SNAKE_CASE`

Do not reformat vendored third-party sources:

- `src/sqlite3.c`
- `src/internal/sqlite3.h`
- `src/internal/date.h`

## Formatting changed files

Run `clang-format` only on project-owned C++ files that you changed:

```console
clang-format -i include/database.h src/database.cc tests/database_test.cc
```

To inspect formatting without modifying files:

```console
clang-format --dry-run --Werror include/database.h src/database.cc tests/database_test.cc
```

## Linting

After configuring CMake with compile commands enabled, run `clang-tidy` on the
files you changed:

```console
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy src/database.cc -p build
```

Keep lint fixes focused on the code touched by the current change. Avoid broad
style-only rewrites unless the change is explicitly scoped as a formatting PR.
