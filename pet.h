#pragma once

#include <string>

#define REFLECTABLE Pet
#define FIELDS \
MEMBER_UNICODE(name) \
MEMBER_INT(age) \
MEMBER_REAL(weight)
#include "reflection.h"