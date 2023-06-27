#include "reflection.hpp"

static struct _ReflectionRegister *p = 0;
struct _ReflectionRegister * _getReflectionRegisterInstance()
{
	if (!p) {
		p = new _ReflectionRegister();
	}
	return p;
}

_Reflection & _getRecordFromTypeId(const std::string& typeId)
{
	_ReflectionRegister &instance = *_getReflectionRegisterInstance();
	auto & metaStruct = instance.records[typeId];
	return metaStruct;
}

char * _getMemberAddress(void * p, const _Reflection& record, size_t i)
{
	char *structStart = (char*)(p);
	size_t varOffset = record.members[i].offset;
	return structStart + varOffset;
}
