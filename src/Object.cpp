#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <Object/Object.h>


/*
Possible implementation of Object runtime code using the C++ std::map.

Feel free to rewrite this in C if you have a fast map type.
*/


/** Size of Class is part of the ABI. */
static_assert(sizeof(Class) == 256, "Object Class size must be 256 bytes");


struct Object {
	// Classes ordered by specialization
	std::vector<const Class*> classes;
	// Quick access to data per class
	std::unordered_map<const Class*, void*> datas;
	// Method overloads: dispatcher function pointer -> method pointer
	std::unordered_map<void*, void*> methods;
	// Super methods: method pointer -> method pointer that was overridden by it
	std::unordered_map<void*, void*> supermethods;

	// Number of shared references
	std::atomic<size_t> refs;
};


Object* Object_create() {
	Object* self = new Object;
	assert(self);
	self->refs = 1;
	return self;
}


void Object_obtain(Object* self) {
	if (!self)
		return;
	// This check isn't part of the thread-safety guarantee, but it protects against obtaining a reference within a finalize() or free() function.
	if (self->refs == 0)
		return;
	// Increment reference count
	++self->refs;
}


void Object_release(Object* self) {
	if (!self)
		return;
	// This check isn't part of the thread-safety guarantee, but it protects against releasing a reference within a finalize() or free() function.
	if (self->refs == 0)
		return;
	// Decrement reference count
	if (--self->refs)
		return;
	// Finalize classes in reverse order
	for (auto it = self->classes.rbegin(); it != self->classes.rend(); it++) {
		const Class* cls = *it;
		if (cls->finalize)
			cls->finalize(self);
	}
	// Free classes in reverse order
	while (!self->classes.empty()) {
		const Class* cls = self->classes.back();
		if (cls->free)
			cls->free(self);
		self->classes.pop_back();
		self->datas.erase(cls);
	}
	assert(self->datas.empty());
	// Free Object
	delete self;
}


size_t Object_refs_get(const Object* self) {
	if (!self)
		return 0;
	return self->refs;
}


void Object_class_push(Object* self, const Class* cls, void* data) {
	if (!self || !cls)
		return;
	// Set class data
	auto result = self->datas.insert({cls, data});
	// Return if class already existed
	if (!result.second)
		return;
	self->classes.push_back(cls);
}


bool Object_class_check(const Object* self, const Class* cls, void** dataOut) {
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


void Object_method_push(Object* self, void* dispatcher, void* method) {
	if (!self)
		return;
	// Try to set method
	auto result = self->methods.insert({dispatcher, method});
	// If method already exists, replace and set supermethod
	if (!result.second) {
		void* supermethod = result.first->second;
		self->supermethods[method] = supermethod;
		result.first->second = method;
	}
}


void* Object_method_get(const Object* self, void* dispatcher) {
	if (!self)
		return NULL;
	auto it = self->methods.find(dispatcher);
	if (it == self->methods.end())
		return NULL;
	return it->second;
}


void* Object_supermethod_get(const Object* self, void* method) {
	if (!self)
		return NULL;
	auto it = self->supermethods.find(method);
	if (it == self->supermethods.end())
		return NULL;
	return it->second;
}


void Object_debug(const Object* self) {
	if (!self)
		return;
	fprintf(stderr, "Object(%p):", self);
	for (const Class* cls : self->classes) {
		void* data = NULL;
		Object_class_check(self, cls, &data);
		fprintf(stderr, " %s(%p)", cls->name, data);
	}
	fprintf(stderr, "\n");
}
