#include <cstdlib>
#include <cassert>
#include <map>
#include <vector>
#include "Object.h"


/*
Possible implementation of Object runtime code using the C++ std::map.

Feel free to rewrite this in C if you have a fast map type.
*/


static_assert(sizeof(Type) == 32 * sizeof(void*));


struct Object {
	// Quick access to data per type
	std::map<const Type*, void*> datas;
	// Types ordered by specialization
	std::vector<const Type*> types;
};


Object* Object_create() {
	Object* self = new Object;
	return self;
}


void Object_free(Object* self) {
	if (!self)
		return;
	// Finalize types in reverse order
	for (auto it = self->types.rbegin(); it != self->types.rend(); it++) {
		const Type* type = *it;
		if (type->finalize)
			type->finalize(self);
	}
	// De-init types in reverse order
	while (!self->types.empty()) {
		const Type* type = self->types.back();
		if (type->free)
			type->free(self);
		self->types.pop_back();
		self->datas.erase(type);
	}
	assert(self->datas.empty());
	delete self;
}


void Object_pushType(Object* self, const Type* type, void* data) {
	if (!self || !type)
		return;
	// Return silently if already inherited
	auto it = self->datas.find(type);
	if (it != self->datas.end())
		return;
	self->datas[type] = data;
	self->types.push_back(type);
}


bool Object_isType(const Object* self, const Type* type) {
	if (!self || !type)
		return false;
	return self->datas.find(type) != self->datas.end();
}


void* Object_getData(const Object* self, const Type* type) {
	if (!self || !type)
		return NULL;
	auto it = self->datas.find(type);
	if (it == self->datas.end())
		return NULL;
	return it->second;
}
