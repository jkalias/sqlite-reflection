#pragma once

#include <string>

#define REFLECTABLE Person
#define FIELDS \
MEMBER_UNICODE_STRING(first_name) \
MEMBER_UNICODE_STRING(last_name) \
MEMBER_STRING(department) \
MEMBER_INT(age) \
MEMBER_REAL(salary) \
FUNC(std::wstring fullName() const)
#include "reflection.hpp"
