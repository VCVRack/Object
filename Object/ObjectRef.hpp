#pragma once

#include "Object.h"


/** Holds a strong reference to an Object using C++ RAII.
T can be `Object` or `const Object`.
*/
template<typename T = Object>
struct ObjectRefT {
	T* object = NULL;

	ObjectRefT() = default;

	/** Adopts an Object, transferring ownership from the caller.
	Does not obtain a new reference.
	*/
	ObjectRefT(T* object) : object(object) {}

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

	/** Adopts an Object, transferring ownership from the caller.
	Does not obtain a new reference.
	*/
	ObjectRefT& operator=(T* object) {
		T* old = this->object;
		this->object = object;
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

	/** Implicit cast to Object* */
	operator T*() const { return object; }

	/** Releases the Object and transfers ownership to the caller.
	*/
	T* release() {
		T* object = this->object;
		this->object = NULL;
		return object;
	}

	/** Returns the Object with a new reference.
	*/
	T* share() const {
		Object_ref(object);
		return object;
	}

	/** Obtains a new reference of an Object.
	*/
	static ObjectRefT obtain(T* object) {
		Object_ref(object);
		ObjectRefT ref;
		ref.object = object;
		return ref;
	}
};

using ObjectRef = ObjectRefT<Object>;
using ConstObjectRef = ObjectRefT<const Object>;


/** Holds a weak reference to an Object using C++ RAII.
A weak reference does not prevent the object from being freed, but can be converted to a strong ObjectRef if still alive with lock().
T can be `Object` or `const Object`.
*/
template<typename T = Object>
struct WeakObjectRefT {
	T* object = NULL;

	WeakObjectRefT() = default;

	/** Obtains a weak reference of an Object. */
	WeakObjectRefT(T* object) : object(object) {
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

	WeakObjectRefT& operator=(T* object) {
		Object_weak_ref(object);
		T* old = this->object;
		this->object = object;
		Object_weak_unref(old);
		return *this;
	}

	template<typename U>
	WeakObjectRefT& operator=(const ObjectRefT<U>& other) {
		T* object = other;
		Object_weak_ref(object);
		T* old = this->object;
		this->object = object;
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
		if (!Object_weak_lock(object))
			return ObjectRefT<T>();
		return ObjectRefT<T>(object);
	}

	/** Attempts to obtain a strong reference and transfers it to the caller.
	Returns NULL if the Object has been freed.
	*/
	T* share() const {
		if (!Object_weak_lock(object))
			return NULL;
		return object;
	}

	operator T*() const { return object; }
};

using WeakObjectRef = WeakObjectRefT<Object>;
using ConstWeakObjectRef = WeakObjectRefT<const Object>;
