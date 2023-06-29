// MIT License
//
// Copyright (c) 2023 Ioannis Kaliakatsos
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef REFLECTION_INTERNAL
#define REFLECTION_INTERNAL

#include <vector>
#include <map>
#include <string>
#include <functional>

#include "reflection_export.h"

#if !defined(_WIN32) && !defined(WIN32)
#include <stddef.h>
#endif

enum class ReflectionMemberTrait
{
	kInt,
	kReal,
	kText,
	kBlob
};

struct Reflection
{
	Reflection() : initialized(false), size(0) {};

	class Member
	{
	public:
		Member(const std::string& _name, ReflectionMemberTrait _trait, size_t _offset)
			: name(_name), trait(_trait), column_type(ToColumnType(_trait)), offset(_offset) { }

		std::string name;
		ReflectionMemberTrait trait;
		std::string column_type;
		size_t offset;

	private:
		static const char* ToColumnType(const ReflectionMemberTrait trait) {
			switch (trait) {
			case ReflectionMemberTrait::kInt:
				return "INTEGER";
			case ReflectionMemberTrait::kReal:
				return "REAL";
			case ReflectionMemberTrait::kText:
				return "TEXT";
			case ReflectionMemberTrait::kBlob:
				return "BLOB";
			default:
				return "";
			}
		}
	};

	bool initialized;
	std::string name;
	size_t size;
	std::vector<Member> members;
	std::map<std::string, size_t> member_index_mapping;
	std::map<std::string, std::function<void(void*, void*)>> key_value_update_mapping;
	std::map<std::string, std::function<std::string(void*)>> value_serialization_mapping;
};

template <typename T, typename R>
size_t OffsetFromStart(R T::* fn) {
	const auto sf = sizeof(fn);
	char bytes_f[sf];
	memcpy(bytes_f, (const char*)&fn, sf);
	auto len = strlen(bytes_f);
	size_t offset = 0;
	memcpy(&offset, bytes_f, len);
	return offset;
}

#define STR_NOEXPAND(A) #A
#define STR(A) STR_NOEXPAND(A)

#define CAT_NOEXPAND(A, B) A##B
#define CAT(A, B) CAT_NOEXPAND(A, B)

#define DEFINE_MEMBER(R, T)	            reflectable.members.push_back(Reflection::Member(STR(R), T, offsetof(struct REFLECTABLE, R))); \
										reflectable.member_index_mapping[STR(R)] = reflectable.members.size() - 1;

#define DEFINE_KEY_PATH(L, R)           reflectable.key_value_update_mapping[STR(R)] = [](void *p, void *v) {   \
                                                REFLECTABLE& model = (*(REFLECTABLE*)(p));                      \
                                                L& value = (*(L*)(v));                                          \
                                                model.*(&REFLECTABLE::R) = value;                               \
                                        };

#define DEFINE_SERIALIZATION(L, R)      reflectable.value_serialization_mapping[STR(R)] = [](void *v) {    \
											return std::to_string(*(L*)v);                                      \
										};

#define ASSIGN_COPY_NOCAT(x) x = _r.x;
#define ASSIGN_COPY(x) ASSIGN_COPY_NOCAT(x)
#define COPY_MEMBER(x) ASSIGN_COPY(x)

struct ReflectionRegister
{
	std::map<std::string, Reflection> records;
};

REFLECTION_EXPORT ReflectionRegister* GetReflectionRegisterInstance();
REFLECTION_EXPORT Reflection& GetRecordFromTypeId(const std::string& type_id);
REFLECTION_EXPORT char* GetMemberAddress(void* p, const Reflection& record, size_t i);

#endif // REFLECTION_INTERNAL

#if defined (REFLECTABLE) && defined (FIELDS)

#ifndef REFLECTABLE_DLL_EXPORT
#define REFLECTABLE_DLL_EXPORT
#endif

#pragma warning (push)
#pragma warning( disable:4002) // "too many actual parameters for macro 'MEMBER'"

struct REFLECTABLE_DLL_EXPORT REFLECTABLE {
    // member declaration
#define MEMBER_DECLARE(L, R)            L R;
#define MEMBER_INT(R)				    MEMBER_DECLARE(int, R)
#define MEMBER_REAL(R)			        MEMBER_DECLARE(double, R)
#define MEMBER_TEXT(R)	                MEMBER_DECLARE(std::wstring, R)
#define MEMBER_BLOB(L, R)				MEMBER_DECLARE(L, R)
#define FUNC(SIGNATURE)
    FIELDS
#undef MEMBER_DECLARE
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_TEXT
#undef MEMBER_BLOB
#undef FUNC
    
    // struct default constructor
    REFLECTABLE()
    {
        // Initialize members
#define MEMBER_INT(R)				    R = 0;
#define MEMBER_REAL(R)			        R = 0.0;
#define MEMBER_TEXT(R)
#define MEMBER_BLOB(L, R)
#define FUNC(SIGNATURE)
        FIELDS
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_TEXT
#undef MEMBER_BLOB
#undef FUNC
    };
    
    // copy constructor
    REFLECTABLE(const REFLECTABLE &_r)
    {
        operator=(_r);
    }
    
    // assignment operator
    const REFLECTABLE & operator = (const REFLECTABLE & _r)
    {
        if (&_r == this) {
            return *this;
        }
        // copy members
#define MEMBER_INT(R)					    COPY_MEMBER(R)
#define MEMBER_REAL(R)					    COPY_MEMBER(R)
#define MEMBER_TEXT(R)		                COPY_MEMBER(R)
#define MEMBER_BLOB(L, R)				    COPY_MEMBER(R)
#define FUNC(SIGNATURE)
        FIELDS
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_TEXT
#undef MEMBER_BLOB
#undef FUNC
        return *this;
    };
    
    // custom function declaration
#define MEMBER_INT(R)
#define MEMBER_REAL(R)
#define MEMBER_TEXT(R)
#define MEMBER_BLOB(L, R)
#define FUNC(SIGNATURE) SIGNATURE;
    FIELDS
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_TEXT
#undef MEMBER_BLOB
#undef FUNC
};

static std::string CAT(Register, REFLECTABLE)() {
    std::string type_id = typeid(REFLECTABLE).name();
    std::string name = STR(REFLECTABLE);
    auto & reflectable = GetRecordFromTypeId(type_id);
    if (!reflectable.initialized) {
        reflectable.name = name;
        
        // store member information
#define MEMBER_INT(R)                           DEFINE_MEMBER(R, ReflectionMemberTrait::kInt)
#define MEMBER_REAL(R)                          DEFINE_MEMBER(R, ReflectionMemberTrait::kReal)
#define MEMBER_TEXT(R)                          DEFINE_MEMBER(R, ReflectionMemberTrait::kText)
#define MEMBER_BLOB(L, R)                       DEFINE_MEMBER(R, ReflectionMemberTrait::kBlob)
#define FUNC(SIGNATURE)
        FIELDS
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_TEXT
#undef MEMBER_BLOB
#undef FUNC
        
        // mappings
#define MEMBER_INT(R)                           DEFINE_SERIALIZATION(int, R)
#define MEMBER_REAL(R)                          DEFINE_SERIALIZATION(double, R)
#define MEMBER_TEXT(R)
#define MEMBER_BLOB(L, R)
#define FUNC(SIGNATURE)
        FIELDS
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_TEXT
#undef MEMBER_BLOB
#undef FUNC
        
        reflectable.initialized = true;
        reflectable.size = sizeof(struct REFLECTABLE);
    }
    return name;
};

static std::string CAT(meta_registered_, REFLECTABLE) = CAT(Register, REFLECTABLE)();

#pragma warning (pop)

#undef FIELDS
#undef REFLECTABLE
#undef REFLECTABLE_DLL_EXPORT

#endif
