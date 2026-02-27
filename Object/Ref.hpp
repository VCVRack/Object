#pragma once

#include "Object.h"


/** Holds a strong reference to an Object using C++ RAII.
T can be `Object` or `const Object`.
*/
template<typename T = Object>
struct RefT {
	T* object = NULL;

	RefT() = default;

	/** Adopts an Object, transferring ownership from the caller.
	Does not obtain a new reference.
	*/
	RefT(T* object) : object(object) {}

	/** Obtains a new reference of the Object from another RefT.
	Both RefTs then own their own reference.
	*/
	RefT(const RefT& other) : object(other.object) {
		Object_ref(object);
	}

	/** Obtains a new reference of the Object from another compatible RefT.
	Allows implicit conversion from Ref to ConstRef.
	*/
	template<typename U>
	RefT(const RefT<U>& other) : object(other.object) {
		Object_ref(object);
	}

	/** Moves a reference from another RefT. */
	RefT(RefT&& other) : object(other.object) {
		other.object = NULL;
	}

	/** Moves a reference from another compatible RefT. */
	template<typename U>
	RefT(RefT<U>&& other) : object(other.object) {
		other.object = NULL;
	}

	~RefT() {
		T* old = object;
		object = NULL;
		Object_unref(old);
	}

	/** Adopts an Object, transferring ownership from the caller.
	Does not obtain a new reference.
	*/
	RefT& operator=(T* object) {
		T* old = this->object;
		this->object = object;
		Object_unref(old);
		return *this;
	}

	RefT& operator=(const RefT& other) {
		Object_ref(other.object);
		T* old = object;
		object = other.object;
		Object_unref(old);
		return *this;
	}

	template<typename U>
	RefT& operator=(const RefT<U>& other) {
		Object_ref(other.object);
		T* old = object;
		object = other.object;
		Object_unref(old);
		return *this;
	}

	RefT& operator=(RefT&& other) {
		if (this != &other) {
			T* old = object;
			object = other.object;
			other.object = NULL;
			Object_unref(old);
		}
		return *this;
	}

	template<typename U>
	RefT& operator=(RefT<U>&& other) {
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
	static RefT obtain(T* object) {
		Object_ref(object);
		RefT ref;
		ref.object = object;
		return ref;
	}
};

using Ref = RefT<Object>;
using ConstRef = RefT<const Object>;


/** Holds a weak reference to an Object using C++ RAII.
A weak reference does not prevent the object from being freed, but can be converted to a strong Ref if still alive with lock().
T can be `Object` or `const Object`.
*/
template<typename T = Object>
struct WeakRefT {
	T* object = NULL;

	WeakRefT() = default;

	/** Obtains a weak reference of an Object. */
	WeakRefT(T* object) : object(object) {
		Object_weak_ref(object);
	}

	/** Obtains a new weak reference of the Object from an Ref. */
	template<typename U>
	WeakRefT(const RefT<U>& other) : object(other.object) {
		Object_weak_ref(object);
	}

	/** Obtains a new weak reference of the Object from another WeakRefT. */
	WeakRefT(const WeakRefT& other) : object(other.object) {
		Object_weak_ref(object);
	}

	/** Obtains a new weak reference of the Object from another compatible WeakRefT.
	Allows implicit conversion from WeakRef to ConstWeakRef.
	*/
	template<typename U>
	WeakRefT(const WeakRefT<U>& other) : object(other.object) {
		Object_weak_ref(object);
	}

	/** Moves a weak reference from another WeakRefT. */
	WeakRefT(WeakRefT&& other) : object(other.object) {
		other.object = NULL;
	}

	/** Moves a weak reference from another compatible WeakRefT. */
	template<typename U>
	WeakRefT(WeakRefT<U>&& other) : object(other.object) {
		other.object = NULL;
	}

	~WeakRefT() {
		T* old = object;
		object = NULL;
		Object_weak_unref(old);
	}

	WeakRefT& operator=(T* object) {
		Object_weak_ref(object);
		T* old = this->object;
		this->object = object;
		Object_weak_unref(old);
		return *this;
	}

	template<typename U>
	WeakRefT& operator=(const RefT<U>& other) {
		T* object = other;
		Object_weak_ref(object);
		T* old = this->object;
		this->object = object;
		Object_weak_unref(old);
		return *this;
	}

	WeakRefT& operator=(const WeakRefT& other) {
		Object_weak_ref(other.object);
		T* old = object;
		object = other.object;
		Object_weak_unref(old);
		return *this;
	}

	template<typename U>
	WeakRefT& operator=(const WeakRefT<U>& other) {
		Object_weak_ref(other.object);
		T* old = object;
		object = other.object;
		Object_weak_unref(old);
		return *this;
	}

	WeakRefT& operator=(WeakRefT&& other) {
		if (this != &other) {
			T* old = object;
			object = other.object;
			other.object = NULL;
			Object_weak_unref(old);
		}
		return *this;
	}

	template<typename U>
	WeakRefT& operator=(WeakRefT<U>&& other) {
		T* old = object;
		object = other.object;
		other.object = NULL;
		Object_weak_unref(old);
		return *this;
	}

	/** Attempts to obtain a strong reference from this weak reference.
	Returns a RefT holding the strong reference, or an empty RefT if the object has been freed.
	*/
	RefT<T> lock() const {
		if (!Object_weak_lock(object))
			return RefT<T>();
		return RefT<T>(object);
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

using WeakRef = WeakRefT<Object>;
using ConstWeakRef = WeakRefT<const Object>;
