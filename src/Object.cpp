/*
Reference implementation of Object runtime code.

Feel free to rewrite this in other languages that can export C symbols.
*/

#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <atomic>
#include <Object/Object.h>
#include "Schema.hpp"


#define LENGTHOF(arr) (sizeof(arr) / sizeof((arr)[0]))


/** Size of Class is part of the ABI. */
static_assert(sizeof(Class) == 256, "Object Class size must be 256 bytes");


static std::atomic<uint64_t> alive{0};


static SchemaNode* rootNode_get() {
	static SchemaNode* const rootNode = new SchemaNode;
	return rootNode;
}


struct alignas(64) Object {
	const SchemaNode* schemaNode = rootNode_get();
	std::atomic<const Schema*> schema{NULL};
	/** Packed reference counts.
	Low 32 bits = strong refs, high 32 bits = weak refs.
	*/
	std::atomic<uint64_t> refs{1};
	void* slotsInline[4] = {};
	void** slotsSpill = NULL;
};


static const Schema* Object_schema_get(const Object* self) {
	const Schema* schema = self->schema.load(std::memory_order_acquire);
	if (schema)
		return schema;
	schema = SchemaNode_schema_get(self->schemaNode);
	const_cast<Object*>(self)->schema.store(schema, std::memory_order_release);
	return schema;
}


Object* Object_create() {
	Object* self = new Object;
	// assert(self);
	alive.fetch_add(1, std::memory_order_relaxed);
	return self;
}


void Object_ref(const Object* self) {
	if (!self)
		return;
	// This check isn't part of the thread-safety guarantee, but it protects against obtaining a reference within a free() function.
	uint64_t refs = self->refs.load();
	if ((refs & 0xFFFFFFFF) == 0)
		return;
	// Increment strong reference count
	const_cast<Object*>(self)->refs.fetch_add(1);
}


void Object_unref(const Object* self) {
	if (!self)
		return;
	// This check isn't part of the thread-safety guarantee, but it protects against releasing a reference within a free() function.
	uint64_t refs = self->refs.load();
	if ((refs & 0xFFFFFFFF) == 0)
		return;
	// Decrement strong reference count
	refs = const_cast<Object*>(self)->refs.fetch_sub(1);
	if ((refs & 0xFFFFFFFF) != 1)
		return;
	// Prevent the Object from being deleted during free callbacks by adding a weak reference.
	Object_weak_ref(self);
	// Remove all classes from top to bottom
	const Class* clsBottom = NULL;
	for (const SchemaNode* n = self->schemaNode; n; n = n->parent) {
		if (n->delta.type == SchemaDelta::CLASS_PUSH)
			clsBottom = n->delta.cls;
	}
	if (clsBottom)
		Object_classes_remove(const_cast<Object*>(self), clsBottom);
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
	if (refs_weak == 1 && refs_strong == 0) {
		alive.fetch_sub(1, std::memory_order_relaxed);
		free(self->slotsSpill);
		delete self;
	}
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


void Object_classes_push(Object* self, const Class* cls, void* slot) {
	if (!self || !cls || !slot)
		return;
	// Fail silently if class already existed
	const Schema* schema = Object_schema_get(self);
	if (schema->slotIndices.find(cls))
		return;
	uint32_t slotIndex = schema->slotIndices.size;
	self->schemaNode = SchemaNode_child_findOrCreate(self->schemaNode, SchemaDelta_classPush(cls));
	self->schema.store(self->schemaNode->schema.load(std::memory_order_acquire), std::memory_order_relaxed);
	// Store slot inline, or grow the spill array to its exact derived size
	if (slotIndex < LENGTHOF(self->slotsInline)) {
		self->slotsInline[slotIndex] = slot;
	}
	else {
		uint32_t spillIndex = slotIndex - LENGTHOF(self->slotsInline);
		self->slotsSpill = (void**) realloc(self->slotsSpill, (spillIndex + 1) * sizeof(void*));
		self->slotsSpill[spillIndex] = slot;
	}
}


void* Object_slots_get(const Object* self, const Class* cls) {
	// It is safe to not check cls, for performance
	if (!self)
		return NULL;
	const Schema* schema = Object_schema_get(self);
	uint32_t* slotIndex = schema->slotIndices.find(cls);
	if (!slotIndex)
		return NULL;
	if (*slotIndex < LENGTHOF(self->slotsInline))
		return self->slotsInline[*slotIndex];
	uint32_t spillIndex = *slotIndex - LENGTHOF(self->slotsInline);
	return self->slotsSpill[spillIndex];
}


void Object_classes_remove(Object* self, const Class* cls) {
	if (!self)
		return;

	// Fail silently if the object does not have the class
	bool found = false;
	for (const SchemaNode* n = self->schemaNode; n; n = n->parent) {
		if (n->delta.type == SchemaDelta::CLASS_PUSH && n->delta.cls == cls) {
			found = true;
			break;
		}
	}
	if (!found)
		return;

	// Remove classes from top down to cls (inclusive)
	for (const SchemaNode* n = self->schemaNode; n; n = n->parent) {
		if (n->delta.type != SchemaDelta::CLASS_PUSH)
			continue;
		const Class* c = n->delta.cls;
		if (c->free)
			c->free(self);
		// Set parent class
		self->schemaNode = n->parent;
		self->schema.store(self->schemaNode->schema.load(std::memory_order_acquire), std::memory_order_relaxed);
		// Stop at class cls
		if (c == cls)
			return;
	}
}


void Object_methods_push(Object* self, void* dispatcher, void* method) {
	if (!self || !dispatcher || !method)
		return;
	// Find and return existing SchemaNode with the exact delta
	SchemaDelta delta = SchemaDelta_methodPush(dispatcher, method);
	SchemaNode* child = SchemaNode_child_find(self->schemaNode, delta);
	if (child) {
		self->schemaNode = child;
		self->schema.store(child->schema.load(std::memory_order_acquire), std::memory_order_relaxed);
		return;
	}
	// Check if the dispatcher already has a method
	if (SchemaNode_method_find(self->schemaNode, dispatcher)) {
		// Don't allow overriding with a method that was already pushed
		if (SchemaNode_dispatcher_find(self->schemaNode, method))
			return;
	}
	self->schemaNode = SchemaNode_child_findOrCreate(self->schemaNode, delta);
	self->schema.store(self->schemaNode->schema.load(std::memory_order_acquire), std::memory_order_relaxed);
}


void* Object_methods_get(const Object* self, void* dispatcher) {
	if (!self)
		return NULL;
	const Schema* schema = Object_schema_get(self);
	void** method = schema->methods.find(dispatcher);
	if (!method)
		return NULL;
	return *method;
}


void* Object_supermethods_get(const Object* self, void* method) {
	if (!self)
		return NULL;
	const Schema* schema = Object_schema_get(self);
	void** supermethod = schema->supermethods.find(method);
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

	uint64_t refs = self->refs.load();
	uint32_t strong = refs & 0xFFFFFFFF;
	uint32_t weak = refs >> 32;
	int size = snprintf(s + pos, capacity - pos, "Object(%p)[%u,%u]:", self, strong, weak);
	if (size < 0) {
		free(s);
		return NULL;
	}
	pos += size;

	// Collect classes in push order
	std::vector<const Class*> classes;
	for (const SchemaNode* n = self->schemaNode; n; n = n->parent) {
		if (n->delta.type == SchemaDelta::CLASS_PUSH)
			classes.push_back(n->delta.cls);
	}

	for (size_t i = classes.size(); i > 0; i--) {
		const Class* cls = classes[i - 1];
		void* slot = Object_slots_get(self, cls);
		size = snprintf(s + pos, capacity - pos, " %s(%p)", cls->name, slot);
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


uint64_t Object_alive_get() {
	return alive.load(std::memory_order_relaxed);
}


uint64_t Object_schemaNodes_count_get() {
	return SchemaNode_count_get(rootNode_get());
}
