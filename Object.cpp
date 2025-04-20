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
	// Quick access to data per class
	std::map<const Class*, void*> datas;
	// Classes ordered by specialization
	std::vector<const Class*> classes;
};


Object* Object_create() {
	Object* self = new Object;
	return self;
}


void Object_free(Object* self) {
	if (!self)
		return;
	// Finalize classes in reverse order
	for (auto it = self->classes.rbegin(); it != self->classes.rend(); it++) {
		const Class* cls = *it;
		if (cls->finalize)
			cls->finalize(self);
	}
	// De-init classes in reverse order
	while (!self->classes.empty()) {
		const Class* cls = self->classes.back();
		if (cls->free)
			cls->free(self);
		self->classes.pop_back();
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
	// Push class and data
	self->datas[cls] = data;
	self->classes.push_back(cls);
}


bool Object_checkClass(const Object* self, const Class* cls, void** dataOut) {
	// It is safe to not check cls, for performance
	if (!self)
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
	printf("Object(%p):", self);
	for (const Class* cls : self->classes) {
		void* data = NULL;
		Object_checkClass(self, cls, &data);
		printf(" %s(%p)", cls->name, data);
	}
	printf("\n");
}
