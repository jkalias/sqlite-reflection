#include "person.h"

std::wstring Person::GetFullName() const {
	return first_name + L" " + last_name;
}
