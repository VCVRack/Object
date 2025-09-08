#pragma once

#include "Object.h"
#include "WeakObject.h"


typedef struct ObjectWrapper ObjectWrapper;


/** Wraps a C++ ObjectWrapper subclass instance so users can interact with your Object API using C++ class syntax.
*/
DECLARE_CLASS(CppWrapper, ());
DECLARE_ACCESSOR(CppWrapper, wrapper, ObjectWrapper*);
typedef void CppWrapper_destructor_f(ObjectWrapper* wrapper);
DECLARE_ACCESSOR(CppWrapper, destructor, CppWrapper_destructor_f*);


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
struct ObjectWrapper {
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

	ObjectWrapper(const ObjectWrapper&) = delete;
	ObjectWrapper& operator=(const ObjectWrapper&) = delete;

	virtual ~ObjectWrapper() {
		// printf("bye ObjectWrapper\n");
		// Remove CppWrapper -> ObjectWrapper association
		CppWrapper_wrapper_set(self, NULL);
		// Proxy wrappers don't own a reference
		if (original)
			Object_release(self);
	}

	Object* self_get() {
		return self;
	}

	static Object* self_get(ObjectWrapper* wrapper) {
		if (!wrapper)
			return NULL;
		return wrapper->self;
	}
	static const Object* self_get(const ObjectWrapper* wrapper) {
		if (!wrapper)
			return NULL;
		return wrapper->self;
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

template <class T>
T* CppWrapper_castOrCreate(Object* self) {
	if (!self)
		return NULL;
	ObjectWrapper* wrapper = GET(self, CppWrapper, wrapper);
	if (wrapper) {
		// Can be null if wrapper exists but invalid type.
		return dynamic_cast<T*>(wrapper);
	}
	T* t = new T(self);
	SET(self, CppWrapper, wrapper, t);
	return t;
}




/** Reference counter for an ObjectWrapper subclass.
Similar to std::shared_ptr.
*/
template <class T>
struct Ref {
	static_assert(std::is_base_of<ObjectWrapper, T>::value, "Ref can only hold references to ObjectWrapper subclasses");

	using element_type = T;

	Ref() {}

	// ObjectWrapper constructor
	Ref(T* wrapper) {
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


/** Weak reference counter for an ObjectWrapper subclass.
Similar to std::weak_ptr.
*/
template <class T>
struct WeakRef {
	static_assert(std::is_base_of<ObjectWrapper, T>::value, "WeakRef can only hold references to ObjectWrapper subclasses");

	using element_type = T;

	WeakRef() {}

	WeakRef(T* wrapper) {
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


template <typename Base>
struct Proxy : Base {
	Proxy() = default;
	Proxy(const Proxy&) = delete;
	Proxy& operator=(const Proxy&) = delete;
};


template <typename Base>
struct GetterProxy : Proxy<Base> {
	using Base::get;
	using T = decltype(std::declval<GetterProxy>().get());
	operator T() const { return get(); }
	T operator->() const { return (T) *this; }
	T operator+() const { return +(T) *this; }
	T operator-() const { return -(T) *this; }
};


template <typename Base>
struct AccessorProxy : GetterProxy<Base> {
	using Base::set;
	using T = typename GetterProxy<Base>::T;
	AccessorProxy& operator=(const T& t) { set(t); return *this; }
	AccessorProxy& operator=(const AccessorProxy& other) { return *this = (T) other; }
	/** The C++ compiler only allows one conversion, so proxy-to-proxy assignment needs an explicit conversion to T. */
	template <typename OtherBase>
	AccessorProxy& operator=(const GetterProxy<OtherBase>& other) { return *this = (T) other; }
	AccessorProxy& operator+=(const T& t) { return *this = (T) *this + t; }
	AccessorProxy& operator-=(const T& t) { return *this = (T) *this - t; }
	AccessorProxy& operator*=(const T& t) { return *this = (T) *this * t; }
	AccessorProxy& operator/=(const T& t) { return *this = (T) *this / t; }
	AccessorProxy& operator++() { return *this += 1; }
	T operator++(int) { T tmp = *this; *this = tmp + 1; return tmp; }
	AccessorProxy& operator--() { return *this -= 1; }
	T operator--(int) { T tmp = *this; *this = tmp - 1; return tmp; }
};


/** Given a pointer to a struct's member, gets the address of the struct itself.

Effectively performs `this - offsetof(Parent, property)` to find the parent pointer.
This is undefined behavior on non-standard-layout types, but it works with virtual classes, virtual inheritance, and multiple inheritance on GCC/Clang x64/arm64.
*/
#define CONTAINER_OF(PTR, STRUCT, MEMBER) \
	((STRUCT*) ((char*) (PTR) - (char*) &((STRUCT*) 0)->MEMBER))


/** Defines self_get() and makes proxy non-copyable.
*/
#define PROXY_METHODS(CPPCLASS, PROP) \
	Object* self_get() const { \
		return CONTAINER_OF(this, CPPCLASS, PROP)->self_get();	\
	}


#define PROXY_CUSTOM(CPPCLASS, PROP, METHODS) \
	struct Proxy_##PROP { \
		PROXY_METHODS(CPPCLASS, PROP) \
		METHODS \
	}; \
	[[no_unique_address]] \
	Proxy_##PROP PROP


#define GETTER_PROXY_METHODS(TYPE, GETTER) \
	TYPE get() const { \
		Object* self = self_get(); \
		GETTER \
	}


/** Defines a 0-byte C++ proxy class with custom methods.
*/
#define GETTER_PROXY_CUSTOM(CPPCLASS, PROP, TYPE, GETTER) \
	struct Proxy_##PROP { \
		PROXY_METHODS(CPPCLASS, PROP) \
		GETTER_PROXY_METHODS(TYPE, GETTER) \
	}; \
	[[no_unique_address]] \
	GetterProxy<Proxy_##PROP> PROP


/** Behaves like a const variable but wraps a getter function.
Zero overhead: 0 bytes, compiles to C function calls.
Example:
	GETTER_PROXY(AnimalWrapper, Animal, legs, int);

Usage:
	int legs = animalWrapper->legs; // Calls Animal_legs_get(animal);
*/
#define GETTER_PROXY(CPPCLASS, CLASS, PROP, TYPE) \
	GETTER_PROXY_CUSTOM(CPPCLASS, PROP, TYPE, { \
		return CLASS##_##PROP##_get(self); \
	})


#define ACCESSOR_PROXY_METHODS(PROP, TYPE, GETTER, SETTER) \
	GETTER_PROXY_METHODS(TYPE, GETTER) \
	void set(TYPE PROP) { \
		Object* self = self_get(); \
		SETTER \
	}


#define ACCESSOR_PROXY_CUSTOM(CPPCLASS, PROP, TYPE, GETTER, SETTER) \
	struct Proxy_##PROP { \
		PROXY_METHODS(CPPCLASS, PROP) \
		ACCESSOR_PROXY_METHODS(PROP, TYPE, GETTER, SETTER) \
	}; \
	[[no_unique_address]] \
	AccessorProxy<Proxy_##PROP> PROP


/** Behaves like a mutable variable but wraps getter/setter functions.
Example:
	ACCESSOR_PROXY(AnimalWrapper, Animal, legs, int);

Usage:
	int legs = animalWrapper->legs; // Calls Animal_legs_get(animal);
	animalWrapper->legs = 4; // Calls Animal_legs_set(animal, 3);
*/
#define ACCESSOR_PROXY(CPPCLASS, CLASS, PROP, TYPE) \
	ACCESSOR_PROXY_CUSTOM(CPPCLASS, PROP, TYPE, { \
		return CLASS##_##PROP##_get(self); \
	}, { \
		CLASS##_##PROP##_set(self, PROP); \
	})


#define ARRAY_GETTER_PROXY_METHODS(TYPE, CODE) \
	TYPE get(size_t index) const { \
		Object* self = self_get(); \
		CODE \
	} \
	TYPE operator[](size_t index) const { \
		return get(index); \
	}


#define ARRAY_GETTER_PROXY_CUSTOM(CPPCLASS, PROP, TYPE, GETTER) \
	PROXY_CUSTOM(CPPCLASS, PROP, \
		ARRAY_GETTER_PROXY_METHODS(TYPE, GETTER) \
	)


/** Behaves like a const array but wraps an element getter function.
Example:
	ARRAY_GETTER_PROXY(AnimalWrapper, Animal, toes, int);

Usage:
	int toes = animalWrapper->toes[leg]; // Calls Animal_toes_get(animal, leg);
*/
#define ARRAY_GETTER_PROXY(CPPCLASS, CLASS, PROP, TYPE) \
	ARRAY_GETTER_PROXY_CUSTOM(CPPCLASS, PROP, TYPE, { \
		return CLASS##_##PROP##_get(self, index); \
	})


#define ARRAY_ACCESSOR_PROXY_METHODS(PROP, TYPE, GETTER, SETTER) \
	struct ElementAccessorProxy { \
		Object* self; \
		size_t index; \
		ElementAccessorProxy(Object* self, size_t index) : self(self), index(index) {} \
		TYPE get() const { \
			GETTER \
		} \
		operator TYPE() const { \
			return get(); \
		} \
		TYPE operator->() const { \
			return get(); \
		} \
		void set(TYPE PROP) { \
			SETTER \
		} \
		TYPE operator=(TYPE PROP) { \
			set(PROP); \
			return PROP; \
		} \
	}; \
	ElementAccessorProxy operator[](size_t index) { \
		Object* self = self_get(); \
		return ElementAccessorProxy(self, index); \
	}


#define ARRAY_ACCESSOR_PROXY_CUSTOM(CPPCLASS, PROP, TYPE, GETTER, SETTER) \
	PROXY_CUSTOM(CPPCLASS, PROP, \
		ARRAY_ACCESSOR_PROXY_METHODS(PROP, TYPE, GETTER, SETTER) \
	)


/** Behaves like a mutable array but wraps element getter/setter functions.
Example:
	ARRAY_ACCESSOR_PROXY(AnimalWrapper, Animal, toes, int);

Usage:
	int toes = animalWrapper->toes[leg]; // Calls Animal_toes_get(animal, leg);
	animalWrapper->toes[leg] = 5; // Calls Animal_toes_set(animal, leg, 5);
*/
#define ARRAY_ACCESSOR_PROXY(CPPCLASS, CLASS, PROP, TYPE) \
ARRAY_ACCESSOR_PROXY_CUSTOM(CPPCLASS, PROP, TYPE, { \
		return CLASS##_##PROP##_get(self, index); \
	}, { \
		CLASS##_##PROP##_set(self, index, PROP); \
	})


#endif // __cplusplus
