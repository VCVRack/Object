#include <assert.h>
#include "SharedObject.h"


struct SharedObject {
	uint64_t count;
};


DEFINE_CLASS(SharedObject, (), (), {
	SharedObject* data = (SharedObject*) calloc(1, sizeof(SharedObject));
	data->count = 1;
	PUSH_CLASS(self, SharedObject, data);
}, {
	// Assert that Object_free() is called with no references (when SharedObject_release() is called), or with 1 reference (the owner who called Object_free())
	assert(data->count <= 1);
	free(data);
})


void SharedObject_obtain(const Object* self) {
	if (!self)
		return;

	// Automatically specialize
	// Cast from const to mutable Object
	SharedObject_specialize((Object*) self);

	SharedObject* data = NULL;
	if (!Object_checkClass(self, &SharedObject_class, (void**) &data))
		return;

	// Increment reference
	data->count++;
}


void SharedObject_release(const Object* self) {
	if (!self)
		return;

	SharedObject* data = NULL;
	if (!Object_checkClass(self, &SharedObject_class, (void**) &data)) {
		// If not a SharedObject, treat it as if it has 1 reference.
		// We are releasing its sole reference, so free it.
		Object_free((Object*) self);
		return;
	}

	// Decrement reference and free if that was the last one
	if (--data->count == 0) {
		Object_free((Object*) self);
	}
}


uint64_t SharedObject_count_get(const Object* self) {
	if (!self)
		return 0;

	SharedObject* data = NULL;
	if (!Object_checkClass(self, &SharedObject_class, (void**) &data)) {
		return 1;
	}
	return data->count;
}
