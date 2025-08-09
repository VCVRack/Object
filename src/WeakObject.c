#include <stdatomic.h>
#include <assert.h>
#include <Object/WeakObject.h>


struct WeakObject {
	Object* object;
	atomic_size_t refs;
};


DEFINE_CLASS(Weak, (), (), {
	WeakObject* weakObject = malloc(sizeof(WeakObject));
	weakObject->object = self;
	atomic_init(&weakObject->refs, 0);
	PUSH_CLASS(self, Weak, weakObject);
}, {
	WeakObject* weakObject = (WeakObject*) data;
	weakObject->object = NULL;
	// Don't delete WeakObject if there are still references to it
	if (atomic_load_explicit(&weakObject->refs, memory_order_acquire) != 0)
		return;
	free(weakObject);
})


WeakObject* WeakObject_create(Object* object) {
	if (!object)
		return NULL;
	// Ensure that Object has Weak class
	Weak_specialize(object);
	WeakObject* weakObject = NULL;
	Object_class_check(object, &Weak_class, (void**) &weakObject);
	assert(weakObject);
	WeakObject_obtain(weakObject);
	return weakObject;
}


void WeakObject_obtain(WeakObject* weakObject) {
	if (!weakObject)
		return;
	// Increment weak reference count
	atomic_fetch_add_explicit(&weakObject->refs, 1, memory_order_acq_rel);
}


void WeakObject_release(WeakObject* weakObject) {
	if (!weakObject)
		return;
	// Decrement weak reference count
	// Don't delete if there are still references
	if (atomic_fetch_sub_explicit(&weakObject->refs, 1, memory_order_acq_rel) - 1 != 0)
		return;
	// Don't delete if Object is still alive. There are no remaining references, but a call to WeakObject_obtain() will reuse this WeakObject.
	if (weakObject->object)
		return;
	free(weakObject);
}


Object* WeakObject_get(WeakObject* weakObject) {
	if (!weakObject)
		return NULL;
	Object* object = weakObject->object;
	// Object_obtain(NULL) is safe, but this avoids a potentially unnecessary call.
	if (!object)
		return NULL;
	// Automatically obtain reference
	Object_obtain(object);
	return object;
}


bool WeakObject_expired_get(WeakObject* weakObject) {
	if (!weakObject)
		return true;
	return !weakObject->object;
}
