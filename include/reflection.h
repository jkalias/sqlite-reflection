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

// todo: rename all reflection to Record etc.

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

/// The trait of given struct member, for which SQLite-related reflection is enabled
enum class REFLECTION_EXPORT ReflectionMemberTrait
{
	kInt,
	kReal,
	kText,
	kBlob
};

/// A struct holding all information needed for introspection of user-defined structs
/// which are meant to be saved to and retrieved from an SQLite database, using
/// the technique of X macro (https://en.wikipedia.org/wiki/X_Macro)
/// A reflectable struct meant to be saved in SQLite is also called in this project a "record"
struct REFLECTION_EXPORT Reflection
{
    /// This holds the metadata of a given struct member
	class MemberMetadata
	{
	public:
		MemberMetadata(const std::string& _name, ReflectionMemberTrait _trait, size_t _offset)
			: name(_name), trait(_trait), column_type(ToColumnType(_trait)), offset(_offset) { }

        /// The struct member name, as defined in the source code
		std::string name;
        
        /// The trait of this struct member, as defined by its type in the source code
		ReflectionMemberTrait trait;
        
        /// The name of the column for this member in the corresponding SQLite Table of this struct
		std::string column_type;
        
        /// The memory offset in bytes of this member from the struct's start, including any padding bits
		size_t offset;

	private:
        /// Helper for conversion between member trait and SQLite column name
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
                {
                    throw std::domain_error("Implementation error: trait is not supported");
                    return "";
                }
			}
		}
	};

    /// The name of the corresponding struct, for which reflection is enabled, as defined in the source code
	std::string name;
    
    /// All member metadata
	std::vector<MemberMetadata> member_metadata;
};

/// Returns the offset in bytes of a reflectable struct member from the struct's start,
/// by enabling type-safe referencing of this member using a pointer-to-member function
/// https://isocpp.org/wiki/faq/pointers-to-members
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

#define DEFINE_MEMBER(R, T)	reflectable.member_metadata.push_back(Reflection::MemberMetadata(STR(R), T, offsetof(struct REFLECTABLE, R)));

#define ASSIGN_COPY_NOCAT(x) x = _r.x;
#define ASSIGN_COPY(x) ASSIGN_COPY_NOCAT(x)
#define COPY_MEMBER(x) ASSIGN_COPY(x)

/// A singleton object which holds all reflectable structs, and is guaranteed to be
/// instantiated before main.cpp starts
struct REFLECTION_EXPORT ReflectionRegister
{
    /// The keys are NOT the names of each struct as defined in source code,
    /// but the identifiers returned from typeid(...).name(), which are typically
    /// mangled in C++ and are compiler-specific
	std::map<std::string, Reflection> records;
};

/// Retrieves the singleton in a safe manner, creating it if needed
REFLECTION_EXPORT ReflectionRegister* GetReflectionRegisterInstance();

/// Retrieve the registered record from its unique identifer, generated from typeid(...).name()
REFLECTION_EXPORT Reflection& GetRecordFromTypeId(const std::string& type_id);

/// Retrieves the start memory address of a given member for this record, by providing its index
/// The index is determined from the order the struct members are defined in the source code
///
/// example:
/// struct Person {
///     double weight;          <---- corresponds to index 0
///     std::wstring name;    <---- corresponds to index 1
/// }
///
/// However, since all reflectable structs must be uniquely identified in SQLite,
/// their first member is always int64_t id
REFLECTION_EXPORT char* GetMemberAddress(void* p, const Reflection& record, size_t i);

#endif // REFLECTION_INTERNAL

#if defined (REFLECTABLE) && defined (FIELDS)

#ifndef REFLECTABLE_DLL_EXPORT
#define REFLECTABLE_DLL_EXPORT
#endif

#pragma warning (push)
#pragma warning( disable:4002) // "too many actual parameters for macro 'MEMBER'"

    struct REFLECTABLE_DLL_EXPORT REFLECTABLE {
        // member declaration according to the order given in source code
#define MEMBER_DECLARE(L, R)            L R;
                                        MEMBER_DECLARE(int64_t, id)
#define MEMBER_INT(R)				    MEMBER_DECLARE(int64_t, R)
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

        // assignment operator
        const REFLECTABLE& operator = (const REFLECTABLE& _r)
        {
            if (&_r == this) {
                return *this;
            }
            // copy members
            id = _r.id;
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

    /// Provide a static registration function for each reflectable struct
    static std::string CAT(Register, REFLECTABLE)() {
        std::string type_id = typeid(REFLECTABLE).name();
        std::string name = STR(REFLECTABLE);
        ReflectionRegister& instance = *GetReflectionRegisterInstance();
        auto isRecordRegisterd = instance.records.find(type_id) != instance.records.end();
        if (!isRecordRegisterd) {
			auto& reflectable = GetRecordFromTypeId(type_id);
            reflectable.name = name;

            // store member metadata
            DEFINE_MEMBER(id, ReflectionMemberTrait::kInt)
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
        }
        return name;
    };

    /// In order to guarantee that all reflectable structs are registered before main starts, we use the C++ feature, that static variables
    /// are initialized before the program starts. In order to trigger the registration function, we store its result to a global string
    static std::string CAT(meta_registered_, REFLECTABLE) = CAT(Register, REFLECTABLE)();

#pragma warning (pop)

#undef FIELDS
#undef REFLECTABLE
#undef REFLECTABLE_DLL_EXPORT

#endif
