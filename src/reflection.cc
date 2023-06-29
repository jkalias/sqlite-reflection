#include "reflection.h"

static ReflectionRegister* p = nullptr;

ReflectionRegister* GetReflectionRegisterInstance() {
	if (!p) {
		p = new ReflectionRegister();
	}
	return p;
}

Reflection& GetRecordFromTypeId(const std::string& type_id) {
	ReflectionRegister& instance = *GetReflectionRegisterInstance();
	auto& meta_struct = instance.records[type_id];
	return meta_struct;
}

char* GetMemberAddress(void* p, const Reflection& record, const size_t i) {
	const auto struct_start = static_cast<char*>(p);
	const size_t var_offset = record.members[i].offset;
	return struct_start + var_offset;
}
