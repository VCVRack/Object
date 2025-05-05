#include <atomic>
#include <assert.h>
#include "WeakRef.h"


struct WeakRef {
	Object* object;
	std::atomic<size_t> refs;
};


DEFINE_CLASS(Weak, (), (), {
	WeakRef* weakRef = new WeakRef;
	weakRef->object = self;
	weakRef->refs = 0;
	PUSH_CLASS(self, Weak, weakRef);
}, {
	WeakRef* weakRef = (WeakRef*) data;
	weakRef->object = NULL;
	// Don't delete WeakRef if there are still references to it
	if (weakRef->refs)
		return;
	delete weakRef;
})


WeakRef* WeakRef_obtain(Object* object) {
	if (!object)
		return NULL;
	// Ensure that Object has Weak class
	Weak_specialize(object);
	WeakRef* weakRef = NULL;
	Object_class_check(object, &Weak_class, (void**) &weakRef);
	assert(weakRef);
	// Increment weak reference count
	++weakRef->refs;
	return weakRef;
}


void WeakRef_release(WeakRef* weakRef) {
	if (!weakRef)
		return;
	// Decrement weak reference count
	// Don't delete if there are still references
	if (--weakRef->refs)
		return;
	// Don't delete if Object is still alive. There are no remaining references, but a call to WeakRef_obtain() will reuse this WeakRef.
	if (weakRef->object)
		return;
	delete weakRef;
}


Object* WeakRef_get(WeakRef* weakRef) {
	if (!weakRef)
		return NULL;
	Object* object = weakRef->object;
	// Object_obtain(NULL) is safe, but this avoids a potentially unnecessary call.
	if (!object)
		return NULL;
	// Automatically obtain reference
	Object_obtain(object);
	return object;
}


