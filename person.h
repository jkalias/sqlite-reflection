#pragma once

#include <string>

#define REFLECTABLE Person
#define FIELDS \
MEMBER_UNICODE(first_name) \
MEMBER_UNICODE(last_name) \
MEMBER_STRING(department) \
MEMBER_INT(age) \
MEMBER_REAL(salary) \
FUNC(std::wstring GetFullName() const)
#include "reflection.h"
