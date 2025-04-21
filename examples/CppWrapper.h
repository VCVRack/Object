#pragma once

#include <Object.h>


/** Wraps a C++ class so users can interact with your Object API using C++ class objects.
*/
DECLARE_CLASS(CppWrapper, (void* ptr));
DECLARE_FUNCTION_CONST(CppWrapper, ptr_get, void*, ());


#ifdef __cplusplus


/** Convenience functions for converting an Object to a particular C++ class type. */
template <class T>
T* CppWrapper_getClass(Object* self) {
	return static_cast<T*>(CppWrapper_ptr_get(self));
}

template <class T>
const T* CppWrapper_getClass(const Object* self) {
	return static_cast<const T*>(CppWrapper_ptr_get(self));
}


struct ObjectWrapper {
	Object* self;
	ObjectWrapper(Object* self) : self(self) {
		CppWrapper_specialize(self, this);
	}
	virtual ~ObjectWrapper() {
		if (self) {
			Object* oldSelf = self;
			// Request to not delete `this` since we are already being deleted.
			self = NULL;
			Object_free(oldSelf);
		}
	}
};


template <typename T,
	T (*Get)(const Object*),
	void (*Set)(Object*, T)>
struct Property {
	Object* self;

	explicit Property(Object* self) : self(self) {}

	Property& operator=(const T& value) {
		Set(self, value);
		return *this;
	}

	operator T() const {
		return Get(self);
	}
};


// TODO: Getter and Setter classes


#endif // __cplusplus
