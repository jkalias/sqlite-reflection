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
	kUnicodeText,
	kText,
	kBlob
};

struct Reflection
{
	Reflection() : initialized(false), size(0) {};

	struct Member
	{
		std::string name;
		ReflectionMemberTrait trait;
		std::string column_type;
		size_t offset;

		Member(const std::string& _name, ReflectionMemberTrait _trait, const std::string& _columnType, size_t _offset)
			: name(_name), trait(_trait), column_type(_columnType), offset(_offset) {};
	};

	bool initialized;
	std::string name;
	size_t size;
	std::vector<Member> members;
	std::map<std::string, size_t> member_index_mapping;
	std::map<std::string, std::function<void(void*, void*)>> key_value_update_mapping;
	std::map<std::string, std::function<std::string(void*)>> value_serialization_mapping;
};

#define STR_NOEXPAND(A) #A
#define STR(A) STR_NOEXPAND(A)

#define CAT_NOEXPAND(A, B) A##B
#define CAT(A, B) CAT_NOEXPAND(A, B)

#define DEFINE_MEMBER(R, T, G)	        reflectable.members.push_back(Reflection::Member(STR(R), T, G, offsetof(struct REFLECTABLE, R))); \
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
#define MEMBER_UNICODE(R)	            MEMBER_DECLARE(std::wstring, R)
#define MEMBER_STRING(R)                MEMBER_DECLARE(std::string, R)
#define MEMBER_BLOB(L, R)				MEMBER_DECLARE(L, R)
#define FUNC(SIGNATURE)
    FIELDS
#undef MEMBER_DECLARE
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_UNICODE
#undef MEMBER_STRING
#undef MEMBER_BLOB
#undef FUNC
    
    // struct default constructor
    REFLECTABLE()
    {
        // Initialize members
#define MEMBER_INT(R)				    R = 0;
#define MEMBER_REAL(R)			        R = 0.0;
#define MEMBER_UNICODE(R)	            R = L"";
#define MEMBER_STRING(R)                R = "";
#define MEMBER_BLOB(L, R)
#define FUNC(SIGNATURE)
        FIELDS
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_UNICODE
#undef MEMBER_STRING
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
#define MEMBER_UNICODE(R)		            COPY_MEMBER(R)
#define MEMBER_STRING(R)                    COPY_MEMBER(R)
#define MEMBER_BLOB(L, R)				    COPY_MEMBER(R)
#define FUNC(SIGNATURE)
        FIELDS
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_UNICODE
#undef MEMBER_STRING
#undef MEMBER_BLOB
#undef FUNC
        return *this;
    };
    
    // custom function declaration
#define MEMBER_INT(R)
#define MEMBER_REAL(R)
#define MEMBER_UNICODE(R)
#define MEMBER_STRING(R)
#define MEMBER_BLOB(L, R)
#define FUNC(SIGNATURE) SIGNATURE;
    FIELDS
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_UNICODE
#undef MEMBER_STRING
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
#define MEMBER_INT(R)                           DEFINE_MEMBER(R, ReflectionMemberTrait::kInt, "INTEGER")
#define MEMBER_REAL(R)                          DEFINE_MEMBER(R, ReflectionMemberTrait::kReal, "REAL")
#define MEMBER_UNICODE(R)                       DEFINE_MEMBER(R, ReflectionMemberTrait::kUnicodeText, "TEXT")
#define MEMBER_STRING(R)                        DEFINE_MEMBER(R, ReflectionMemberTrait::kText, "TEXT")
#define MEMBER_BLOB(L, R)                       DEFINE_MEMBER(R, ReflectionMemberTrait::kBlob, "BLOB")
#define FUNC(SIGNATURE)
        FIELDS
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_UNICODE
#undef MEMBER_STRING
#undef MEMBER_BLOB
#undef FUNC
        
        // mappings
#define MEMBER_INT(R)                           DEFINE_SERIALIZATION(int, R)
#define MEMBER_REAL(R)                          DEFINE_SERIALIZATION(double, R)
#define MEMBER_UNICODE(R)
#define MEMBER_STRING(R)
#define MEMBER_BLOB(L, R)
#define FUNC(SIGNATURE)
        FIELDS
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_UNICODE
#undef MEMBER_STRING
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