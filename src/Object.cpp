/*
Reference implementation of Object runtime code using FlatMap.

Feel free to rewrite this in other languages that can export C symbols.
*/

#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <atomic>
#include <Object/Object.h>
#include "FlatMap.hpp"


/** Size of Class is part of the ABI. */
static_assert(sizeof(Class) == 256, "Object Class size must be 256 bytes");


struct Object {
	struct Override {
		void* dispatcher;
		void* method;
	};
	struct ClassSlot {
		const Class* cls;
		std::vector<Override> overrides;
	};
	// Classes ordered by specialization
	std::vector<ClassSlot> classes;
	// Class -> data pointer
	FlatMap<const Class*, void*> datas;
	// dispatch function pointer -> method function pointer
	FlatMap<void*, void*> methods;
	// method function pointer -> overridden method function pointer
	FlatMap<void*, void*> supermethods;

	// Packed reference counts: low 32 bits = strong refs, high 32 bits = weak refs
	std::atomic<uint64_t> refs{1};
};


Object* Object_create() {
	Object* self = new Object;
	assert(self);
	return self;
}


void Object_ref(const Object* self) {
	if (!self)
		return;
	// This check isn't part of the thread-safety guarantee, but it protects against obtaining a reference within a finalize() or free() function.
	uint64_t refs = self->refs.load();
	if ((refs & 0xFFFFFFFF) == 0)
		return;
	// Increment strong reference count
	const_cast<Object*>(self)->refs.fetch_add(1);
}


void Object_unref(const Object* self) {
	if (!self)
		return;
	// This check isn't part of the thread-safety guarantee, but it protects against releasing a reference within a finalize() or free() function.
	uint64_t refs = self->refs.load();
	if ((refs & 0xFFFFFFFF) == 0)
		return;
	// Decrement strong reference count
	refs = const_cast<Object*>(self)->refs.fetch_sub(1);
	if ((refs & 0xFFFFFFFF) != 1)
		return;
	// Prevent the Object from being deleted during finalize or free callbacks by adding a weak reference.
	Object_weak_ref(self);
	// Remove all classes from top to bottom
	if (!self->classes.empty()) {
		Object_class_remove(const_cast<Object*>(self), self->classes.front().cls);
	}
	// Clear all maps so weak reference holders get clean failures
	const_cast<Object*>(self)->datas.clear();
	const_cast<Object*>(self)->methods.clear();
	const_cast<Object*>(self)->supermethods.clear();
	// Release the prevent-deletion weak reference, allowing the Object to be deleted if no other weak references remain.
	Object_weak_unref(self);
}


uint32_t Object_refs_get(const Object* self) {
	if (!self)
		return 0;
	return self->refs.load() & 0xFFFFFFFF;
}


void Object_weak_ref(const Object* self) {
	if (!self)
		return;
	const_cast<Object*>(self)->refs.fetch_add(uint64_t(1) << 32);
}


void Object_weak_unref(const Object* self) {
	if (!self)
		return;
	uint64_t refs = self->refs.load();
	if ((refs >> 32) == 0)
		return;
	// Decrement weak reference count
	refs = const_cast<Object*>(self)->refs.fetch_sub(uint64_t(1) << 32);
	uint32_t refs_strong = refs & 0xFFFFFFFF;
	uint32_t refs_weak = refs >> 32;
	// Free Object shell if this was the last weak ref and strong refs are already gone
	if (refs_weak == 1 && refs_strong == 0)
		delete self;
}


uint32_t Object_weak_refs_get(const Object* self) {
	if (!self)
		return 0;
	return self->refs.load() >> 32;
}


bool Object_weak_lock(const Object* self) {
	if (!self)
		return false;
	// Atomically increment strong refs only if currently > 0
	uint64_t refs = self->refs.load();
	while ((refs & 0xFFFFFFFF) > 0) {
		if (const_cast<Object*>(self)->refs.compare_exchange_weak(refs, refs + 1))
			return true;
	}
	return false;
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
	self->classes.push_back({cls, {}});
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


void Object_class_remove(Object* self, const Class* cls) {
	if (!self)
		return;

	// Find target class, searching from the top since removal usually targets recent classes
	auto rit = std::find_if(self->classes.rbegin(), self->classes.rend(), [&](const Object::ClassSlot& slot) {
		return slot.cls == cls;
	});
	if (rit == self->classes.rend())
		return;
	size_t target = self->classes.rend() - rit - 1;

	// Remove classes from top down to target (inclusive)
	while (self->classes.size() > target) {
		auto& slot = self->classes.back();
		const Class* c = slot.cls;

		if (c->free)
			c->free(self);

		// Revert method overrides in reverse order
		for (size_t j = slot.overrides.size(); j > 0; j--) {
			void* dispatcher = slot.overrides[j - 1].dispatcher;
			void* method = slot.overrides[j - 1].method;

			void** restorep = self->supermethods.find(method);
			void* restore = restorep ? *restorep : NULL;
			if (restorep)
				self->supermethods.erase(method);

			if (restore) {
				void** methodp = self->methods.find(dispatcher);
				if (methodp)
					*methodp = restore;
			}
			else {
				self->methods.erase(dispatcher);
			}
		}

		self->datas.erase(c);
		self->classes.pop_back();
	}
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
		// Method is already an override on another dispatcher
		if (self->supermethods.find(method))
			return;
		self->supermethods.insert(method, supermethod);
		*existing = method;
	}
	else {
		self->methods.insert(dispatcher, method);
	}
	// Record override in the current class
	if (!self->classes.empty())
		self->classes.back().overrides.push_back({dispatcher, method});
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


void Object_method_remove(Object* self, void* dispatcher, void* method) {
	if (!self)
		return;
	void** headp = self->methods.find(dispatcher);
	if (!headp)
		return;
	void* head = *headp;

	// Verify method is in the dispatcher's chain
	void* current = head;
	while (current && current != method) {
		void** currentp = self->supermethods.find(current);
		current = currentp ? *currentp : NULL;
	}
	if (current != method)
		return;

	// Save the restore point before modifying anything
	void** restorep = self->supermethods.find(method);
	void* restore = restorep ? *restorep : NULL;

	// Remove methods from head down to method (inclusive)
	current = head;
	while (true) {
		void** nextp = self->supermethods.find(current);
		void* next = nextp ? *nextp : NULL;

		// Remove from supermethods cache
		self->supermethods.erase(current);

		// Remove from owning class's override list
		for (auto& slot : self->classes) {
			auto it = std::find_if(slot.overrides.begin(), slot.overrides.end(), [&](const Object::Override& entry) {
				return entry.dispatcher == dispatcher && entry.method == current;
			});
			if (it != slot.overrides.end()) {
				slot.overrides.erase(it);
				break;
			}
		}

		if (current == method)
			break;
		current = next;
	}

	// Restore dispatcher to the method below the removed range
	if (restore)
		*headp = restore;
	else
		self->methods.erase(dispatcher);
}


char* Object_inspect(const Object* self) {
	if (!self)
		return NULL;

	size_t capacity = 1024;
	size_t pos = 0;
	char* s = (char*) malloc(capacity);
	if (!s)
		return NULL;

	uint64_t refs = self->refs.load();
	uint32_t strong = refs & 0xFFFFFFFF;
	uint32_t weak = refs >> 32;
	int size = snprintf(s + pos, capacity - pos, "Object(%p)[%u,%u]:", self, strong, weak);
	if (size < 0) {
		free(s);
		return NULL;
	}
	pos += size;

	for (auto& slot : self->classes) {
		const Class* cls = slot.cls;
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
