/*
Reference implementation of Object runtime code using FlatMap.

Feel free to rewrite this in other languages that can export C symbols.
*/

#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <vector>
#include <atomic>
#include <Object/Object.h>
#include "FlatMap.hpp"


/** Size of Class is part of the ABI. */
static_assert(sizeof(Class) == 256, "Object Class size must be 256 bytes");


struct Object {
	// Classes ordered by specialization
	std::vector<const Class*> classes;
	// Quick access to data per class
	FlatMap<const Class*, void*> datas;
	// Method overloads: dispatcher function pointer -> method pointer
	FlatMap<void*, void*> methods;
	// Super methods: method pointer -> method pointer that was overridden by it
	FlatMap<void*, void*> supermethods;

	// Number of shared references
	std::atomic<size_t> refs;
};


Object* Object_create() {
	Object* self = new Object;
	assert(self);
	self->refs = 1;
	return self;
}


void Object_obtain(const Object* self) {
	if (!self)
		return;
	// This check isn't part of the thread-safety guarantee, but it protects against obtaining a reference within a finalize() or free() function.
	if (self->refs == 0)
		return;
	// Increment reference count
	++const_cast<Object*>(self)->refs;
}


void Object_release(const Object* self) {
	if (!self)
		return;
	// This check isn't part of the thread-safety guarantee, but it protects against releasing a reference within a finalize() or free() function.
	if (self->refs == 0)
		return;
	// Decrement reference count
	if (--const_cast<Object*>(self)->refs)
		return;
	// Finalize classes in reverse order
	for (auto it = self->classes.rbegin(); it != self->classes.rend(); it++) {
		const Class* cls = *it;
		if (cls->finalize)
			cls->finalize(const_cast<Object*>(self));
	}
	// Free classes in reverse order
	while (!self->classes.empty()) {
		const Class* cls = self->classes.back();
		if (cls->free)
			cls->free(const_cast<Object*>(self));
		const_cast<Object*>(self)->classes.pop_back();
		// For performance, we don't need to erase the self->datas element, since calling Object_class_check() for a derived class in a Base class's free() is undefined behavior.
	}
	// Free Object
	delete self;
}


size_t Object_refs_get(const Object* self) {
	if (!self)
		return 0;
	return self->refs;
}


void Object_class_push(Object* self, const Class* cls, void* data) {
	if (!self)
		return;
	assert(cls);
	// Return if class already existed
	if (self->datas.find(cls))
		return;
	// Set class data
	self->datas.insert(cls, data);
	self->classes.push_back(cls);
}


bool Object_class_check(const Object* self, const Class* cls, void** dataOut) {
	// It is safe to not check cls, for performance
	if (!self)
		return false;
	void** data = self->datas.find(cls);
	if (!data)
		return false;
	if (dataOut)
		*dataOut = *data;
	return true;
}


void Object_method_push(Object* self, void* dispatcher, void* method) {
	if (!self)
		return;
	assert(dispatcher);
	assert(method);
	// Check if method already exists
	void** existing = self->methods.find(dispatcher);
	if (existing) {
		void* supermethod = *existing;
		// We can't re-override the same method
		assert(method != supermethod);
		// Don't replace method's supermethod if already set
		if (!self->supermethods.find(method))
			self->supermethods.insert(method, supermethod);
		*existing = method;
	}
	else {
		self->methods.insert(dispatcher, method);
	}
}


void* Object_method_get(const Object* self, void* dispatcher) {
	if (!self)
		return NULL;
	void** method = self->methods.find(dispatcher);
	if (!method)
		return NULL;
	return *method;
}


void* Object_supermethod_get(const Object* self, void* method) {
	if (!self)
		return NULL;
	void** supermethod = self->supermethods.find(method);
	if (!supermethod)
		return NULL;
	return *supermethod;
}


char* Object_inspect(const Object* self) {
	if (!self)
		return NULL;

	size_t capacity = 1024;
	size_t pos = 0;
	char* s = (char*) malloc(capacity);
	if (!s)
		return NULL;

	int size = snprintf(s + pos, capacity - pos, "Object(%p):", self);
	if (size < 0) {
		free(s);
		return NULL;
	}
	pos += size;

	for (const Class* cls : self->classes) {
		void* data = NULL;
		Object_class_check(self, cls, &data);
		size = snprintf(s + pos, capacity - pos, " %s(%p)", cls->name, data);
		if (size < 0)
			break;
		pos += size;
		if (pos >= capacity) {
			pos = capacity - 1;
			break;
		}
	}
	s = (char*) realloc(s, pos + 1);
	return s;
}
