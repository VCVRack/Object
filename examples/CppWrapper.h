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


/** Base class that wraps an Object and its methods.
*/
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


/** Behaves like a const variable but wraps a getter function.
Example:
	Getter<int, value_get> value(object);
	int v = value; // Calls value_get(object);
*/
template <typename T,
	T (*Get)(const Object* self)>
struct Getter {
	Object* self;

	explicit Getter(Object* self) : self(self) {}

	operator T() const {
		return Get(self);
	}
};


/** Behaves like a mutable variable but wraps getter/setter functions.
Example:
	Accessor<int, value_get, value_set> value(object);
	int n = value; // Calls value_get(object);
	value = 42; // Calls value_set(object, 42);
*/
template <typename T,
	T (*Get)(const Object* self),
	void (*Set)(Object* self, T value)>
struct Accessor : Getter<T, Get> {
	explicit Accessor(Object* self) : Getter<T, Get>(self) {}

	Accessor& operator=(const T& value) {
		Set(this->self, value);
		return *this;
	}
};


/** Behaves like a const std::array but wraps an element getter function.
Example:
	ArrayGetter<int, array_size, array_get> array(object);
	array.size(); // Calls array_size(object);
	int n = array[42]; // Calls array_get(object, 42);
*/
template <class T,
	size_t (*Size)(const Object* self),
	T (*Get)(const Object* self, size_t index)>
struct ArrayGetter {
	using value_type = T;
	using size_type = size_t;

	Object* self;

	explicit ArrayGetter(Object* self) : self(self) {}

	T operator[](size_t index) const {
		return Get(self, index);
	}

	size_t size() const {
		return Size(self);
	}
};


/** Behaves like a mutable std::array but wraps element getter/setter functions.
Example:
	ArrayAccessor<int, array_size, array_get, array_set> array(object);
	array.size(); // Calls array_size(object);
	int n = array[42]; // Calls array_get(object, 42);
	array[42] = 1; // Calls array_set(object, 42, 1);
*/
template <class T,
	size_t (*Size)(const Object* self),
	T (*Get)(const Object* self, size_t index),
	void (*Set)(Object* self, size_t index, T value)>
struct ArrayAccessor : ArrayGetter<T, Size, Get> {
	explicit ArrayAccessor(Object* self) : ArrayGetter<T, Size, Get>(self) {}

	struct ElementAccessor {
		Object* self;
		size_t index;

		ElementAccessor(Object* self, size_t index) : self(self), index(index) {}

		operator T() const {
			return Get(self, index);
		}

		ElementAccessor& operator=(const T& value) {
			Set(self, index, value);
			return *this;
		}
	};

	ElementAccessor operator[](size_t index) {
		return ElementAccessor(this->self, index);
	}
};


#endif // __cplusplus
