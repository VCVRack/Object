#pragma once

#include "Object.h"


DECLARE_CLASS(Weak, ());


typedef struct WeakObject WeakObject;


/** Obtains and returns a weak reference to an Object.
A weak reference does not share ownership of the Object, but it can safely track whether it is alive.

The caller *must* release each weak reference with WeakObject_release() after use, or it will never be freed, leaking memory.

Not currently thread-safe.
*/
EXTERNC WeakObject* WeakObject_create(Object* object);

/** Increments ownership of the weak reference.
*/
EXTERNC void WeakObject_obtain(WeakObject* weakObject);

/** Releases ownership of the weak reference.
WeakObject should be considered invalid after calling this function.

Not currently thread-safe.
*/
EXTERNC void WeakObject_release(WeakObject* weakObject);

/** Obtains ownership of the Object and returns it if still alive, or returns NULL if the Object was freed.

If non-NULL, the caller *must* release the Object with `Object_release()` after use, or the Object will never be freed, leaking memory.

Example:
	Object* animal = WeakObject_get(weak_animal);
	if (animal) {
		Animal_speak(animal);
		Object_release(animal);
	}

Incorrect example:
	// This leaks memory because the Object reference is never released after being obtained.
	Animal_speak(WeakObject_get(weak_animal));

TODO: This is not currently thread-safe with Object_release(). If the Object is freed in another thread while this function is called, an invalid Object might be returned. WeakObject should probably be integrated into Object.h.
*/
EXTERNC Object* WeakObject_get(WeakObject* weakObject);

/** Checks if the Object is expired without obtaining a reference.
*/
EXTERNC bool WeakObject_expired_get(WeakObject* weakObject);
