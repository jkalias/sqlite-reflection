#include "person.hpp"

std::wstring Person::fullName() const
{
    return first_name + L" " + last_name;
}
