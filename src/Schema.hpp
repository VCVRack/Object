#pragma once

#include <cstdint>
#include <atomic>
#include <vector>

#include <Object/Object.h>
#include "FlatMap.hpp"


struct SchemaDelta {
	enum Type : uint8_t {
		NONE,
		CLASS_PUSH,
		METHOD_PUSH,
	};
	Type type;
	union {
		// CLASS_PUSH
		const Class* cls;
		// METHOD_PUSH
		struct {
			void* dispatcher;
			void* method;
		};
	};
};


static inline SchemaDelta SchemaDelta_classPush(const Class* cls) {
	SchemaDelta delta = {};
	delta.type = SchemaDelta::CLASS_PUSH;
	delta.cls = cls;
	return delta;
}


static inline SchemaDelta SchemaDelta_methodPush(void* dispatcher, void* method) {
	SchemaDelta delta = {};
	delta.type = SchemaDelta::METHOD_PUSH;
	delta.dispatcher = dispatcher;
	delta.method = method;
	return delta;
}


static inline bool SchemaDelta_equal_is(const SchemaDelta& a, const SchemaDelta& b) {
	if (a.type != b.type)
		return false;
	if (a.type == SchemaDelta::CLASS_PUSH)
		return a.cls == b.cls;
	else if (a.type == SchemaDelta::METHOD_PUSH)
		return a.dispatcher == b.dispatcher && a.method == b.method;
	else
		return true;
}


struct Schema {
	// dispatcher method pointer -> direct method pointer
	FlatMap<void*, void*> methods;
	// method -> the method it overrode
	FlatMap<void*, void*> supermethods;
	// class -> index into Object::datas
	FlatMap<const Class*, uint32_t> dataIndices;
};


struct alignas(64) SchemaNode {
	std::atomic<const Schema*> schema{NULL};
	const SchemaNode* parent = NULL;
	SchemaDelta delta = {};
	// Next sibling in the parent's children list
	SchemaNode* sibling = NULL;
	std::atomic<SchemaNode*> children{NULL};
};


/** Builds the schema of a node by applying each ancestor delta from the root down to the node, and caches it in the node.
Thread-safe. If another thread builds the schema first, returns that schema.
This function is called infrequently, so we don't want to inline it in hot Object functions.
*/
__attribute__((noinline, cold))
static const Schema* SchemaNode_schema_build(const SchemaNode* node) {
	if (!node)
		return NULL;

	const Schema* schema = node->schema.load(std::memory_order_acquire);
	if (schema)
		return schema;

	// Collect ancestors
	std::vector<const SchemaNode*> ancestors;
	for (const SchemaNode* n = node; n; n = n->parent)
		ancestors.push_back(n);

	Schema* newSchema = new Schema;
	uint32_t classCount = 0;
	for (size_t i = ancestors.size(); i > 0; i--) {
		const SchemaDelta& delta = ancestors[i - 1]->delta;
		if (delta.type == SchemaDelta::CLASS_PUSH) {
			newSchema->dataIndices.insert(delta.cls, classCount);
			classCount++;
		}
		else if (delta.type == SchemaDelta::METHOD_PUSH) {
			void** existing = newSchema->methods.find(delta.dispatcher);
			// Override existing method and set supermethod
			if (existing) {
				newSchema->supermethods.insert(delta.method, *existing);
				*existing = delta.method;
			}
			else {
				newSchema->methods.insert(delta.dispatcher, delta.method);
			}
		}
	}
	schema = newSchema;

	const Schema* existingSchema = NULL;
	const_cast<SchemaNode*>(node)->schema.compare_exchange_strong(existingSchema, schema, std::memory_order_acq_rel, std::memory_order_acquire);
	// Another thread built the same schema first
	if (existingSchema) {
		delete schema;
		schema = existingSchema;
	}
	return schema;
}


static inline const Schema* SchemaNode_schema_get(const SchemaNode* node) {
	const Schema* schema = node->schema.load(std::memory_order_acquire);
	if (schema)
		return schema;
	return SchemaNode_schema_build(node);
}


static SchemaNode* SchemaNode_child_find(const SchemaNode* node, const SchemaDelta& delta) {
	for (SchemaNode* c = node->children.load(std::memory_order_acquire); c; c = c->sibling) {
		if (SchemaDelta_equal_is(c->delta, delta))
			return c;
	}
	return NULL;
}


static SchemaNode* SchemaNode_child_findOrCreate(const SchemaNode* node, const SchemaDelta& delta) {
	// Find child with matching delta, otherwise get first child
	SchemaNode* head = node->children.load(std::memory_order_acquire);
	for (SchemaNode* c = head; c; c = c->sibling) {
		if (SchemaDelta_equal_is(c->delta, delta))
			return c;
	}

	// Create child
	SchemaNode* child = new SchemaNode;
	child->parent = node;
	child->delta = delta;
	child->sibling = head;

	// Race to replace the node's head child until success
	while (!const_cast<SchemaNode*>(node)->children.compare_exchange_weak(head, child, std::memory_order_acq_rel, std::memory_order_acquire)) {
		// Another thread prepended children, so recheck only that new prefix
		SchemaNode* existingChild = NULL;
		for (SchemaNode* c = head; c != child->sibling; c = c->sibling) {
			if (SchemaDelta_equal_is(c->delta, delta)) {
				existingChild = c;
				break;
			}
		}
		// Another thread created the same child first
		if (existingChild) {
			delete child;
			child = existingChild;
			break;
		}
		child->sibling = head;
	}
	return child;
}


static uint64_t SchemaNode_count_get(const SchemaNode* node) {
	uint64_t count = 1;
	for (const SchemaNode* c = node->children.load(std::memory_order_acquire); c; c = c->sibling)
		count += SchemaNode_count_get(c);
	return count;
}


static void* SchemaNode_method_find(const SchemaNode* node, void* dispatcher) {
	// Walk up the ancestors to find the first method push delta for the dispatcher
	for (const SchemaNode* n = node; n; n = n->parent) {
		if (n->delta.type == SchemaDelta::METHOD_PUSH && n->delta.dispatcher == dispatcher)
			return n->delta.method;
	}
	return NULL;
}


static void* SchemaNode_dispatcher_find(const SchemaNode* node, void* method) {
	// Walk up the ancestors to find the first method push delta for the method
	for (const SchemaNode* n = node; n; n = n->parent) {
		if (n->delta.type == SchemaDelta::METHOD_PUSH && n->delta.method == method)
			return n->delta.dispatcher;
	}
	return NULL;
}
