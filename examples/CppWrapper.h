#pragma once

#include <Object.h>


struct ObjectWrapper;


/** Wraps a C++ ObjectWrapper subclass instance so users can interact with your Object API using C++ class syntax.
*/
DECLARE_CLASS(CppWrapper, (ObjectWrapper* wrapper));
DECLARE_ACCESSOR_FUNCTION(CppWrapper, wrapper, ObjectWrapper*);


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


/** Reference counter for an ObjectWrapper subclass.
Work-in-progress, untested.
*/
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


/** Defines a method that returns the `self` Object pointer of the "parent" class of a proxy class below.
Effectively performs `this - offsetof(Parent, property)` to find the parent pointer.
This is probably undefined behavior on non-standard-layout types, but it seems to work with virtual classes, virtual inheritance, and multiple inheritance on GCC/Clang x64/arm64.
*/
#define DEFINE_PROXY_SELF_GET(CPPCLASS, PROP) \
	Object* self_get() const { \
		size_t offset = (size_t) (char*) &((CPPCLASS*) NULL)->PROP; \
		CPPCLASS* wrapper = (CPPCLASS*) ((char*) this - offset); \
		return wrapper->self; \
	}


/** Behaves like a const variable but wraps a getter function.
Zero overhead: 0 bytes, compiles to C function calls.
Example:
	GETTER_PROXY(AnimalWrapper, Animal, legs, int);

Usage:
	int legs = animalWrapper->legs; // Calls Animal_legs_get(animal);
*/
#define GETTER_PROXY(CPPCLASS, CLASS, PROP, TYPE) \
	[[no_unique_address]] \
	struct { \
		DEFINE_PROXY_SELF_GET(CPPCLASS, PROP) \
		operator TYPE() const { \
			return CLASS##_##PROP##_get(self_get()); \
		} \
	} PROP


/** Behaves like a mutable variable but wraps getter/setter functions.
Example:
	ACCESSOR_PROXY(AnimalWrapper, Animal, legs, int);

Usage:
	int legs = animalWrapper->legs; // Calls Animal_legs_get(animal);
	animalWrapper->legs = 4; // Calls Animal_legs_set(animal, 3);
*/
#define ACCESSOR_PROXY(CPPCLASS, CLASS, PROP, TYPE) \
	[[no_unique_address]] \
	struct { \
		DEFINE_PROXY_SELF_GET(CPPCLASS, PROP) \
		operator TYPE() const { \
			return CLASS##_##PROP##_get(self_get()); \
		} \
		std::add_const<TYPE>::type& operator=(std::add_const<TYPE>::type& PROP) { \
			CLASS##_##PROP##_set(self_get(), PROP); \
			return PROP; \
		} \
	} PROP


/** Behaves like a const array but wraps an element getter function.
Example:
	ARRAY_GETTER_PROXY(AnimalWrapper, Animal, toes, int);

Usage:
	int toes = animalWrapper->toes[leg]; // Calls Animal_toes_get(animal, leg);
*/
#define ARRAY_GETTER_PROXY(CPPCLASS, CLASS, PROP, TYPE) \
	[[no_unique_address]] \
	struct { \
		DEFINE_PROXY_SELF_GET(CPPCLASS, PROP) \
		TYPE operator[](size_t index) const { \
			return CLASS##_##PROP##_get(self_get(), index); \
		} \
	} PROP


/** Behaves like a mutable array but wraps element getter/setter functions.
Example:
	ARRAY_ACCESSOR_PROXY(AnimalWrapper, Animal, toes, int);

Usage:
	int toes = animalWrapper->toes[leg]; // Calls Animal_toes_get(animal, leg);
	animalWrapper->toes[leg] = 5; // Calls Animal_toes_set(animal, leg, 5);
*/
#define ARRAY_ACCESSOR_PROXY(CPPCLASS, CLASS, PROP, TYPE) \
	[[no_unique_address]] \
	struct { \
		DEFINE_PROXY_SELF_GET(CPPCLASS, PROP) \
		auto operator[](size_t index) { \
			return ElementAccessor<TYPE, CLASS##_##PROP##_get, CLASS##_##PROP##_set>(self_get(), index); \
		} \
	} PROP


template <class T,
	T (*Get)(const Object* self, size_t index),
	void (*Set)(Object* self, size_t index, T value)>
struct ElementAccessor {
	Object* self;
	size_t index;

	ElementAccessor(Object* self, size_t index) : self(self), index(index) {}

	/** Does not check bounds, relies on the C getter and setter functions to do so.
	*/
	operator T() const {
		return Get(self, index);
	}

	const T& operator=(const T& value) {
		Set(self, index, value);
		return value;
	}
};


// TODO: Make the above class subclass a template function that implements all methods that don't need template magic.


#endif // __cplusplus
