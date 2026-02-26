#pragma once

#include "ObjectProxies.h"
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
The ObjectProxy does not own the Object (unless you call `own()`), and the ObjectProxy is deleted when the Object is freed.
You do not need to delete the ObjectProxy, but it is safe to do so.

## Bound

Or, you can create an ObjectProxy which creates, owns, and "binds" an Object, which means that it overrides all of Object's virtual methods with its own C++ virtual methods.
When the Object's methods are called (such as `Animal_speak()`), your overridden C++ methods (such as `Animal::speak()`) are called.

See Animal.hpp for an example C++ class that wraps an Animal object.
*/
struct ObjectProxy {
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
		// Check if the Object has a bound C++ proxy that can downcast to T
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

	/** Returns a proxy of type T and releases the Object.
	*/
	template <class T>
	static T* of_release(Object* self) {
		if (!self)
			return NULL;
		T* proxy = of<T>(self);
		Object_unref(self);
		return proxy;
	}

	template <class T>
	static const T* of_release(const Object* self) {
		return of_release<T>(const_cast<Object*>(self));
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
		if (bound) {
			ObjectProxies_bound_set(self, (void*) this, &typeid(ObjectProxy), destructor);
		}
	}

public:
	/** Objects can't be copied by default, so disable copying ObjectProxy. */
	ObjectProxy(const ObjectProxy&) = delete;
	ObjectProxy& operator=(const ObjectProxy&) = delete;

	virtual ~ObjectProxy() {
		// Remove proxy from ObjectProxies
		if (bound) {
			// Clear bound proxy destructor, so Object_unref below doesn't call ObjectProxy destructor again
			ObjectProxies_bound_set(self, this, &typeid(ObjectProxy), NULL);
		}
		else {
			ObjectProxies_remove(self, this);
		}
		// Release Object if the proxy owns it
		if (owns) {
			Object_unref(self);
		}
	}

	/** Gets the Object, preserving constness. */
	Object* self_get() { return self; }
	const Object* self_get() const { return self; }

	bool bound_get() const { return bound; }

	size_t use_count() const {
		return Object_refs_get(self_get());
	}

	/** Takes ownership of the Object.
	Increments the reference count. The proxy will unreference the Object when destroyed.
	*/
	void own() {
		if (owns)
			return;
		owns = true;
		Object_ref(self);
	}

	/** Gives up ownership of the Object.
	Decrements the reference count. The proxy will no longer unreference the Object when destroyed.
	Note: If the Object has no other owners, this deletes the Object and ObjectProxy.
	*/
	void disown() {
		if (!owns)
			return;
		owns = false;
		Object_unref(self);
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

private:
	static void destructor(void* p) {
		ObjectProxy* proxy = static_cast<ObjectProxy*>(p);
		// Release ObjectProxy's ownership of Object so it doesn't release it again in ~ObjectProxy().
		proxy->owns = false;
		delete proxy;
	}

	Object* self;
	bool bound;
	bool owns;
};


/** Overrides an Object's method with a lambda/thunk that calls your ObjectProxy subclass' method.
Provides a `that` variable that points to your ObjectProxy.

Example:
	BIND_METHOD(Animal, Animal, speak, (), {
		that->speak();
	});
*/
#define BIND_METHOD(CPPCLASS, CLASS, METHOD, ARGTYPES, CODE) \
	Object_method_push(self_get(), (void*) &CLASS##_##METHOD, (void*) static_cast<CLASS##_##METHOD##_m*>([](Object* self COMMA_EXPAND ARGTYPES) { \
		CPPCLASS* that = static_cast<CPPCLASS*>(ObjectProxies_bound_get(self, NULL)); \
		CODE \
	}))

#define BIND_METHOD_CONST(CPPCLASS, CLASS, METHOD, ARGTYPES, CODE) \
	Object_method_push(self_get(), (void*) &CLASS##_##METHOD, (void*) static_cast<CLASS##_##METHOD##_m*>([](const Object* self COMMA_EXPAND ARGTYPES) { \
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
	(bound_get() ? CLASS##_##METHOD##_mdirect : BASE_CLASS##_##METHOD)(self_get(), ##__VA_ARGS__)

/** Gets a virtual property, using direct call if bound or dispatch call if not. */
#define PROXY_GET(CLASS, BASE_CLASS, PROP, ...) \
	(bound_get() ? CLASS##_##PROP##_get_mdirect : BASE_CLASS##_##PROP##_get)(self_get(), ##__VA_ARGS__)

/** Sets a virtual property, using direct call if bound or dispatch call if not. */
#define PROXY_SET(CLASS, BASE_CLASS, PROP, ...) \
	(bound_get() ? CLASS##_##PROP##_set_mdirect : BASE_CLASS##_##PROP##_set)(self_get(), ##__VA_ARGS__)


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
		if (!proxy->bound_get())
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
			Object_ref(proxy->self_get());
	}

	void release() noexcept {
		if (proxy)
			Object_unref(proxy->self_get());
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

	WeakRef(T* proxy) : proxy(proxy), object(ObjectProxy::self_get(proxy)) {
		Object_weak_ref(object);
	}

	WeakRef(const Ref<T>& other) : proxy(other.get()), object(ObjectProxy::self_get(proxy)) {
		Object_weak_ref(object);
	}

	WeakRef(const WeakRef& other) : proxy(other.proxy), object(other.object) {
		Object_weak_ref(object);
	}

	WeakRef(WeakRef&& other) : proxy(other.proxy), object(other.object) {
		other.proxy = nullptr;
		other.object = nullptr;
	}

	~WeakRef() {
		reset();
	}

	WeakRef& operator=(const WeakRef& other) {
		if (this != &other) {
			reset();
			proxy = other.proxy;
			object = other.object;
			Object_weak_ref(object);
		}
		return *this;
	}

	WeakRef& operator=(WeakRef&& other) {
		if (this != &other) {
			reset();
			proxy = other.proxy;
			object = other.object;
			other.proxy = nullptr;
			other.object = nullptr;
		}
		return *this;
	}

	Ref<T> lock() const {
		if (!object || !proxy)
			return Ref<T>();
		if (!Object_weak_lock(object))
			return Ref<T>();
		// We now have a strong reference. Cached ObjectProxy is still valid.
		Ref<T> ref(proxy);
		// ref obtained another reference, so release the one from weak_lock.
		Object_unref(object);
		return ref;
	}

	void reset() {
		Object_weak_unref(object);
		object = nullptr;
		proxy = nullptr;
	}

	void swap(WeakRef& other) {
		T* p = proxy;
		Object* o = object;
		proxy = other.proxy;
		object = other.object;
		other.proxy = p;
		other.object = o;
	}

	size_t use_count() const {
		return object ? Object_refs_get(object) : 0;
	}

	bool expired() const {
		return object ? Object_refs_get(object) == 0 : true;
	}

	bool operator==(const WeakRef& other) {
		return proxy == other.proxy;
	}

private:
	T* proxy = nullptr;
	Object* object = nullptr;
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
	T operator~() const { return ~(T) *this; }

	template <typename U = T, std::enable_if_t<!std::is_same_v<U, bool>, int> = 0>
	explicit operator bool() const { return static_cast<bool>((T) *this); }

	template <typename U = T, std::enable_if_t<!std::is_same_v<U, bool>, int> = 0>
	bool operator!() const { return !static_cast<bool>((T) *this); }

	template <typename U, std::enable_if_t<std::is_class_v<U>, int> = 0>
	friend bool operator==(const GetterProxy& a, const U& b) { return a.get() == b; }
	template <typename U, std::enable_if_t<std::is_class_v<U>, int> = 0>
	friend bool operator==(const U& a, const GetterProxy& b) { return a == b.get(); }
	template <typename U, std::enable_if_t<std::is_class_v<U>, int> = 0>
	friend bool operator!=(const GetterProxy& a, const U& b) { return a.get() != b; }
	template <typename U, std::enable_if_t<std::is_class_v<U>, int> = 0>
	friend bool operator!=(const U& a, const GetterProxy& b) { return a != b.get(); }
	template <typename U, std::enable_if_t<std::is_class_v<U>, int> = 0>
	friend bool operator<(const GetterProxy& a, const U& b) { return a.get() < b; }
	template <typename U, std::enable_if_t<std::is_class_v<U>, int> = 0>
	friend bool operator<(const U& a, const GetterProxy& b) { return a < b.get(); }
	template <typename U, std::enable_if_t<std::is_class_v<U>, int> = 0>
	friend bool operator>(const GetterProxy& a, const U& b) { return a.get() > b; }
	template <typename U, std::enable_if_t<std::is_class_v<U>, int> = 0>
	friend bool operator>(const U& a, const GetterProxy& b) { return a > b.get(); }
	template <typename U, std::enable_if_t<std::is_class_v<U>, int> = 0>
	friend bool operator<=(const GetterProxy& a, const U& b) { return a.get() <= b; }
	template <typename U, std::enable_if_t<std::is_class_v<U>, int> = 0>
	friend bool operator<=(const U& a, const GetterProxy& b) { return a <= b.get(); }
	template <typename U, std::enable_if_t<std::is_class_v<U>, int> = 0>
	friend bool operator>=(const GetterProxy& a, const U& b) { return a.get() >= b; }
	template <typename U, std::enable_if_t<std::is_class_v<U>, int> = 0>
	friend bool operator>=(const U& a, const GetterProxy& b) { return a >= b.get(); }
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


#define GLOBAL_GETTER_PROXY_CUSTOM(PROP, TYPE, GETTER) \
	struct Proxy_##PROP { \
		TYPE get() const { GETTER } \
	}; \
	[[maybe_unused]] static const GetterProxy<Proxy_##PROP> PROP


/** Behaves like a const global variable but wraps a free getter function.
Example:
	GLOBAL_GETTER_PROXY(zoo, temperature, float);

Usage:
	float t = temperature; // Calls zoo_temperature_get()
*/
#define GLOBAL_GETTER_PROXY(PREFIX, PROP, TYPE) \
	GLOBAL_GETTER_PROXY_CUSTOM(PROP, TYPE, { \
		return PREFIX##_##PROP##_get(); \
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
	AccessorProxy& operator%=(const T& t) { return *this = (T) *this % t; }
	AccessorProxy& operator&=(const T& t) { return *this = (T) *this & t; }
	AccessorProxy& operator|=(const T& t) { return *this = (T) *this | t; }
	AccessorProxy& operator^=(const T& t) { return *this = (T) *this ^ t; }
	AccessorProxy& operator<<=(const T& t) { return *this = (T) *this << t; }
	AccessorProxy& operator>>=(const T& t) { return *this = (T) *this >> t; }
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


#define GLOBAL_ACCESSOR_PROXY_CUSTOM(PROP, TYPE, GETTER, SETTER) \
	struct Proxy_##PROP { \
		TYPE get() const { GETTER } \
		void set(TYPE PROP) { SETTER } \
	}; \
	[[maybe_unused]] static AccessorProxy<Proxy_##PROP> PROP


/** Behaves like a mutable global variable but wraps free getter/setter functions.
Example:
	GLOBAL_ACCESSOR_PROXY(zoo, temperature, float);

Usage:
	float t = temperature; // Calls zoo_temperature_get()
	temperature = 72.0f; // Calls zoo_temperature_set(72.0f)
*/
#define GLOBAL_ACCESSOR_PROXY(PREFIX, PROP, TYPE) \
	GLOBAL_ACCESSOR_PROXY_CUSTOM(PROP, TYPE, { \
		return PREFIX##_##PROP##_get(); \
	}, { \
		PREFIX##_##PROP##_set(PROP); \
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
		ElementAccessor& operator%=(const T& t) { return *this = (T) *this % t; }
		ElementAccessor& operator&=(const T& t) { return *this = (T) *this & t; }
		ElementAccessor& operator|=(const T& t) { return *this = (T) *this | t; }
		ElementAccessor& operator^=(const T& t) { return *this = (T) *this ^ t; }
		ElementAccessor& operator<<=(const T& t) { return *this = (T) *this << t; }
		ElementAccessor& operator>>=(const T& t) { return *this = (T) *this >> t; }
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


/** Holds a std::string and implicitly converts to const char*. */
struct CStrProxy {
	std::string s;
	operator const char*() const {
		return s.c_str();
	}
};


template <typename Base>
struct StringGetterProxy : GetterProxy<Base> {
	size_t size() const { return Base::get().size(); }
	size_t length() const { return Base::get().length(); }
	bool empty() const { return Base::get().empty(); }
	CStrProxy c_str() const { return {Base::get()}; }

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


#define GLOBAL_STRING_GETTER_PROXY_CUSTOM(PROP, GETTER) \
	struct Proxy_##PROP { \
		std::string get() const { GETTER } \
	}; \
	[[maybe_unused]] static const StringGetterProxy<Proxy_##PROP> PROP


/** Behaves like a `const std::string` global but wraps a `char*` getter where the caller must free(). */
#define GLOBAL_STRING_GETTER_PROXY(PREFIX, PROP) \
	GLOBAL_STRING_GETTER_PROXY_CUSTOM(PROP, { \
		char* c = PREFIX##_##PROP##_get(); \
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
	CStrProxy c_str() const { return {Base::get()}; }

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


#define GLOBAL_STRING_ACCESSOR_PROXY_CUSTOM(PROP, GETTER, SETTER) \
	struct Proxy_##PROP { \
		std::string get() const { GETTER } \
		void set(std::string PROP) { SETTER } \
	}; \
	[[maybe_unused]] static StringAccessorProxy<Proxy_##PROP> PROP


/** Behaves like a `std::string` global but wraps a `char*` getter (caller frees) and `const char*` setter. */
#define GLOBAL_STRING_ACCESSOR_PROXY(PREFIX, PROP) \
	GLOBAL_STRING_ACCESSOR_PROXY_CUSTOM(PROP, { \
		char* c = PREFIX##_##PROP##_get(); \
		if (!c) return std::string(); \
		std::string s = c; \
		free(c); \
		return s; \
	}, { \
		PREFIX##_##PROP##_set(PROP.c_str()); \
	})
