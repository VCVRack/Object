#include "Ref.h"


struct Ref {
	uint64_t ref;
};


DEFINE_CLASS(Ref, (), (), {
	Ref* data = (Ref*) calloc(1, sizeof(Ref));
	data->ref = 1;
	PUSH_CLASS(self, Ref, data);
}, {
	free(data);
})


void Ref_obtain(const Object* self) {
	if (!self)
		return;

	// Automatically specialize
	// Cast from const to mutable Object
	Ref_specialize((Object*) self);

	Ref* data = NULL;
	if (!Object_checkClass(self, &Ref_class, (void**) &data))
		return;

	// Increment reference
	data->ref++;
}


void Ref_release(const Object* self) {
	if (!self)
		return;

	Ref* data = NULL;
	if (!Object_checkClass(self, &Ref_class, (void**) &data)) {
		// If Object is not a Ref, treat it as if it has 1 reference.
		// We are releasing its sole reference, so free it.
		Object_free((Object*) self);
		return;
	}

	// Decrement reference and free if that was the last one
	if (--data->ref == 0) {
		Object_free((Object*) self);
	}
}


uint64_t Ref_ref_get(const Object* self) {
	if (!self)
		return 0;

	Ref* data = NULL;
	if (!Object_checkClass(self, &Ref_class, (void**) &data)) {
		return 1;
	}
	return data->ref;
}
