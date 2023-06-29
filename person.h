#pragma once

#include <string>

#define REFLECTABLE Person
#define FIELDS \
MEMBER_TEXT(first_name) \
MEMBER_TEXT(last_name) \
MEMBER_TEXT(department) \
MEMBER_INT(age) \
MEMBER_REAL(salary) \
FUNC(std::wstring GetFullName() const)
#include "reflection.h"
