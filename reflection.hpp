#ifndef _REFLECTIONINTERNALS_
#define _REFLECTIONINTERNALS_

#include <vector>
#include <map>
#include <string>
#include <functional>

#include "reflection_export.def"

#if !defined(_WIN32) && !defined(WIN32)
#include <stddef.h>
#endif

enum class _ReflectionMemberTrait { INT, REAL, UNICODE_TEXT, TEXT, BLOB };

struct _Reflection {
    _Reflection() : initialized(false), size(0) {};
    
    struct Member {
        std::string name;
        _ReflectionMemberTrait trait;
        std::string columnType;
        size_t offset;
        Member(const std::string& _name, _ReflectionMemberTrait _trait, const std::string& _columnType, size_t _offset)
        : name(_name), trait(_trait), columnType(_columnType), offset(_offset)
        {};
    };
    
    bool initialized;
    std::string name;
    std::string typeId;
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

#define _member_base_def_(R, T, G)	    reflectable.members.push_back(_Reflection::Member(STR(R), T, G, offsetof(struct REFLECTABLE, R))); \
                                    reflectable.member_index_mapping[STR(R)] = reflectable.members.size() - 1;


#define _member_key_path_def_(L, R)     reflectable.key_value_update_mapping[STR(R)] = [](void *p, void *v) {   \
                                                REFLECTABLE& model = (*(REFLECTABLE*)(p));                      \
                                                L& value = (*(L*)(v));                                          \
                                                model.*(&REFLECTABLE::R) = value;                               \
                                        };

#define _member_serialization_(L, R)    reflectable.value_serialization_mapping[STR(R)] = [](void *v) {    \
                                                return std::to_string(*(L*)v);                                      \
                                            };

#define ASSIGN_COPY_NOCAT(x) x = _r.x;
#define ASSIGN_COPY(x) ASSIGN_COPY_NOCAT(x)
#define COPY_MEMBER(x) ASSIGN_COPY(x)

struct _ReflectionRegister {
    std::map<std::string, _Reflection> records;
};

REFLECTION_EXPORT struct _ReflectionRegister * _getReflectionRegisterInstance();
REFLECTION_EXPORT _Reflection & _getRecordFromTypeId(const std::string& typeId);
REFLECTION_EXPORT char * _getMemberAddress(void * p, const _Reflection& record, size_t i);

#endif // _REFLECTIONINTERNALS_

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
#define MEMBER_UNICODE_STRING(R)	    MEMBER_DECLARE(std::wstring, R)
#define MEMBER_STRING(R)                MEMBER_DECLARE(std::string, R)
#define MEMBER_BLOB(L, R)				MEMBER_DECLARE(L, R)
#define FUNC(SIGNATURE)
    FIELDS
#undef MEMBER_DECLARE
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_UNICODE_STRING
#undef MEMBER_STRING
#undef MEMBER_BLOB
#undef FUNC
    
    // struct default constructor
    REFLECTABLE()
    {
        // initialize members
#define MEMBER_INT(R)				    R = 0;
#define MEMBER_REAL(R)			        R = 0.0;
#define MEMBER_UNICODE_STRING(R)	    R = L"";
#define MEMBER_STRING(R)                R = "";
#define MEMBER_BLOB(L, R)
#define FUNC(SIGNATURE)
        FIELDS
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_UNICODE_STRING
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
#define MEMBER_UNICODE_STRING(R)		    COPY_MEMBER(R)
#define MEMBER_STRING(R)                    COPY_MEMBER(R)
#define MEMBER_BLOB(L, R)				    COPY_MEMBER(R)
#define FUNC(SIGNATURE)
        FIELDS
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_UNICODE_STRING
#undef MEMBER_STRING
#undef MEMBER_BLOB
#undef FUNC
        return *this;
    };
    
    // custom function declaration
#define MEMBER_INT(R)
#define MEMBER_REAL(R)
#define MEMBER_UNICODE_STRING(R)
#define MEMBER_STRING(R)
#define MEMBER_BLOB(L, R)
#define FUNC(SIGNATURE) SIGNATURE;
    FIELDS
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_UNICODE_STRING
#undef MEMBER_STRING
#undef MEMBER_BLOB
#undef FUNC
};

static std::string CAT(_register_, REFLECTABLE)() {
    std::string typeId = typeid(REFLECTABLE).name();
    std::string name = STR(REFLECTABLE);
    auto & reflectable = _getRecordFromTypeId(typeId);
    if (!reflectable.initialized) {
        reflectable.name = name;
        reflectable.typeId = typeId;
        
        // store member information
#define MEMBER_INT(R)                           _member_base_def_(R, _ReflectionMemberTrait::INT, "INT")
#define MEMBER_REAL(R)                          _member_base_def_(R, _ReflectionMemberTrait::REAL, "REAL")
#define MEMBER_UNICODE_STRING(R)                _member_base_def_(R, _ReflectionMemberTrait::UNICODE_TEXT, "TEXT")
#define MEMBER_STRING(R)                        _member_base_def_(R, _ReflectionMemberTrait::TEXT, "TEXT")
#define MEMBER_BLOB(L, R)                       _member_base_def_(R, _ReflectionMemberTrait::BLOB, "BLOB")
#define FUNC(SIGNATURE)
        FIELDS
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_UNICODE_STRING
#undef MEMBER_STRING
#undef MEMBER_BLOB
#undef FUNC
        
        // mappings
#define MEMBER_INT(R)                           _member_serialization_(int, R)
#define MEMBER_REAL(R)                          _member_serialization_(double, R)
#define MEMBER_UNICODE_STRING(R)
#define MEMBER_STRING(R)
#define MEMBER_BLOB(L, R)
#define FUNC(SIGNATURE)
        FIELDS
#undef MEMBER_INT
#undef MEMBER_REAL
#undef MEMBER_UNICODE_STRING
#undef MEMBER_STRING
#undef MEMBER_BLOB
#undef FUNC
        
        reflectable.initialized = true;
        reflectable.size = sizeof(struct REFLECTABLE);
    }
    return name;
};

static std::string CAT(_meta_registered_, REFLECTABLE) = CAT(_register_, REFLECTABLE)();

#pragma warning (pop)

#undef FIELDS
#undef REFLECTABLE
#undef REFLECTABLE_DLL_EXPORT

#endif
