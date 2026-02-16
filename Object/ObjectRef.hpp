#pragma once

#include "Object.h"


/** Holds a strong reference to an Object using C++ RAII.
T can be `Object` or `const Object`.
TODO: Move to Object library.
*/
template<typename T = Object>
struct ObjectRefT {
	T* object = NULL;

	ObjectRefT() = default;

	/** Transfers ownership of an Object from the caller.
	Does not obtain a new reference.
	*/
	ObjectRefT(T* obj) : object(obj) {}

	/** Obtains a new reference of the Object from another ObjectRefT.
	Both ObjectRefTs then own their own reference.
	*/
	ObjectRefT(const ObjectRefT& other) : object(other.object) {
		Object_ref(object);
	}

	/** Obtains a new reference of the Object from another compatible ObjectRefT.
	Allows implicit conversion from ObjectRef to ConstObjectRef.
	*/
	template<typename U>
	ObjectRefT(const ObjectRefT<U>& other) : object(other.object) {
		Object_ref(object);
	}

	/** Moves a reference from another ObjectRefT. */
	ObjectRefT(ObjectRefT&& other) : object(other.object) {
		other.object = NULL;
	}

	/** Moves a reference from another compatible ObjectRefT. */
	template<typename U>
	ObjectRefT(ObjectRefT<U>&& other) : object(other.object) {
		other.object = NULL;
	}

	~ObjectRefT() {
		T* old = object;
		object = NULL;
		Object_unref(old);
	}

	ObjectRefT& operator=(T* obj) {
		T* old = object;
		object = obj;
		Object_unref(old);
		return *this;
	}

	ObjectRefT& operator=(const ObjectRefT& other) {
		Object_ref(other.object);
		T* old = object;
		object = other.object;
		Object_unref(old);
		return *this;
	}

	template<typename U>
	ObjectRefT& operator=(const ObjectRefT<U>& other) {
		Object_ref(other.object);
		T* old = object;
		object = other.object;
		Object_unref(old);
		return *this;
	}

	ObjectRefT& operator=(ObjectRefT&& other) {
		if (this != &other) {
			T* old = object;
			object = other.object;
			other.object = NULL;
			Object_unref(old);
		}
		return *this;
	}

	template<typename U>
	ObjectRefT& operator=(ObjectRefT<U>&& other) {
		T* old = object;
		object = other.object;
		other.object = NULL;
		Object_unref(old);
		return *this;
	}

	/** Releases ownership of the Object and transfers ownership to the caller.
	*/
	T* release() {
		T* obj = object;
		object = NULL;
		return obj;
	}

	/** Obtains a new reference of the Object.
	*/
	static ObjectRefT obtain(T* obj) {
		Object_ref(obj);
		ObjectRefT ref;
		ref.object = obj;
		return ref;
	}

	operator T*() const { return object; }
};

using ObjectRef = ObjectRefT<Object>;
using ConstObjectRef = ObjectRefT<const Object>;


/** Holds a weak reference to an Object using C++ RAII.
A weak reference does not prevent the object from being freed, but can be converted to a strong ObjectRef if still alive with lock().
T can be `Object` or `const Object`.
TODO: Move to Object library.
*/
template<typename T = Object>
struct WeakObjectRefT {
	T* object = NULL;

	WeakObjectRefT() = default;

	/** Obtains a weak reference of an Object. */
	WeakObjectRefT(T* obj) : object(obj) {
		Object_weak_ref(object);
	}

	/** Obtains a new weak reference of the Object from an ObjectRef. */
	template<typename U>
	WeakObjectRefT(const ObjectRefT<U>& other) : object(other.object) {
		Object_weak_ref(object);
	}

	/** Obtains a new weak reference of the Object from another WeakObjectRefT. */
	WeakObjectRefT(const WeakObjectRefT& other) : object(other.object) {
		Object_weak_ref(object);
	}

	/** Obtains a new weak reference of the Object from another compatible WeakObjectRefT.
	Allows implicit conversion from WeakObjectRef to ConstWeakObjectRef.
	*/
	template<typename U>
	WeakObjectRefT(const WeakObjectRefT<U>& other) : object(other.object) {
		Object_weak_ref(object);
	}

	/** Moves a weak reference from another WeakObjectRefT. */
	WeakObjectRefT(WeakObjectRefT&& other) : object(other.object) {
		other.object = NULL;
	}

	/** Moves a weak reference from another compatible WeakObjectRefT. */
	template<typename U>
	WeakObjectRefT(WeakObjectRefT<U>&& other) : object(other.object) {
		other.object = NULL;
	}

	~WeakObjectRefT() {
		T* old = object;
		object = NULL;
		Object_weak_unref(old);
	}

	WeakObjectRefT& operator=(T* obj) {
		Object_weak_ref(obj);
		T* old = object;
		object = obj;
		Object_weak_unref(old);
		return *this;
	}

	template<typename U>
	WeakObjectRefT& operator=(const ObjectRefT<U>& other) {
		T* obj = other;
		Object_weak_ref(obj);
		T* old = object;
		object = obj;
		Object_weak_unref(old);
		return *this;
	}

	WeakObjectRefT& operator=(const WeakObjectRefT& other) {
		Object_weak_ref(other.object);
		T* old = object;
		object = other.object;
		Object_weak_unref(old);
		return *this;
	}

	template<typename U>
	WeakObjectRefT& operator=(const WeakObjectRefT<U>& other) {
		Object_weak_ref(other.object);
		T* old = object;
		object = other.object;
		Object_weak_unref(old);
		return *this;
	}

	WeakObjectRefT& operator=(WeakObjectRefT&& other) {
		if (this != &other) {
			T* old = object;
			object = other.object;
			other.object = NULL;
			Object_weak_unref(old);
		}
		return *this;
	}

	template<typename U>
	WeakObjectRefT& operator=(WeakObjectRefT<U>&& other) {
		T* old = object;
		object = other.object;
		other.object = NULL;
		Object_weak_unref(old);
		return *this;
	}

	/** Attempts to obtain a strong reference from this weak reference.
	Returns an ObjectRefT holding the strong reference, or an empty ObjectRefT if the object has been freed.
	*/
	ObjectRefT<T> lock() const {
		if (Object_weak_lock(object))
			return ObjectRefT<T>(object);
		return ObjectRefT<T>();
	}

	operator T*() const { return object; }
};

using WeakObjectRef = WeakObjectRefT<Object>;
using ConstWeakObjectRef = WeakObjectRefT<const Object>;
