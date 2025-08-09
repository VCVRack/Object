#pragma once

#include "Object.h"
#include "WeakObject.h"


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
Similar to std::shared_ptr.
*/
template <class T>
class Ref {
public:
	static_assert(std::is_base_of<ObjectWrapper, T>::value, "Ref can only hold references to ObjectWrapper subclasses");

	using element_type = T;

	// ObjectWrapper constructor
	explicit Ref(T* wrapper = nullptr) {
		this->wrapper = wrapper;
		obtain();
	}

	// Copy constructor
	Ref(const Ref& other) {
		wrapper = other.wrapper;
		obtain();
	}

	// Move constructor
	Ref(Ref&& other) {
		wrapper = other.wrapper;
		other.wrapper = nullptr;
	}

	~Ref() {
		release();
	}

	// Copy assignment
	Ref& operator=(const Ref& other) {
		reset(other.wrapper);
		return *this;
	}

	// Move assignment
	Ref& operator=(Ref&& other) {
		if (this != &other) {
			release();
			wrapper = other.wrapper;
			other.wrapper = nullptr;
		}
		return *this;
	}

	void reset(T* wrapper = nullptr) {
		if (this->wrapper == wrapper)
			return;
		release();
		this->wrapper = wrapper;
		obtain();
	}

	void swap(Ref& other) {
		T* wrapper = this->wrapper;
		this->wrapper = other.wrapper;
		other.wrapper = wrapper;
	}

	T* get() const { return wrapper; }
	T& operator*() const { return *wrapper; }
	T* operator->() const { return wrapper; }

	size_t use_count() const {
		return wrapper ? Object_refs_get(wrapper->self) : 0;
	}

	explicit operator bool() const { return wrapper; }

	bool operator==(const Ref& other) {
		return wrapper == other.wrapper;
	}

private:
	T* wrapper = NULL;

	void obtain() noexcept {
		if (wrapper)
			Object_obtain(wrapper->self);
	}

	void release() noexcept {
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


/** Weak reference counter for an ObjectWrapper subclass.
Similar to std::weak_ptr.
*/
template <class T>
class WeakRef {
public:
	static_assert(std::is_base_of<ObjectWrapper, T>::value, "WeakRef can only hold references to ObjectWrapper subclasses");

	using element_type = T;

	explicit WeakRef(T* wrapper = nullptr) {
		this->wrapper = wrapper;
		weakObject = wrapper ? WeakObject_create(wrapper->self) : nullptr;
	}

	WeakRef(const Ref<T>& other) {
		wrapper = other.get();
		weakObject = wrapper ? WeakObject_create(wrapper->self) : nullptr;
	}

	WeakRef(const WeakRef& other) {
		wrapper = other.wrapper;
		weakObject = other.weakObject;
		if (weakObject)
			WeakObject_obtain(weakObject);
	}

	WeakRef(WeakRef&& other) {
		wrapper = other.wrapper;
		weakObject = other.weakObject;
		other.wrapper = nullptr;
		other.weakObject = nullptr;
	}

	~WeakRef() {
		reset();
	}

	WeakRef& operator=(const WeakRef& other) {
		if (this != &other) {
			reset();
			wrapper = other.wrapper;
			weakObject = other.weakObject;
			if (weakObject)
				WeakObject_obtain(weakObject);
		}
		return *this;
	}

	WeakRef& operator=(WeakRef&& other) {
		if (this != &other) {
			reset();
			wrapper = other.wrapper;
			weakObject = other.weakObject;
			other.wrapper = nullptr;
			other.weakObject = nullptr;
		}
		return *this;
	}

	Ref<T> lock() const {
		if (!weakObject || !wrapper)
			return Ref<T>();
		Object* object = WeakObject_get(weakObject);
		if (!object)
			return Ref<T>();
		// Cached ObjectWrapper is guaranteed to still be valid.
		Ref<T> ref(wrapper);
		// ref now owns a reference so we don't need this one.
		Object_release(object);
		return ref;
	}

	void reset() {
		if (!weakObject)
			return;
		WeakObject_release(weakObject);
		weakObject = nullptr;
		wrapper = nullptr;
	}

	void swap(WeakRef& other) {
		T* wrapper = this->wrapper;
		WeakObject* weakObject = this->weakObject;
		this->wrapper = other.wrapper;
		this->weakObject = other.weakObject;
		other.wrapper = wrapper;
		other.weakObject = weakObject;
	}

	size_t use_count() const {
		if (!weakObject)
			return 0;
		// Because Object stores its own ref count, a reference must be obtained in order to get it.
		Object* object = WeakObject_get(weakObject);
		if (!object)
			return 0;
		size_t refs = Object_refs_get(object);
		Object_release(object);
		// Don't count the reference we just obtained.
		return refs - 1;
	}

	bool expired() const {
		return weakObject ? WeakObject_expired_get(weakObject) : true;
	}

	bool operator==(const WeakRef& other) {
		return wrapper == other.wrapper;
	}

private:
	T* wrapper = nullptr;
	WeakObject* weakObject = nullptr;
};


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
