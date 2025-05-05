#pragma once

#include <Object.h>


typedef struct WeakRef WeakRef;

/** Obtains and returns a weak reference to an Object.
A weak reference does not share ownership of the Object, but it can safely track whether it is alive.

The caller *must* release each weak reference with WeakRef_release() after use, or it will never be freed, leaking memory.

WeakRef is not currently thread-safe.
*/
EXTERNC WeakRef* WeakRef_obtain(Object* object);

/** Releases ownership of the weak reference.
WeakRef should be considered invalid after calling this function.
*/
EXTERNC void WeakRef_release(WeakRef* weakRef);

/** Obtains ownership of the Object and returns it if still alive, or returns NULL if the Object was freed.

If non-NULL, the caller *must* release the Object with `Object_release()` after use, or the Object will never be freed, leaking memory.

Example:
	Object* animal = WeakRef_get(weak_animal);
	if (animal) {
		Animal_speak(animal);
		Object_release(animal);
	}

Incorrect example:
	// This leaks memory because the Object reference is never released after being obtained.
	Animal_speak(WeakRef_get(weak_animal));
*/
EXTERNC Object* WeakRef_get(WeakRef* weakRef);
