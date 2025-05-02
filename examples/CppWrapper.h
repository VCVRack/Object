#pragma once

#include <Object.h>


struct ObjectWrapper;


/** Wraps a C++ ObjectWrapper subclass instance so users can interact with your Object API using C++ class syntax.
*/
DECLARE_CLASS(CppWrapper, (ObjectWrapper* wrapper));
DECLARE_ACCESSORS_FUNCTION(CppWrapper, wrapper, ObjectWrapper*);


#ifdef __cplusplus

// #include <stdio.h>
#include <assert.h>
#include <type_traits>


template <class T>
T* CppWrapper_cast(Object* self) {
	ObjectWrapper* wrapper = GET(self, CppWrapper, wrapper);
	return dynamic_cast<T*>(wrapper);
}

template <class T>
const T* CppWrapper_cast(const Object* self) {
	const ObjectWrapper* wrapper = GET(self, CppWrapper, wrapper);
	return dynamic_cast<const T*>(wrapper);
}


/** Base class that wraps an Object and its methods.

A library using VCV Object can offer a C++ wrapper API by writing an ObjectWrapper subclass for each Object class and writing proxy virtual/non-virtual methods and accessors.

Once defined, there are two ways to use an ObjectWrapper subclass.

You can proxy an existing Object by calling the constructor `ObjectWrapper(object)`.
No reference is obtained, and the proxy is deleted when the Object is freed.

Or you can further subclass an ObjectWrapper subclass and override its virtual methods.
When instantiated, it creates and owns its Object until deleted.
When the Object's methods are called (such as `Animal_speak()`), your overridden C++ methods are called.

Subclasses can define virtual accessors methods (getters/setters) with names like `getFoo()` or `foo_get()`.
*/
struct ObjectWrapper {
	Object* self;
	bool proxy;

	ObjectWrapper() : ObjectWrapper(Object_create(), false) {}

	ObjectWrapper(Object* self, bool proxy = true) : self(self), proxy(proxy) {
		CppWrapper_specialize(self, this);
	}

	virtual ~ObjectWrapper() {
		// printf("bye ObjectWrapper\n");
		// Remove CppWrapper -> ObjectWrapper association
		SET(self, CppWrapper, wrapper, NULL);
		// Proxy wrappers don't own a reference
		if (!proxy)
			Object_release(self);
	}
};


template <class T>
struct Ref {
	using element_type = T;

	T* wrapper;

	Ref(T* wrapper) : wrapper(wrapper) {
		static_assert(std::is_base_of<ObjectWrapper, T>::value, "Ref can only hold references to ObjectWrapper subclasses");
		obtain();
	}

	~Ref() {
		release();
	}

	T& operator*() const {
		return *wrapper;
	}

	T* operator->() const {
		return wrapper;
	}

	void reset(T* wrapper) {
		release();
		this->wrapper = wrapper;
		obtain();
	}

private:
	void obtain() {
		if (wrapper)
			Object_obtain(wrapper->self);
	}

	void release() {
		if (wrapper)
			Object_release(wrapper->self);
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

		/** Does not check bounds, relies on the getter and setter functions to do so.
		*/
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
