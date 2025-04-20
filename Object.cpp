#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <map>
#include <vector>
#include "Object.h"


/*
Possible implementation of Object runtime code using the C++ std::map.

Feel free to rewrite this in C if you have a fast map type.
*/


/** Size of Class is part of the ABI. */
static_assert(sizeof(Class) == 256);


struct Object {
	// Quick access to data per Class
	std::map<const Class*, void*> datas;
	// Types ordered by specialization
	std::vector<const Class*> types;
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
		const Class* cls = *it;
		if (cls->finalize)
			cls->finalize(self);
	}
	// De-init types in reverse order
	while (!self->types.empty()) {
		const Class* cls = self->types.back();
		if (cls->free)
			cls->free(self);
		self->types.pop_back();
		self->datas.erase(cls);
	}
	assert(self->datas.empty());
	delete self;
}


void Object_pushClass(Object* self, const Class* cls, void* data) {
	if (!self || !cls)
		return;
	// Return silently if already inherited
	auto it = self->datas.find(cls);
	if (it != self->datas.end())
		return;
	// Push cls and data
	self->datas[cls] = data;
	self->types.push_back(cls);
}


bool Object_checkClass(const Object* self, const Class* cls, void** dataOut) {
	if (!self || !cls)
		return false;
	auto it = self->datas.find(cls);
	if (it == self->datas.end())
		return false;
	if (dataOut)
		*dataOut = it->second;
	return true;
}


void Object_debug(const Object* self) {
	if (!self)
		return;
	printf("Object %p\n", self);
	for (const Class* cls : self->types) {
		void* data = NULL;
		Object_checkClass(self, cls, &data);
		printf("  %s %p\n", cls->name, data);
	}
}