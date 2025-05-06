#pragma once

#include <Object.h>


typedef struct ObjectWrapper ObjectWrapper;


/** Wraps a C++ ObjectWrapper subclass instance so users can interact with your Object API using C++ class syntax.
*/
DECLARE_CLASS(CppWrapper, ());
DECLARE_ACCESSOR_FUNCTION(CppWrapper, wrapper, ObjectWrapper*);
typedef void CppWrapper_destructor_f(ObjectWrapper* wrapper);
DECLARE_ACCESSOR_FUNCTION(CppWrapper, destructor, CppWrapper_destructor_f*);


#ifdef __cplusplus

// #include <stdio.h>
#include <assert.h>
#include <type_traits>


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
class ObjectWrapper {
public:
	Object* self;
	bool original;

	ObjectWrapper() : ObjectWrapper(Object_create(), true) {}

	ObjectWrapper(Object* self, bool original = false) : self(self), original(original) {
		// TODO Assert that existing wrapper does not already exist
		CppWrapper_specialize(self);
		CppWrapper_wrapper_set(self, this);
		CppWrapper_destructor_set(self, [](ObjectWrapper* wrapper) {
			delete wrapper;
		});
	}

	virtual ~ObjectWrapper() {
		// printf("bye ObjectWrapper\n");
		// Remove CppWrapper -> ObjectWrapper association
		CppWrapper_wrapper_set(self, NULL);
		// Proxy wrappers don't own a reference
		if (original)
			Object_release(self);
	}
};


/** Reference counter for an ObjectWrapper subclass.
Work-in-progress, untested.
*/
template <class T>
class Ref {
public:
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


/** Defines a struct that can calculate the `self` Object pointer of the "parent" class of a proxy class below.
Effectively performs `this - offsetof(Parent, property)` to find the parent pointer.
This is probably undefined behavior on non-standard-layout types, but it seems to work with virtual classes, virtual inheritance, and multiple inheritance on GCC/Clang x64/arm64.
*/
#define PROXY_CLASS(CPPCLASS, PROP) \
	struct Proxy_##PROP { \
		Object* self_get() const { \
			size_t offset = (size_t) (char*) &((CPPCLASS*) NULL)->PROP; \
			CPPCLASS* wrapper = (CPPCLASS*) ((char*) this - offset); \
			return wrapper->self; \
		} \
	}


template <class Base,
	typename Type,
	Type (*Get)(const Object* self)>
struct GetterProxy : Base {
	operator Type() const {
		return Get(Base::self_get());
	}
};


/** Behaves like a const variable but wraps a getter function.
Zero overhead: 0 bytes, compiles to C function calls.
Example:
	GETTER_PROXY(AnimalWrapper, Animal, legs, int);

Usage:
	int legs = animalWrapper->legs; // Calls Animal_legs_get(animal);
*/
#define GETTER_PROXY(CPPCLASS, CLASS, PROP, TYPE) \
	PROXY_CLASS(CPPCLASS, PROP); \
	[[no_unique_address]] \
	GetterProxy<Proxy_##PROP, TYPE, CLASS##_##PROP##_get> PROP


template <class Base,
	typename Type,
	Type (*Get)(const Object* self),
	void (*Set)(Object* self, Type value)>
struct AccessorProxy : GetterProxy<Base, Type, Get> {
	Type operator=(const Type& value) {
		Set(Base::self_get(), value);
		return value;
	}
};


/** Behaves like a mutable variable but wraps getter/setter functions.
Example:
	ACCESSOR_PROXY(AnimalWrapper, Animal, legs, int);

Usage:
	int legs = animalWrapper->legs; // Calls Animal_legs_get(animal);
	animalWrapper->legs = 4; // Calls Animal_legs_set(animal, 3);
*/
#define ACCESSOR_PROXY(CPPCLASS, CLASS, PROP, TYPE) \
	PROXY_CLASS(CPPCLASS, PROP); \
	[[no_unique_address]] \
	AccessorProxy<Proxy_##PROP, TYPE, CLASS##_##PROP##_get, CLASS##_##PROP##_set> PROP


template <class Base,
	typename Type,
	Type (*Get)(const Object* self, size_t index)>
struct ArrayGetterProxy : Base {
	Type operator[](size_t index) const {
		return Get(Base::self_get(), index);
	}
};


/** Behaves like a const array but wraps an element getter function.
Example:
	ARRAY_GETTER_PROXY(AnimalWrapper, Animal, toes, int);

Usage:
	int toes = animalWrapper->toes[leg]; // Calls Animal_toes_get(animal, leg);
*/
#define ARRAY_GETTER_PROXY(CPPCLASS, CLASS, PROP, TYPE) \
	PROXY_CLASS(CPPCLASS, PROP); \
	[[no_unique_address]] \
	ArrayGetterProxy<Proxy_##PROP, TYPE, CLASS##_##PROP##_get> PROP


template <class Base,
	typename Type,
	Type (*Get)(const Object* self, size_t index),
	void (*Set)(Object* self, size_t index, Type value)>
struct ArrayAccessorProxy : Base {
	struct ElementAccessorProxy {
		Object* self;
		size_t index;

		ElementAccessorProxy(Object* self, size_t index) : self(self), index(index) {}

		/** Does not check bounds, relies on the C getter and setter functions to do so.
		*/
		operator Type() const {
			return Get(self, index);
		}

		Type operator=(const Type& value) {
			Set(self, index, value);
			return value;
		}
	};

	ElementAccessorProxy operator[](size_t index) {
		return ElementAccessorProxy(Base::self_get(), index);
	}
};


/** Behaves like a mutable array but wraps element getter/setter functions.
Example:
	ARRAY_ACCESSOR_PROXY(AnimalWrapper, Animal, toes, int);

Usage:
	int toes = animalWrapper->toes[leg]; // Calls Animal_toes_get(animal, leg);
	animalWrapper->toes[leg] = 5; // Calls Animal_toes_set(animal, leg, 5);
*/
#define ARRAY_ACCESSOR_PROXY(CPPCLASS, CLASS, PROP, TYPE) \
	PROXY_CLASS(CPPCLASS, PROP); \
	[[no_unique_address]] \
	ArrayAccessorProxy<Proxy_##PROP, TYPE, CLASS##_##PROP##_get, CLASS##_##PROP##_set> PROP


#endif // __cplusplus
