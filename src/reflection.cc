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

#include "reflection.h"
#include <memory>

static std::unique_ptr<ReflectionRegister> p = nullptr;

ReflectionRegister* GetReflectionRegisterInstance() {
	if (!p) {
		p = std::unique_ptr<ReflectionRegister>(new ReflectionRegister());
	}
	return p.get();
}

Reflection& GetRecordFromTypeId(const std::string& type_id) {
	ReflectionRegister& instance = *GetReflectionRegisterInstance();
	auto& meta_struct = instance.records[type_id];
	return meta_struct;
}

char* GetMemberAddress(void* precord, const Reflection& record, const size_t i) {
	const auto struct_start = static_cast<char*>(precord);
	const size_t var_offset = record.member_metadata[i].offset;
	return struct_start + var_offset;
}
