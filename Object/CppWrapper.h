#pragma once

#include "Object.h"
#include "WeakObject.h"


typedef struct ObjectWrapper ObjectWrapper;


/** Wraps a C++ ObjectWrapper subclass instance so users can interact with your Object API using C++ class syntax.
*/
CLASS(CppWrapper, ());
ACCESSOR(CppWrapper, wrapper, ObjectWrapper*);
typedef void CppWrapper_destructor_f(ObjectWrapper* wrapper);
ACCESSOR(CppWrapper, destructor, CppWrapper_destructor_f*);


#ifdef __cplusplus

#include <assert.h>
#include <vector>
#include <type_traits>
#include <iterator>


/** Base class that wraps an Object and its methods.

A library using VCV Object can offer a C++ wrapper API by writing an ObjectWrapper subclass for each Object class and writing proxy virtual/non-virtual methods and accessors.

Once defined, there are two ways to use an ObjectWrapper subclass.

You can proxy an existing Object by calling the constructor `ObjectWrapper(object)`.
No reference is obtained, and the proxy is deleted when the Object is freed.

Or you can further subclass an ObjectWrapper subclass and override its virtual methods.
When instantiated, it creates and owns its Object until deleted.
When the Object's methods are called (such as `Animal_speak()`), your overridden C++ methods are called.

See Animal.hpp for an example C++ class that wraps an Animal object.
*/
struct ObjectWrapper {
	/** Owned if `original` is true. */
	Object* self;
	bool original;

	/** Constructs an ObjectWrapper with a new Object.
	Each ObjectWrapper subclass should implement its own default constructor that creates a specialized Object.
	*/
	ObjectWrapper() : ObjectWrapper(Object_create(), true) {}

	/** Constructs an ObjectWrapper with an existing Object.
	If `original` is true, use BIND_* macros to override the Object's virtual methods with C++ virtual methods.
	*/
	ObjectWrapper(Object* self, bool original = false) : self(self), original(original) {
		// TODO Assert that existing wrapper does not already exist
		CppWrapper_specialize(self);
		CppWrapper_wrapper_set(self, this);
		CppWrapper_destructor_set(self, [](ObjectWrapper* wrapper) {
			delete wrapper;
		});
	}

	/** Objects can't be copied by default, so disable copying ObjectWrapper. */
	ObjectWrapper(const ObjectWrapper&) = delete;
	ObjectWrapper& operator=(const ObjectWrapper&) = delete;

	virtual ~ObjectWrapper() {
		// Remove CppWrapper -> ObjectWrapper association
		CppWrapper_wrapper_set(self, NULL);
		// Proxy wrappers don't own a reference
		if (original)
			Object_release(self);
	}

	/** Gets the Object, preserving constness. */
	Object* self_get() { return self; }
	const Object* self_get() const { return self; }

	/** Gets an ObjectWrapper's Object, gracefully handling NULL. */
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


/** Overrides an Object's method with a lambda/thunk that calls your ObjectWrapper subclass' method.
Provides a `that` variable that points to your ObjectWrapper.

Example:
	BIND_METHOD(Animal, Animal, speak, (), {
		that->speak();
	});
*/
#define BIND_METHOD(CPPCLASS, CLASS, METHOD, ARGTYPES, CODE) \
	Object_method_push(self, (void*) &CLASS##_##METHOD, (void*) +[](Object* self COMMA_EXPAND ARGTYPES) { \
		CPPCLASS* that = CppWrapper_cast<CPPCLASS>(self); \
		assert(that); \
		CODE \
	})

#define BIND_METHOD_CONST(CPPCLASS, CLASS, METHOD, ARGTYPES, CODE) \
	Object_method_push(self, (void*) &CLASS##_##METHOD, (void*) +[](const Object* self COMMA_EXPAND ARGTYPES) { \
		const CPPCLASS* that = CppWrapper_cast<CPPCLASS>(self); \
		assert(that); \
		CODE \
	})

#define BIND_GETTER(CPPCLASS, CLASS, PROP, CODE) \
	BIND_METHOD_CONST(CPPCLASS, CLASS, PROP##_get, (), CODE)

#define BIND_SETTER(CPPCLASS, CLASS, PROP, TYPE, CODE) \
	BIND_METHOD(CPPCLASS, CLASS, PROP##_set, (TYPE PROP), CODE)

#define BIND_ACCESSOR(CPPCLASS, CLASS, PROP, TYPE, GETTER, SETTER) \
	BIND_GETTER(CPPCLASS, CLASS, PROP, GETTER); \
	BIND_SETTER(CPPCLASS, CLASS, PROP, TYPE, SETTER)


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


/** Given a pointer to a struct's member, gets the address of the struct itself.

Effectively performs `this - offsetof(Parent, property)` to find the parent pointer.
This is undefined behavior on non-standard-layout types, but it works with virtual classes, virtual inheritance, and multiple inheritance on GCC/Clang x64/arm64.

This article explains it well.
https://radek.io/posts/magical-container_of-macro/
*/
#define CONTAINER_OF(PTR, STRUCT, MEMBER) \
	((STRUCT*) ((char*) (PTR) - (char*) &((STRUCT*) 0)->MEMBER))


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


template <typename Base>
struct GetterProxy : Proxy<Base> {
	using T = decltype(std::declval<Base>().get());

	operator T() const { return Base::get(); }
	T operator->() const { return (T) *this; }
	T operator+() const { return +(T) *this; }
	T operator-() const { return -(T) *this; }
};


#define GETTER_PROXY_METHODS(TYPE, GETTER) \
	TYPE get() const { \
		Object* self = self_get(); \
		(void) self; \
		GETTER \
	}


#define GETTER_PROXY_CUSTOM(CPPCLASS, PROP, TYPE, GETTER) \
	struct Proxy_##PROP { \
		PROXY_METHODS(CPPCLASS, PROP) \
		GETTER_PROXY_METHODS(TYPE, GETTER) \
	}; \
	[[no_unique_address]] \
	const GetterProxy<Proxy_##PROP> PROP


/** Behaves like a const variable but wraps a getter function.
Zero overhead: 0 bytes, compiles to C function calls.
Example:
	GETTER_PROXY(AnimalWrapper, Animal, legs, int);

Usage:
	int legs = animalWrapper->legs; // Calls Animal_legs_get(animal);
*/
#define GETTER_PROXY(CPPCLASS, CLASS, PROP, TYPE) \
	GETTER_PROXY_CUSTOM(CPPCLASS, PROP, TYPE, { \
		return GET(self, CLASS, PROP); \
	})


template <typename Base>
struct AccessorProxy : GetterProxy<Base> {
	using T = typename GetterProxy<Base>::T;

	AccessorProxy& operator=(const T& t) { Base::set(t); return *this; }
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


#define ACCESSOR_PROXY_METHODS(PROP, TYPE, GETTER, SETTER) \
	GETTER_PROXY_METHODS(TYPE, GETTER) \
	void set(TYPE PROP) { \
		Object* self = self_get(); \
		(void) self; \
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
		return GET(self, CLASS, PROP); \
	}, { \
		SET(self, CLASS, PROP, PROP); \
	})


template <typename Proxy>
struct ArrayGetterProxyIterator {
	Proxy& proxy;
	size_t i;

	using iterator_category = std::random_access_iterator_tag;
	using value_type = typename Proxy::value_type;
	using difference_type = std::ptrdiff_t;
	using reference = value_type;
	using pointer = value_type;

	ArrayGetterProxyIterator(Proxy& proxy, size_t i) : proxy(proxy), i(i) {}

	reference operator*() const { return proxy[i]; }
	pointer operator->() const { return &proxy[i]; }

	ArrayGetterProxyIterator& operator++() { ++i; return *this; }
	ArrayGetterProxyIterator operator++(int) { ArrayGetterProxyIterator tmp = *this; ++i; return tmp; }
	ArrayGetterProxyIterator& operator--() { --i; return *this; }
	ArrayGetterProxyIterator operator--(int) { ArrayGetterProxyIterator tmp = *this; --i; return tmp; }

	ArrayGetterProxyIterator& operator+=(difference_type n) { i += n; return *this; }
	ArrayGetterProxyIterator& operator-=(difference_type n) { i -= n; return *this; }
	ArrayGetterProxyIterator operator+(difference_type n) const { return ArrayGetterProxyIterator(proxy, i + n); }
	ArrayGetterProxyIterator operator-(difference_type n) const { return ArrayGetterProxyIterator(proxy, i - n); }
	difference_type operator-(const ArrayGetterProxyIterator& other) const { return i - other.i; }

	value_type operator[](difference_type n) const { return proxy[i + n]; }

	bool operator==(const ArrayGetterProxyIterator& other) const { return i == other.i; }
	bool operator!=(const ArrayGetterProxyIterator& other) const { return !(*this == other); }
	bool operator<(const ArrayGetterProxyIterator& other) const { return i < other.i; }
	bool operator>(const ArrayGetterProxyIterator& other) const { return i > other.i; }
	bool operator<=(const ArrayGetterProxyIterator& other) const { return i <= other.i; }
	bool operator>=(const ArrayGetterProxyIterator& other) const { return i >= other.i; }
};


template <typename Base>
struct ArrayGetterProxy : Proxy<Base> {
	using value_type = decltype(std::declval<Base>().get(0));
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;

	size_t size() const { return Base::count_get(); }
	bool empty() const { return size() == 0; }
	value_type operator[](size_t index) const { return (value_type) Base::get(index); }
	value_type front() const { return (*this)[0]; }
	value_type back() const { return (*this)[size() - 1]; }

	/** Implicit cast to vector */
	operator std::vector<value_type>() const {
		size_t count = size();
		std::vector<value_type> v;
		v.resize(count);
		for (size_t i = 0; i < count; i++) {
			v[i] = (*this)[i];
		}
		return v;
	}

	// Iterators

	using iterator = ArrayGetterProxyIterator<const ArrayGetterProxy>;
	using const_iterator = iterator;

	iterator begin() { return iterator(*this, 0); }
	const_iterator begin() const { return const_iterator(*this, 0); }

	iterator end() { return iterator(*this, size()); }
	const_iterator end() const { return const_iterator(*this, size()); }

	std::reverse_iterator<iterator> rbegin() { return std::reverse_iterator<iterator>(end()); }
	std::reverse_iterator<const_iterator> rbegin() const { return std::reverse_iterator<const_iterator>(end()); }

	std::reverse_iterator<iterator> rend() { return std::reverse_iterator<iterator>(begin()); }
	std::reverse_iterator<const_iterator> rend() const { return std::reverse_iterator<const_iterator>(begin()); }
};


#define ARRAY_GETTER_PROXY_METHODS(TYPE, COUNT, GETTER) \
	size_t count_get() const { \
		Object* self = self_get(); \
		(void) self; \
		COUNT \
	} \
	TYPE get(size_t index) const { \
		Object* self = self_get(); \
		(void) self; \
		GETTER \
	}


#define ARRAY_GETTER_PROXY_CUSTOM(CPPCLASS, PROPS, TYPE, COUNT, GETTER) \
	struct Proxy_##PROPS { \
		PROXY_METHODS(CPPCLASS, PROPS) \
		ARRAY_GETTER_PROXY_METHODS(TYPE, COUNT, GETTER) \
	}; \
	[[no_unique_address]] \
	const ArrayGetterProxy<Proxy_##PROPS> PROPS


/** Behaves like a const array but wraps an element getter function.
PROPS should be plural since C++ arrays are typically plural.

Example:
	ARRAY_GETTER_PROXY(AnimalWrapper, Animal, child, children, Object*);

Usage:
	size_t childrenSize = animalWrapper->children.size(); // Calls Animal_child_count_get(animal);
	Object* child = animalWrapper->children[index]; // Calls Animal_child_get(animal, index);
*/
#define ARRAY_GETTER_PROXY(CPPCLASS, CLASS, PROP, PROPS, TYPE) \
	ARRAY_GETTER_PROXY_CUSTOM(CPPCLASS, PROPS, TYPE, { \
		return GET(self, CLASS, PROP##_count); \
	}, { \
		return GET(self, CLASS, PROP, index); \
	})


template <typename Proxy>
struct ArrayAccessorProxyIterator : ArrayGetterProxyIterator<Proxy> {
	using Base = ArrayGetterProxyIterator<Proxy>;
	using value_type = typename Base::value_type;
	using difference_type = typename Base::difference_type;
	using reference = value_type;
	using pointer = value_type*;
	using Base::Base;

	reference operator*() const { return Base::proxy[Base::i]; }
	pointer operator->() const { return &Base::proxy[Base::i]; }
	reference operator[](difference_type n) const { return Base::proxy[Base::i + n]; }
};


template <typename Base>
struct ArrayAccessorProxy : Proxy<Base> {
	using T = decltype(std::declval<Base>().get(0));
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;

	size_t size() const { return Base::count_get(); }
	bool empty() const { return size() == 0; }

	// TODO Handle const ArrayAccessorProxy
	struct ElementAccessor {
		ArrayAccessorProxy& proxy;
		size_t index;
		ElementAccessor(ArrayAccessorProxy& proxy, size_t index) : proxy(proxy), index(index) {}
		operator T() const { return proxy.get(index); }
		T operator->() const { return (T) *this; }
		T operator+() const { return +(T) *this; }
		T operator-() const { return -(T) *this; }
		ElementAccessor& operator=(const T& t) { proxy.set(index, t); return *this; }
		ElementAccessor& operator=(const ElementAccessor& other) { return *this = (T) other; }
		ElementAccessor& operator+=(const T& t) { return *this = (T) *this + t; }
		ElementAccessor& operator-=(const T& t) { return *this = (T) *this - t; }
		ElementAccessor& operator*=(const T& t) { return *this = (T) *this * t; }
		ElementAccessor& operator/=(const T& t) { return *this = (T) *this / t; }
		ElementAccessor& operator++() { return *this += 1; }
		T operator++(int) { T tmp = *this; *this = tmp + 1; return tmp; }
		ElementAccessor& operator--() { return *this -= 1; }
		T operator--(int) { T tmp = *this; *this = tmp - 1; return tmp; }
	};
	using value_type = ElementAccessor;

	ElementAccessor operator[](size_t index) { return ElementAccessor(*this, index); }

	// Resize methods. Only valid if Base::count_set() exists.

	void resize(size_t count) { Base::count_set(count); }
	void clear() { resize(0); }

	/** Assignment from vector */
	ArrayAccessorProxy& operator=(const std::vector<T>& v) {
		size_t count = v.size();
		resize(count);
		for (size_t i = 0; i < count; i++) {
			(*this)[i] = v[i];
		}
		return *this;
	}

	void push_back(const T& t) {
		size_t count = size();
		resize(count + 1);
		(*this)[count] = t;
	}

	void pop_back() {
		resize(size() - 1);
	}

	// Iterators

	using iterator = ArrayAccessorProxyIterator<ArrayAccessorProxy>;
	using const_iterator = ArrayAccessorProxyIterator<const ArrayAccessorProxy>;

	iterator begin() { return iterator(*this, 0); }
	const_iterator begin() const { return const_iterator(*this, 0); }
	iterator end() { return iterator(*this, size()); }
	const_iterator end() const { return const_iterator(*this, size()); }
	std::reverse_iterator<iterator> rbegin() { return std::reverse_iterator<iterator>(end()); }
	std::reverse_iterator<const_iterator> rbegin() const { return std::reverse_iterator<const_iterator>(end()); }
	std::reverse_iterator<iterator> rend() { return std::reverse_iterator<iterator>(begin()); }
	std::reverse_iterator<const_iterator> rend() const { return std::reverse_iterator<const_iterator>(begin()); }
};


#define ARRAY_ACCESSOR_PROXY_METHODS(PROP, TYPE, COUNT, GETTER, SETTER) \
	ARRAY_GETTER_PROXY_METHODS(TYPE, COUNT, GETTER) \
	void set(size_t index, TYPE PROP) { \
		Object* self = self_get(); \
		(void) self; \
		SETTER \
	}


#define ARRAY_ACCESSOR_PROXY_CUSTOM(CPPCLASS, PROP, PROPS, TYPE, COUNT, GETTER, SETTER) \
	struct Proxy_##PROPS { \
		PROXY_METHODS(CPPCLASS, PROPS) \
		ARRAY_ACCESSOR_PROXY_METHODS(PROP, TYPE, COUNT, GETTER, SETTER) \
	}; \
	[[no_unique_address]] \
	ArrayAccessorProxy<Proxy_##PROPS> PROPS


/** Behaves like a mutable array but wraps an element getter/setter function pair.
PROPS should be plural since C++ arrays are typically plural.

Example:
	ARRAY_ACCESSOR_PROXY(AnimalWrapper, Animal, child, children, Object*);

Usage:
	// Everything in ARRAY_GETTER_PROXY, plus
	animalWrapper->children[index] = child; // Calls Animal_child_set(animal, index, child);
*/
#define ARRAY_ACCESSOR_PROXY(CPPCLASS, CLASS, PROP, PROPS, TYPE) \
	ARRAY_ACCESSOR_PROXY_CUSTOM(CPPCLASS, PROP, PROPS, TYPE, { \
		return GET(self, CLASS, PROP##_count); \
	}, { \
		return GET(self, CLASS, PROP, index); \
	}, { \
		SET(self, CLASS, PROP, index, PROP); \
	})


#endif // __cplusplus
