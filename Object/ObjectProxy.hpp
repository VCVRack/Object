#pragma once

#include "ObjectProxies.h"
#include "WeakObject.h"
#include <cassert>
#include <cstdlib>
#include <vector>
#include <string>
#include <type_traits>
#include <iterator>
#include <utility>


/** Base class that wraps an Object and its methods.

A library using VCV Object can offer a C++ proxy API by writing an ObjectProxy subclass for each Object class and writing proxy virtual/non-virtual methods and accessors.

Once an ObjectProxy subclass is defined, there are two ways to use it.

## Non-bound

You can obtain an ObjectProxy from an existing Object by calling `ObjectProxy::of<T>(object)`.
This calls the constructor `T(Object)` unless an ObjectProxy of type T is cached.
The ObjectProxy does not own the Object (unless you call `adopt()`), and the ObjectProxy is deleted when the Object is freed.
You do not need to delete the ObjectProxy, but it is safe to do so.

## Bound

Or, you can create an ObjectProxy which creates, owns, and "binds" an Object, which means that it overrides all of Object's virtual methods with its own C++ virtual methods.
When the Object's methods are called (such as `Animal_speak()`), your overridden C++ methods (such as `Animal::speak()`) are called.

See Animal.hpp for an example C++ class that wraps an Animal object.
*/
struct ObjectProxy {
	Object* const self;
	/** True if this proxy bound its virtual methods to the Object. */
	const bool bound;
	/** True if this proxy owns the Object and will release it upon destruction. */
	bool owns;

	/** Returns a proxy of type T for the given Object.
	If the Object has a bound C++ proxy that can be cast to T, returns that.
	Otherwise, returns a cached proxy if one exists for type T.
	Otherwise, creates a new T instance and registers it with &typeid(T).
	*/
	template <class T>
	static T* of(Object* self) {
		static_assert(std::is_base_of<ObjectProxy, T>::value, "T must be a subclass of ObjectProxy");
		if (!self)
			return NULL;
		// Check if the Object has a bound C++ proxy that can cast to T
		const void* boundType = NULL;
		void* boundProxy = ObjectProxies_bound_get(self, &boundType);
		if (boundProxy && boundType == &typeid(ObjectProxy)) {
			T* boundT = dynamic_cast<T*>(static_cast<ObjectProxy*>(boundProxy));
			if (boundT)
				return boundT;
		}
		// Look up cached proxy by type
		T* proxy = static_cast<T*>(ObjectProxies_get(self, &typeid(T)));
		if (proxy)
			return proxy;
		// Create new proxy
		proxy = new T(self);
		ObjectProxies_add(self, proxy, &typeid(T), destructor);
		return proxy;
	}

	template <class T>
	static const T* of(const Object* self) {
		return of<T>(const_cast<Object*>(self));
	}

	/** Constructs an ObjectProxy with a new Object.
	Each ObjectProxy subclass should implement its own default constructor that creates a specialized Object.
	*/
	ObjectProxy() : ObjectProxy(Object_create(), true) {}

protected:
	/** Constructs an ObjectProxy with an existing Object.
	If `bind` is true, use BIND_* macros to override the Object's virtual methods with C++ virtual methods.
	*/
	ObjectProxy(Object* self, bool bind = false) : self(self), bound(bind), owns(bind) {
		ObjectProxies_specialize(self);
		if (bound)
			ObjectProxies_bound_set(self, (void*) this, &typeid(ObjectProxy));
	}

public:
	/** Objects can't be copied by default, so disable copying ObjectProxy. */
	ObjectProxy(const ObjectProxy&) = delete;
	ObjectProxy& operator=(const ObjectProxy&) = delete;

	virtual ~ObjectProxy() {
		if (owns)
			Object_release(self);
		else
			ObjectProxies_remove(self, this);
	}

	/** Takes ownership of the Object.
	After adopting, this proxy will release the Object when destroyed.
	Does not obtain a reference to Object with Object_obtain().
	*/
	void adopt() {
		if (owns)
			return;
		owns = true;
		ObjectProxies_remove(self, this);
	}

	/** Gets the Object, preserving constness. */
	Object* self_get() { return self; }
	const Object* self_get() const { return self; }

	size_t use_count() const {
		return Object_refs_get(self_get());
	}

	/** Gets an ObjectProxy's Object, gracefully handling NULL. */
	static Object* self_get(ObjectProxy* proxy) {
		if (!proxy)
			return NULL;
		return proxy->self;
	}
	static const Object* self_get(const ObjectProxy* proxy) {
		if (!proxy)
			return NULL;
		return proxy->self;
	}

	static void destructor(void* proxy) {
		delete static_cast<ObjectProxy*>(proxy);
	}
};


/** Overrides an Object's method with a lambda/thunk that calls your ObjectProxy subclass' method.
Provides a `that` variable that points to your ObjectProxy.

Example:
	BIND_METHOD(Animal, Animal, speak, (), {
		that->speak();
	});
*/
#define BIND_METHOD(CPPCLASS, CLASS, METHOD, ARGTYPES, CODE) \
	Object_method_push(self, (void*) &CLASS##_##METHOD, (void*) static_cast<CLASS##_##METHOD##_m*>([](Object* self COMMA_EXPAND ARGTYPES) { \
		CPPCLASS* that = static_cast<CPPCLASS*>(ObjectProxies_bound_get(self, NULL)); \
		CODE \
	}))

#define BIND_METHOD_CONST(CPPCLASS, CLASS, METHOD, ARGTYPES, CODE) \
	Object_method_push(self, (void*) &CLASS##_##METHOD, (void*) static_cast<CLASS##_##METHOD##_m*>([](const Object* self COMMA_EXPAND ARGTYPES) { \
		const CPPCLASS* that = static_cast<CPPCLASS*>(ObjectProxies_bound_get(self, NULL)); \
		CODE \
	}))

#define BIND_GETTER(CPPCLASS, CLASS, PROP, CODE) \
	BIND_METHOD_CONST(CPPCLASS, CLASS, PROP##_get, (), CODE)

#define BIND_SETTER(CPPCLASS, CLASS, PROP, TYPE, CODE) \
	BIND_METHOD(CPPCLASS, CLASS, PROP##_set, (TYPE PROP), CODE)

#define BIND_ACCESSOR(CPPCLASS, CLASS, PROP, TYPE, GETTER, SETTER) \
	BIND_GETTER(CPPCLASS, CLASS, PROP, GETTER); \
	BIND_SETTER(CPPCLASS, CLASS, PROP, TYPE, SETTER)


/** Calls a virtual method, using direct call if bound or dispatch call if not.
Use in ObjectProxy subclass virtual methods.
*/
#define PROXY_CALL(CLASS, BASE_CLASS, METHOD, ...) \
	(bound ? CLASS##_##METHOD##_mdirect : BASE_CLASS##_##METHOD)(self, ##__VA_ARGS__)

/** Gets a virtual property, using direct call if bound or dispatch call if not. */
#define PROXY_GET(CLASS, BASE_CLASS, PROP, ...) \
	(bound ? CLASS##_##PROP##_get_mdirect : BASE_CLASS##_##PROP##_get)(self, ##__VA_ARGS__)

/** Sets a virtual property, using direct call if bound or dispatch call if not. */
#define PROXY_SET(CLASS, BASE_CLASS, PROP, ...) \
	(bound ? CLASS##_##PROP##_set_mdirect : BASE_CLASS##_##PROP##_set)(self, ##__VA_ARGS__)


/** Reference counter for an ObjectProxy subclass.

If T is newly constructed, use
	Ref<Animal> ref(new Animal(), true);

or construct in-place like
	Ref<Animal> ref(std::in_place, "Fido");
*/
template <class T>
struct Ref {
	static_assert(std::is_base_of<ObjectProxy, T>::value, "Ref can only hold references to ObjectProxy subclasses");

	using element_type = T;

	Ref() {}

	// ObjectProxy constructor
	Ref(T* proxy, bool adopt = false) : proxy(proxy) {
		if (!adopt)
			obtain();
	}

	// Available in C++17
#if __cplusplus >= 201703L
	// In-place constructor
	template <typename... Args>
	explicit Ref(std::in_place_t, Args&&... args) : proxy(new T(std::forward<Args>(args)...)) {
		// If constructing a bound proxy, we already hold the sole reference.
		// If constructing a proxy from an existing Object, obtain a reference.
		if (!proxy->bound)
			obtain();
	}
#endif

	// Copy constructor
	Ref(const Ref& other) {
		proxy = other.proxy;
		obtain();
	}

	// Move constructor
	Ref(Ref&& other) {
		proxy = other.proxy;
		other.proxy = nullptr;
	}

	~Ref() {
		release();
	}

	// Copy assignment
	Ref& operator=(const Ref& other) {
		reset(other.proxy);
		return *this;
	}

	// Move assignment
	Ref& operator=(Ref&& other) {
		if (this != &other) {
			release();
			proxy = other.proxy;
			other.proxy = nullptr;
		}
		return *this;
	}

	void reset(T* proxy = nullptr) {
		if (this->proxy == proxy)
			return;
		release();
		this->proxy = proxy;
		obtain();
	}

	void swap(Ref& other) {
		T* proxy = this->proxy;
		this->proxy = other.proxy;
		other.proxy = proxy;
	}

	T* get() const { return proxy; }
	T& operator*() const { return *proxy; }
	T* operator->() const { return proxy; }

	size_t use_count() const {
		return proxy ? Object_refs_get(proxy->self_get()) : 0;
	}

	explicit operator bool() const { return proxy; }

	bool operator==(const Ref& other) {
		return proxy == other.proxy;
	}

private:
	T* proxy = NULL;

	void obtain() noexcept {
		if (proxy)
			Object_obtain(proxy->self_get());
	}

	void release() noexcept {
		if (proxy)
			Object_release(proxy->self_get());
	}
};


/** Weak reference counter for an ObjectProxy subclass.
Similar to std::weak_ptr.
*/
template <class T>
struct WeakRef {
	static_assert(std::is_base_of<ObjectProxy, T>::value, "WeakRef can only hold references to ObjectProxy subclasses");

	using element_type = T;

	WeakRef() {}

	WeakRef(T* proxy) {
		this->proxy = proxy;
		weakObject = proxy ? WeakObject_create(proxy->self) : nullptr;
	}

	WeakRef(const Ref<T>& other) {
		proxy = other.get();
		weakObject = proxy ? WeakObject_create(proxy->self) : nullptr;
	}

	WeakRef(const WeakRef& other) {
		proxy = other.proxy;
		weakObject = other.weakObject;
		if (weakObject)
			WeakObject_obtain(weakObject);
	}

	WeakRef(WeakRef&& other) {
		proxy = other.proxy;
		weakObject = other.weakObject;
		other.proxy = nullptr;
		other.weakObject = nullptr;
	}

	~WeakRef() {
		reset();
	}

	WeakRef& operator=(const WeakRef& other) {
		if (this != &other) {
			reset();
			proxy = other.proxy;
			weakObject = other.weakObject;
			if (weakObject)
				WeakObject_obtain(weakObject);
		}
		return *this;
	}

	WeakRef& operator=(WeakRef&& other) {
		if (this != &other) {
			reset();
			proxy = other.proxy;
			weakObject = other.weakObject;
			other.proxy = nullptr;
			other.weakObject = nullptr;
		}
		return *this;
	}

	Ref<T> lock() const {
		if (!weakObject || !proxy)
			return Ref<T>();
		Object* object = WeakObject_get(weakObject);
		if (!object)
			return Ref<T>();
		// Cached ObjectProxy is guaranteed to still be valid.
		Ref<T> ref(proxy);
		// ref now owns a reference so we don't need this one.
		Object_release(object);
		return ref;
	}

	void reset() {
		if (!weakObject)
			return;
		WeakObject_release(weakObject);
		weakObject = nullptr;
		proxy = nullptr;
	}

	void swap(WeakRef& other) {
		T* proxy = this->proxy;
		WeakObject* weakObject = this->weakObject;
		this->proxy = other.proxy;
		this->weakObject = other.weakObject;
		other.proxy = proxy;
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
		return proxy == other.proxy;
	}

private:
	T* proxy = nullptr;
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
	CPPCLASS* this_get() const { \
		return CONTAINER_OF(this, CPPCLASS, PROP); \
	} \
	Object* self_get() const { \
		return this_get()->self_get(); \
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
	GETTER_PROXY(AnimalProxy, Animal, legs, int);

Usage:
	int legs = animalProxy->legs; // Calls Animal_legs_get(animal);
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
	AccessorProxy& operator++() { return *this += T(1); }
	T operator++(int) { T tmp = *this; *this = tmp + T(1); return tmp; }
	AccessorProxy& operator--() { return *this -= T(1); }
	T operator--(int) { T tmp = *this; *this = tmp - T(1); return tmp; }
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
	ACCESSOR_PROXY(AnimalProxy, Animal, legs, int);

Usage:
	int legs = animalProxy->legs; // Calls Animal_legs_get(animal);
	animalProxy->legs = 4; // Calls Animal_legs_set(animal, 3);
*/
#define ACCESSOR_PROXY(CPPCLASS, CLASS, PROP, TYPE) \
	ACCESSOR_PROXY_CUSTOM(CPPCLASS, PROP, TYPE, { \
		return GET(self, CLASS, PROP); \
	}, { \
		SET(self, CLASS, PROP, PROP); \
	})


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

	struct iterator {
		const ArrayGetterProxy& proxy;
		size_t i;

		using iterator_category = std::random_access_iterator_tag;
		using value_type = ArrayGetterProxy::value_type;
		using difference_type = std::ptrdiff_t;
		using reference = value_type;
		using pointer = value_type;

		reference operator*() const { return proxy[i]; }
		pointer operator->() const { return proxy[i]; }

		iterator& operator++() { ++i; return *this; }
		iterator operator++(int) { iterator tmp = *this; ++i; return tmp; }
		iterator& operator--() { --i; return *this; }
		iterator operator--(int) { iterator tmp = *this; --i; return tmp; }

		iterator& operator+=(difference_type n) { i += n; return *this; }
		iterator& operator-=(difference_type n) { i -= n; return *this; }
		iterator operator+(difference_type n) const { return iterator{proxy, i + n}; }
		iterator operator-(difference_type n) const { return iterator{proxy, i - n}; }
		difference_type operator-(const iterator& other) const { return i - other.i; }

		reference operator[](difference_type n) const { return proxy[i + n]; }

		bool operator==(const iterator& other) const { return i == other.i; }
		bool operator!=(const iterator& other) const { return i != other.i; }
		bool operator<(const iterator& other) const { return i < other.i; }
		bool operator>(const iterator& other) const { return i > other.i; }
		bool operator<=(const iterator& other) const { return i <= other.i; }
		bool operator>=(const iterator& other) const { return i >= other.i; }
	};
	using const_iterator = iterator;

	iterator begin() const { return iterator{*this, 0}; }
	iterator end() const { return iterator{*this, size()}; }
	std::reverse_iterator<iterator> rbegin() const { return std::reverse_iterator<iterator>(end()); }
	std::reverse_iterator<iterator> rend() const { return std::reverse_iterator<iterator>(begin()); }
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
	ARRAY_GETTER_PROXY(AnimalProxy, children, Animal, child, Object*);

Usage:
	size_t childrenSize = animalProxy->children.size(); // Calls Animal_child_count_get(animal);
	Object* child = animalProxy->children[index]; // Calls Animal_child_get(animal, index);
*/
#define ARRAY_GETTER_PROXY(CPPCLASS, PROPS, CLASS, PROP, TYPE) \
	ARRAY_GETTER_PROXY_CUSTOM(CPPCLASS, PROPS, TYPE, { \
		return GET(self, CLASS, PROP##_count); \
	}, { \
		return GET(self, CLASS, PROP, index); \
	})


template <typename Base>
struct ArrayAccessorProxy : Proxy<Base> {
	using T = decltype(std::declval<Base>().get(0));
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;

	size_t size() const { return Base::count_get(); }
	bool empty() const { return size() == 0; }

	struct ElementAccessor {
		ArrayAccessorProxy& proxy;
		size_t index;
		ElementAccessor(ArrayAccessorProxy& proxy, size_t index) : proxy(proxy), index(index) {}
		ElementAccessor(const ElementAccessor&) = default;
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
	T operator[](size_t index) const { return Base::get(index); }

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

	struct iterator {
		ArrayAccessorProxy& proxy;
		size_t i;

		using iterator_category = std::random_access_iterator_tag;
		using value_type = ElementAccessor;
		using difference_type = std::ptrdiff_t;
		using reference = value_type;
		using pointer = value_type*;

		reference operator*() const { return proxy[i]; }
		reference operator[](difference_type n) const { return proxy[i + n]; }

		iterator& operator++() { ++i; return *this; }
		iterator operator++(int) { iterator tmp = *this; ++i; return tmp; }
		iterator& operator--() { --i; return *this; }
		iterator operator--(int) { iterator tmp = *this; --i; return tmp; }

		iterator& operator+=(difference_type n) { i += n; return *this; }
		iterator& operator-=(difference_type n) { i -= n; return *this; }
		iterator operator+(difference_type n) const { return iterator{proxy, i + n}; }
		iterator operator-(difference_type n) const { return iterator{proxy, i - n}; }
		difference_type operator-(const iterator& other) const { return i - other.i; }

		bool operator==(const iterator& other) const { return i == other.i; }
		bool operator!=(const iterator& other) const { return i != other.i; }
		bool operator<(const iterator& other) const { return i < other.i; }
		bool operator>(const iterator& other) const { return i > other.i; }
		bool operator<=(const iterator& other) const { return i <= other.i; }
		bool operator>=(const iterator& other) const { return i >= other.i; }
	};

	struct const_iterator {
		const ArrayAccessorProxy& proxy;
		size_t i;

		using iterator_category = std::random_access_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using reference = value_type;
		using pointer = value_type;

		reference operator*() const { return proxy[i]; }
		reference operator[](difference_type n) const { return proxy[i + n]; }

		const_iterator& operator++() { ++i; return *this; }
		const_iterator operator++(int) { const_iterator tmp = *this; ++i; return tmp; }
		const_iterator& operator--() { --i; return *this; }
		const_iterator operator--(int) { const_iterator tmp = *this; --i; return tmp; }

		const_iterator& operator+=(difference_type n) { i += n; return *this; }
		const_iterator& operator-=(difference_type n) { i -= n; return *this; }
		const_iterator operator+(difference_type n) const { return const_iterator{proxy, i + n}; }
		const_iterator operator-(difference_type n) const { return const_iterator{proxy, i - n}; }
		difference_type operator-(const const_iterator& other) const { return i - other.i; }

		bool operator==(const const_iterator& other) const { return i == other.i; }
		bool operator!=(const const_iterator& other) const { return i != other.i; }
		bool operator<(const const_iterator& other) const { return i < other.i; }
		bool operator>(const const_iterator& other) const { return i > other.i; }
		bool operator<=(const const_iterator& other) const { return i <= other.i; }
		bool operator>=(const const_iterator& other) const { return i >= other.i; }
	};

	iterator begin() { return iterator{*this, 0}; }
	const_iterator begin() const { return const_iterator{*this, 0}; }
	iterator end() { return iterator{*this, size()}; }
	const_iterator end() const { return const_iterator{*this, size()}; }
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


#define ARRAY_ACCESSOR_PROXY_CUSTOM(CPPCLASS, PROPS, PROP, TYPE, COUNT, GETTER, SETTER) \
	struct Proxy_##PROPS { \
		PROXY_METHODS(CPPCLASS, PROPS) \
		ARRAY_ACCESSOR_PROXY_METHODS(PROP, TYPE, COUNT, GETTER, SETTER) \
	}; \
	[[no_unique_address]] \
	ArrayAccessorProxy<Proxy_##PROPS> PROPS


/** Behaves like a mutable array but wraps an element getter/setter function pair.
PROPS should be plural since C++ arrays are typically plural.

Example:
	ARRAY_ACCESSOR_PROXY(AnimalProxy, children, Animal, child, Object*);

Usage:
	// Everything in ARRAY_GETTER_PROXY, plus
	animalProxy->children[index] = child; // Calls Animal_child_set(animal, index, child);
*/
#define ARRAY_ACCESSOR_PROXY(CPPCLASS, PROPS, CLASS, PROP, TYPE) \
	ARRAY_ACCESSOR_PROXY_CUSTOM(CPPCLASS, PROPS, PROP, TYPE, { \
		return GET(self, CLASS, PROP##_count); \
	}, { \
		return GET(self, CLASS, PROP, index); \
	}, { \
		SET(self, CLASS, PROP, index, PROP); \
	})


#define VECTOR_ACCESSOR_PROXY_METHODS(PROP, TYPE, COUNTGETTER, COUNTSETTER, GETTER, SETTER) \
	ARRAY_ACCESSOR_PROXY_METHODS(PROP, TYPE, COUNTGETTER, GETTER, SETTER) \
	void count_set(size_t count) { \
		Object* self = self_get(); \
		(void) self; \
		COUNTSETTER \
	}


#define VECTOR_ACCESSOR_PROXY_CUSTOM(CPPCLASS, PROPS, PROP, TYPE, COUNTGETTER, COUNTSETTER, GETTER, SETTER) \
	struct Proxy_##PROPS { \
		PROXY_METHODS(CPPCLASS, PROPS) \
		VECTOR_ACCESSOR_PROXY_METHODS(PROP, TYPE, COUNTGETTER, COUNTSETTER, GETTER, SETTER) \
	}; \
	[[no_unique_address]] \
	ArrayAccessorProxy<Proxy_##PROPS> PROPS


#define VECTOR_ACCESSOR_PROXY(CPPCLASS, PROPS, CLASS, PROP, TYPE) \
	VECTOR_ACCESSOR_PROXY_CUSTOM(CPPCLASS, PROPS, PROP, TYPE, { \
		return GET(self, CLASS, PROP##_count); \
	}, { \
		SET(self, CLASS, PROP##_count, count); \
	}, { \
		return GET(self, CLASS, PROP, index); \
	}, { \
		SET(self, CLASS, PROP, index, PROP); \
	})


template <typename Base>
struct StringGetterProxy : GetterProxy<Base> {
	size_t size() const { return Base::get().size(); }
	size_t length() const { return Base::get().length(); }
	bool empty() const { return Base::get().empty(); }

	friend std::string operator+(const StringGetterProxy& a, const std::string& b) { return a.get() + b; }
	friend std::string operator+(const std::string& a, const StringGetterProxy& b) { return a + b.get(); }
	friend bool operator==(const StringGetterProxy& a, const std::string& b) { return a.get() == b; }
	friend bool operator==(const std::string& a, const StringGetterProxy& b) { return a == b.get(); }
	friend bool operator!=(const StringGetterProxy& a, const std::string& b) { return a.get() != b; }
	friend bool operator!=(const std::string& a, const StringGetterProxy& b) { return a != b.get(); }
};


#define STRING_GETTER_PROXY_CUSTOM(CPPCLASS, PROP, GETTER) \
	struct Proxy_##PROP { \
		PROXY_METHODS(CPPCLASS, PROP) \
		GETTER_PROXY_METHODS(std::string, GETTER) \
	}; \
	[[no_unique_address]] \
	const StringGetterProxy<Proxy_##PROP> PROP


/** Behaves like a `const std::string` member but proxies a `char*` getter function where the caller must free(). */
#define STRING_GETTER_PROXY(CPPCLASS, CLASS, PROP) \
	STRING_GETTER_PROXY_CUSTOM(CPPCLASS, PROP, { \
		char* c = GET(self, CLASS, PROP); \
		if (!c) return std::string(); \
		std::string s = c; \
		free(c); \
		return s; \
	})


template <typename Base>
struct StringAccessorProxy : AccessorProxy<Base> {
	using AccessorProxy<Base>::operator=;

	size_t size() const { return Base::get().size(); }
	size_t length() const { return Base::get().length(); }
	bool empty() const { return Base::get().empty(); }

	friend std::string operator+(const StringAccessorProxy& a, const std::string& b) { return a.get() + b; }
	friend std::string operator+(const std::string& a, const StringAccessorProxy& b) { return a + b.get(); }
	friend bool operator==(const StringAccessorProxy& a, const std::string& b) { return a.get() == b; }
	friend bool operator==(const std::string& a, const StringAccessorProxy& b) { return a == b.get(); }
	friend bool operator!=(const StringAccessorProxy& a, const std::string& b) { return a.get() != b; }
	friend bool operator!=(const std::string& a, const StringAccessorProxy& b) { return a != b.get(); }
};


#define STRING_ACCESSOR_PROXY_CUSTOM(CPPCLASS, PROP, GETTER, SETTER) \
	struct Proxy_##PROP { \
		PROXY_METHODS(CPPCLASS, PROP) \
		ACCESSOR_PROXY_METHODS(PROP, std::string, GETTER, SETTER) \
	}; \
	[[no_unique_address]] \
	StringAccessorProxy<Proxy_##PROP> PROP


/** Behaves like a `std::string` member but proxies a `char*` getter function where the caller must free(), and a `const char*` setter function where the setter copies the string. */
#define STRING_ACCESSOR_PROXY(CPPCLASS, CLASS, PROP) \
	STRING_ACCESSOR_PROXY_CUSTOM(CPPCLASS, PROP, { \
		char* c = GET(self, CLASS, PROP); \
		if (!c) return std::string(); \
		std::string s = c; \
		free(c); \
		return s; \
	}, { \
		SET(self, CLASS, PROP, PROP.c_str()); \
	})
