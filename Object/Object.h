#pragma once

#include <stdint.h> // for int32_t, uint64_t, etc.
#include <stdbool.h> // for bool, true, false
#include <stddef.h> // for NULL


#ifdef __cplusplus
	#define EXTERNC extern "C"
	#define EXTERNC_BEGIN extern "C" {
	#define EXTERNC_END }
#else
	#define EXTERNC
	#define EXTERNC_BEGIN
	#define EXTERNC_END
#endif


/** Converts `EXPAND (1, 2, 3)` to `1, 2, 3`
and `EXPAND ()` to ``.
*/
#define EXPAND(...) __VA_ARGS__

/** Converts `0 COMMA_EXPAND (1, 2, 3)` to `0, 1, 2, 3`
and `0 COMMA_EXPAND ()` to `0`
*/
#define COMMA_EXPAND(...) __VA_OPT__(,) __VA_ARGS__

/** Represents the "value" of a void return type.
Example:
	void f() {
		return VOID;
	}

Example:
	DEFINE_METHOD(Class, Method, void, (), VOID, {})
*/
#define VOID


/**************************************
Declaration macros for public headers
*/


/** Declares a free function (not tied to an Object class).

Example:
	FUNCTION(zoo, init, void, ())

Declares:
	void zoo_init();
*/
#define FUNCTION(PREFIX, NAME, RETTYPE, ARGTYPES) \
	EXTERNC RETTYPE PREFIX##_##NAME(EXPAND ARGTYPES)


/** Declares a global getter function.

Example:
	GLOBAL_GETTER(zoo, temperature, float)

Declares:
	float zoo_temperature_get();
*/
#define GLOBAL_GETTER(PREFIX, NAME, TYPE) \
	FUNCTION(PREFIX, NAME##_get, TYPE, ())


/** Declares a global setter function.

Example:
	GLOBAL_SETTER(zoo, temperature, float)

Declares:
	void zoo_temperature_set(float temperature);
*/
#define GLOBAL_SETTER(PREFIX, NAME, TYPE) \
	FUNCTION(PREFIX, NAME##_set, void, (TYPE NAME))


/** Declares a global getter/setter function pair.

Example:
	GLOBAL_ACCESSOR(zoo, temperature, float)

Declares:
	float zoo_temperature_get();
	void zoo_temperature_set(float temperature);
*/
#define GLOBAL_ACCESSOR(PREFIX, NAME, TYPE) \
	GLOBAL_GETTER(PREFIX, NAME, TYPE); \
	GLOBAL_SETTER(PREFIX, NAME, TYPE)


/** Declares a class Class struct and constructor function.

CLASS is the class name such as `Animal`.
INITARGS are the arguments of the create/specialize functions such as `(const char* name, int32_t legs)`.

Superclasses are defined in your implementation, not in your header.

Example:
	CLASS(Animal, (const char* name, int32_t legs))

Declares the following functions.

Constructor:
	Object* Animal_create(const char* name, int32_t legs);

Specialization function:
	void Animal_specialize(Object* self, const char* name, int32_t legs);
*/
#define CLASS(CLASS, INITARGS) \
	extern const Class CLASS##_class; \
	EXTERNC Object* CLASS##_create(EXPAND INITARGS); \
	EXTERNC void CLASS##_specialize(Object* self COMMA_EXPAND INITARGS)


/** Declares a non-virtual method for a class.
This method cannot be overridden by specialized classes (subclasses).

CLASS is the class name such as `Animal`.
METHOD is the method name such as `speak`.
RETTYPE is the return value such as `void`.
ARGTYPES are the arguments such as `(const char* name, int32_t loudness)`.

Example:
	METHOD(Animal, speak, void, (const char* name))

Declares the function:
	void Animal_speak(Object* self, const char* name);
*/
#define METHOD(CLASS, METHOD, RETTYPE, ARGTYPES) \
	EXTERNC RETTYPE CLASS##_##METHOD(Object* self COMMA_EXPAND ARGTYPES)


/** Interfaces are virtual methods with no implementation.
*/
#define METHOD_INTERFACE(CLASS, METHOD_, RETTYPE, ARGTYPES) \
	typedef RETTYPE CLASS##_##METHOD_##_m(Object* self COMMA_EXPAND ARGTYPES); \
	METHOD(CLASS, METHOD_, RETTYPE, ARGTYPES)


/** Declares a method that overrides a different class's virtual method.
*/
#define METHOD_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES) \
	EXTERNC RETTYPE CLASS##_##METHOD##_mdirect(Object* self COMMA_EXPAND ARGTYPES)


/** Declares a virtual method for a class.

CLASS is the class name such as `Animal`.
METHOD is the method name such as `speak`.
RETTYPE is the return value such as `void`.
ARGTYPES are the arguments such as `(const char* name, int32_t loudness)`.

Example:
	METHOD_VIRTUAL(Animal, speak, void, (const char* name))

Declares the following.

Method function type:
	typedef void Animal_speak_m(Object* self, const char* name);

Virtual dispatch call:
	void Animal_speak(Object* self, const char* name);

Non-virtual (direct) call:
	void Animal_speak_mdirect(Object* self, const char* name);
*/
#define METHOD_VIRTUAL(CLASS, METHOD, RETTYPE, ARGTYPES) \
	METHOD_INTERFACE(CLASS, METHOD, RETTYPE, ARGTYPES); \
	METHOD_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES)


/** Declares a non-virtual method for a class with a `const Object*` argument.
*/
#define METHOD_CONST(CLASS, METHOD, RETTYPE, ARGTYPES) \
	EXTERNC RETTYPE CLASS##_##METHOD(const Object* self COMMA_EXPAND ARGTYPES)


#define METHOD_CONST_INTERFACE(CLASS, METHOD, RETTYPE, ARGTYPES) \
	typedef RETTYPE CLASS##_##METHOD##_m(const Object* self COMMA_EXPAND ARGTYPES); \
	METHOD_CONST(CLASS, METHOD, RETTYPE, ARGTYPES)


#define METHOD_CONST_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES) \
	EXTERNC RETTYPE CLASS##_##METHOD##_mdirect(const Object* self COMMA_EXPAND ARGTYPES)


#define METHOD_CONST_VIRTUAL(CLASS, METHOD, RETTYPE, ARGTYPES) \
	METHOD_CONST_INTERFACE(CLASS, METHOD, RETTYPE, ARGTYPES); \
	METHOD_CONST_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES)


/** Declares a non-virtual getter method for a class.

CLASS is the class name such as `Animal`.
PROP is the property name such as `name`.
TYPE is the type of the property such as `const char*`.

Example:
	GETTER(Animal, name, const char*)

Declares the function:
	const char* Animal_name_get(const Object* self);
*/
#define GETTER(CLASS, PROP, TYPE) \
	METHOD_CONST(CLASS, PROP##_get, TYPE, ())


#define GETTER_INTERFACE(CLASS, PROP, TYPE) \
	METHOD_CONST_INTERFACE(CLASS, PROP##_get, TYPE, ())


/** Declares a getter method that overrides a different class's virtual getter.
*/
#define GETTER_OVERRIDE(CLASS, PROP, TYPE) \
	METHOD_CONST_OVERRIDE(CLASS, PROP##_get, TYPE, ())


/** Declares a virtual getter method for a class.
Same arguments as GETTER().

Example:
	GETTER_VIRTUAL(Animal, name, const char*)

Declares the functions:
	const char* Animal_name_get(const Object* self);
	const char* Animal_name_get_mdirect(const Object* self);
*/
#define GETTER_VIRTUAL(CLASS, PROP, TYPE) \
	METHOD_CONST_VIRTUAL(CLASS, PROP##_get, TYPE, ())


/** Declares a non-virtual setter method for a class.
Same arguments as GETTER().

Example:
	SETTER(Animal, name, const char*)

Declares the function:
	void Animal_name_set(Object* self, const char* name);
*/
#define SETTER(CLASS, PROP, TYPE) \
	METHOD(CLASS, PROP##_set, void, (TYPE PROP))


#define SETTER_INTERFACE(CLASS, PROP, TYPE) \
	METHOD_INTERFACE(CLASS, PROP##_set, void, (TYPE PROP))


#define SETTER_OVERRIDE(CLASS, PROP, TYPE) \
	METHOD_OVERRIDE(CLASS, PROP##_set, void, (TYPE PROP))


/** Declares a virtual setter method for a class.
Same arguments as GETTER().

Example:
	SETTER_VIRTUAL(Animal, name, const char*)

Declares the functions:
	void Animal_name_set(Object* self, const char* name);
	void Animal_name_set_mdirect(Object* self, const char* name);
*/
#define SETTER_VIRTUAL(CLASS, PROP, TYPE) \
	METHOD_VIRTUAL(CLASS, PROP##_set, void, (TYPE PROP))


/** Declares a non-virtual getter/setter method pair for a class.
Same arguments as GETTER() and SETTER().

Example:
	ACCESSOR(Animal, name, const char*)
*/
#define ACCESSOR(CLASS, PROP, TYPE) \
	GETTER(CLASS, PROP, TYPE); \
	SETTER(CLASS, PROP, TYPE)


#define ACCESSOR_INTERFACE(CLASS, PROP, TYPE) \
	GETTER_INTERFACE(CLASS, PROP, TYPE); \
	SETTER_INTERFACE(CLASS, PROP, TYPE)


#define ACCESSOR_OVERRIDE(CLASS, PROP, TYPE) \
	GETTER_OVERRIDE(CLASS, PROP, TYPE); \
	SETTER_OVERRIDE(CLASS, PROP, TYPE)


#define ACCESSOR_VIRTUAL(CLASS, PROP, TYPE) \
	GETTER_VIRTUAL(CLASS, PROP, TYPE); \
	SETTER_VIRTUAL(CLASS, PROP, TYPE)


/** Declares a non-virtual indexed getter method for a class.
PROP should be a plural noun, since it names an array.

Example:
	ARRAY_GETTER(Animal, children, Object*)

Declares the functions:
	uint64_t Animal_children_count_get(const Object* self);
	Object* Animal_children_get(const Object* self, uint64_t index);
*/
#define ARRAY_GETTER(CLASS, PROP, TYPE) \
	GETTER(CLASS, PROP##_count, uint64_t); \
	METHOD_CONST(CLASS, PROP##_get, TYPE, (uint64_t index))


/** Declares a non-virtual indexed getter/setter method pair for a class.
PROP should be a plural noun, since it names an array.

Example:
	ARRAY_ACCESSOR(Animal, children, Object*)

Declares the functions:
	uint64_t Animal_children_count_get(const Object* self);
	Object* Animal_children_get(const Object* self, uint64_t index);
	void Animal_children_set(Object* self, uint64_t index, Object* element);
*/
#define ARRAY_ACCESSOR(CLASS, PROP, TYPE) \
	ARRAY_GETTER(CLASS, PROP, TYPE); \
	METHOD(CLASS, PROP##_set, void, (uint64_t index, TYPE element))


/** Declares a non-virtual indexed resizable getter/setter method pair for a class.
PROP should be a plural noun, since it names an array.

Example:
	VECTOR_ACCESSOR(Animal, children, Object*)

Declares the functions:
	uint64_t Animal_children_count_get(const Object* self);
	void Animal_children_count_set(Object* self, uint64_t children_count);
	Object* Animal_children_get(const Object* self, uint64_t index);
	void Animal_children_set(Object* self, uint64_t index, Object* element);

*/
#define VECTOR_ACCESSOR(CLASS, PROP, TYPE) \
	ARRAY_ACCESSOR(CLASS, PROP, TYPE); \
	SETTER(CLASS, PROP##_count, uint64_t)


/**************************************
Definition macros for source implementation files
*/


#define DEFINE_CLASS_FUNCTIONS(CLASS, INITARGS, INITARGNAMES, ...) \
	EXTERNC void CLASS##_specialize(Object* self COMMA_EXPAND INITARGS) { \
		if (!self) \
			return; \
		if (Object_data_get(self, &CLASS##_class)) \
			return; \
		__VA_ARGS__ \
	} \
	EXTERNC Object* CLASS##_create(EXPAND INITARGS) { \
		Object* self = Object_create(); \
		CLASS##_specialize(self COMMA_EXPAND INITARGNAMES); \
		return self; \
	}


#define DEFINE_CLASS_FREE(CLASS, ...) \
	static void CLASS##_free(Object* self) { \
		if (!self) \
			return; \
		CLASS* data = (CLASS*) Object_data_get(self, &CLASS##_class); \
		if (!data) \
			return; \
		__VA_ARGS__ \
	}


#define DEFINE_CLASS(CLASS, INITARGS, INITARGNAMES, INIT, ...) \
	extern const Class CLASS##_class; \
	typedef struct CLASS CLASS; \
	DEFINE_CLASS_FUNCTIONS(CLASS, INITARGS, INITARGNAMES, INIT) \
	DEFINE_CLASS_FREE(CLASS, __VA_ARGS__) \
	const Class CLASS##_class = { \
		#CLASS, \
		CLASS##_free, \
		{} \
	};


/** A class's non-virtual methods cannot be overridden.
Upgrading a non-virtual method to a virtual method creates linker symbols and does not break the ABI.
Downgrading a virtual method to a non-virtual method removes linker symbols and therefore breaks the ABI.
*/
#define DEFINE_METHOD(CLASS, METHOD, RETTYPE, ARGTYPES, RETDEFAULT, ...) \
	EXTERNC RETTYPE CLASS##_##METHOD(Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data = (CLASS*) Object_data_get(self, &CLASS##_class); \
		if (!data) \
			return RETDEFAULT; \
		__VA_ARGS__ \
	}


#define DEFINE_METHOD_INTERFACE(CLASS, METHOD, RETTYPE, ARGTYPES, RETDEFAULT, ARGNAMES) \
	typedef RETTYPE CLASS##_##METHOD##_m(Object* self COMMA_EXPAND ARGTYPES); \
	EXTERNC RETTYPE CLASS##_##METHOD(Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS##_##METHOD##_m* m = (CLASS##_##METHOD##_m*) Object_method_get(self, (void*) &CLASS##_##METHOD); \
		if (!m) \
			return RETDEFAULT; \
		return m(self COMMA_EXPAND ARGNAMES); \
	}


#define DEFINE_METHOD_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES, RETDEFAULT, ...) \
	DEFINE_METHOD(CLASS, METHOD##_mdirect, RETTYPE, ARGTYPES, RETDEFAULT, __VA_ARGS__)


#define DEFINE_METHOD_VIRTUAL(CLASS, METHOD, RETTYPE, ARGTYPES, RETDEFAULT, ARGNAMES, ...) \
	DEFINE_METHOD_INTERFACE(CLASS, METHOD, RETTYPE, ARGTYPES, RETDEFAULT, ARGNAMES) \
	DEFINE_METHOD_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES, RETDEFAULT, __VA_ARGS__)


#define DEFINE_METHOD_CONST(CLASS, METHOD, RETTYPE, ARGTYPES, RETDEFAULT, ...) \
	EXTERNC RETTYPE CLASS##_##METHOD(const Object* self COMMA_EXPAND ARGTYPES) { \
		const CLASS* data = (const CLASS*) Object_data_get(self, &CLASS##_class); \
		if (!data) \
			return RETDEFAULT; \
		__VA_ARGS__ \
	}


#define DEFINE_METHOD_CONST_INTERFACE(CLASS, METHOD, RETTYPE, ARGTYPES, RETDEFAULT, ARGNAMES) \
	typedef RETTYPE CLASS##_##METHOD##_m(const Object* self COMMA_EXPAND ARGTYPES); \
	EXTERNC RETTYPE CLASS##_##METHOD(const Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS##_##METHOD##_m* m = (CLASS##_##METHOD##_m*) Object_method_get(self, (void*) &CLASS##_##METHOD); \
		if (!m) \
			return RETDEFAULT; \
		return m(self COMMA_EXPAND ARGNAMES); \
	}


#define DEFINE_METHOD_CONST_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES, RETDEFAULT, ...) \
	DEFINE_METHOD_CONST(CLASS, METHOD##_mdirect, RETTYPE, ARGTYPES, RETDEFAULT, __VA_ARGS__)


#define DEFINE_METHOD_CONST_VIRTUAL(CLASS, METHOD, RETTYPE, ARGTYPES, RETDEFAULT, ARGNAMES, ...) \
	DEFINE_METHOD_CONST_INTERFACE(CLASS, METHOD, RETTYPE, ARGTYPES, RETDEFAULT, ARGNAMES) \
	DEFINE_METHOD_CONST_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES, RETDEFAULT, __VA_ARGS__)


#define DEFINE_GETTER(CLASS, PROP, TYPE, DEFAULT, ...) \
	DEFINE_METHOD_CONST(CLASS, PROP##_get, TYPE, (), DEFAULT, __VA_ARGS__)


#define DEFINE_GETTER_AUTOMATIC(CLASS, PROP, TYPE, DEFAULT) \
	DEFINE_GETTER(CLASS, PROP, TYPE, DEFAULT, { \
		return data->PROP; \
	})


#define DEFINE_GETTER_INTERFACE(CLASS, PROP, TYPE, DEFAULT) \
	DEFINE_METHOD_CONST_INTERFACE(CLASS, PROP##_get, TYPE, (), DEFAULT, ())


#define DEFINE_GETTER_OVERRIDE(CLASS, PROP, TYPE, DEFAULT, ...) \
	DEFINE_METHOD_CONST_OVERRIDE(CLASS, PROP##_get, TYPE, (), DEFAULT, __VA_ARGS__)


#define DEFINE_GETTER_VIRTUAL(CLASS, PROP, TYPE, DEFAULT, ...) \
	DEFINE_METHOD_CONST_VIRTUAL(CLASS, PROP##_get, TYPE, (), DEFAULT, (), __VA_ARGS__)


/** Defines a getter method that returns the property from the data struct.
*/
#define DEFINE_GETTER_VIRTUAL_AUTOMATIC(CLASS, PROP, TYPE, DEFAULT) \
	DEFINE_GETTER_VIRTUAL(CLASS, PROP, TYPE, DEFAULT, { \
		return data->PROP; \
	})


#define DEFINE_SETTER(CLASS, PROP, TYPE, ...) \
	DEFINE_METHOD(CLASS, PROP##_set, void, (TYPE PROP), VOID, __VA_ARGS__)


#define DEFINE_SETTER_AUTOMATIC(CLASS, PROP, TYPE) \
	DEFINE_SETTER(CLASS, PROP, TYPE, { \
		data->PROP = PROP; \
	})


#define DEFINE_SETTER_INTERFACE(CLASS, PROP, TYPE) \
	DEFINE_METHOD_INTERFACE(CLASS, PROP##_set, void, (TYPE PROP), VOID, (PROP))


#define DEFINE_SETTER_OVERRIDE(CLASS, PROP, TYPE, ...) \
	DEFINE_METHOD_OVERRIDE(CLASS, PROP##_set, void, (TYPE PROP), VOID, __VA_ARGS__)


#define DEFINE_SETTER_VIRTUAL(CLASS, PROP, TYPE, ...) \
	DEFINE_METHOD_VIRTUAL(CLASS, PROP##_set, void, (TYPE PROP), VOID, (PROP), __VA_ARGS__)


/** Defines a setter method that sets the property to the data struct.
*/
#define DEFINE_SETTER_VIRTUAL_AUTOMATIC(CLASS, PROP, TYPE) \
	DEFINE_SETTER_VIRTUAL(CLASS, PROP, TYPE, { \
		data->PROP = PROP; \
	})


#define DEFINE_ACCESSOR(CLASS, PROP, TYPE, DEFAULT, GETTER, ...) \
	DEFINE_GETTER(CLASS, PROP, TYPE, DEFAULT, GETTER) \
	DEFINE_SETTER(CLASS, PROP, TYPE, __VA_ARGS__)


#define DEFINE_ACCESSOR_AUTOMATIC(CLASS, PROP, TYPE, DEFAULT) \
	DEFINE_GETTER_AUTOMATIC(CLASS, PROP, TYPE, DEFAULT) \
	DEFINE_SETTER_AUTOMATIC(CLASS, PROP, TYPE)


#define DEFINE_ACCESSOR_INTERFACE(CLASS, PROP, TYPE, DEFAULT) \
	DEFINE_GETTER_INTERFACE(CLASS, PROP, TYPE, DEFAULT) \
	DEFINE_SETTER_INTERFACE(CLASS, PROP, TYPE)


#define DEFINE_ACCESSOR_OVERRIDE(CLASS, PROP, TYPE, DEFAULT, GETTER, ...) \
	DEFINE_GETTER_OVERRIDE(CLASS, PROP, TYPE, DEFAULT, GETTER) \
	DEFINE_SETTER_OVERRIDE(CLASS, PROP, TYPE, __VA_ARGS__)


#define DEFINE_ACCESSOR_VIRTUAL(CLASS, PROP, TYPE, DEFAULT, GETTER, ...) \
	DEFINE_GETTER_VIRTUAL(CLASS, PROP, TYPE, DEFAULT, GETTER) \
	DEFINE_SETTER_VIRTUAL(CLASS, PROP, TYPE, __VA_ARGS__)


#define DEFINE_ACCESSOR_VIRTUAL_AUTOMATIC(CLASS, PROP, TYPE, DEFAULT) \
	DEFINE_GETTER_VIRTUAL_AUTOMATIC(CLASS, PROP, TYPE, DEFAULT) \
	DEFINE_SETTER_VIRTUAL_AUTOMATIC(CLASS, PROP, TYPE)


#define DEFINE_ARRAY_GETTER(CLASS, PROP, TYPE, DEFAULT, COUNTGETTER, ...) \
	DEFINE_GETTER(CLASS, PROP##_count, uint64_t, 0, COUNTGETTER) \
	DEFINE_METHOD_CONST(CLASS, PROP##_get, TYPE, (uint64_t index), DEFAULT, __VA_ARGS__)


#define DEFINE_ARRAY_ACCESSOR(CLASS, PROP, TYPE, DEFAULT, COUNTGETTER, GETTER, ...) \
	DEFINE_ARRAY_GETTER(CLASS, PROP, TYPE, DEFAULT, COUNTGETTER, GETTER) \
	DEFINE_METHOD(CLASS, PROP##_set, void, (uint64_t index, TYPE element), VOID, __VA_ARGS__)


#define DEFINE_VECTOR_ACCESSOR(CLASS, PROP, TYPE, DEFAULT, COUNTGETTER, COUNTSETTER, GETTER, ...) \
	DEFINE_ARRAY_ACCESSOR(CLASS, PROP, TYPE, DEFAULT, COUNTGETTER, GETTER, __VA_ARGS__) \
	DEFINE_SETTER(CLASS, PROP##_count, uint64_t, COUNTSETTER)


/** Defines a free function (not tied to an Object class).

Example:
	DEFINE_FUNCTION(zoo, init, void, (), {
		...
	})

Defines:
	void zoo_init() { ... }
*/
#define DEFINE_FUNCTION(PREFIX, NAME, RETTYPE, ARGTYPES, ...) \
	EXTERNC RETTYPE PREFIX##_##NAME(EXPAND ARGTYPES) { \
		__VA_ARGS__ \
	}


#define DEFINE_GLOBAL_GETTER(PREFIX, NAME, TYPE, ...) \
	DEFINE_FUNCTION(PREFIX, NAME##_get, TYPE, (), __VA_ARGS__)


#define DEFINE_GLOBAL_GETTER_AUTOMATIC(PREFIX, NAME, TYPE) \
	DEFINE_GLOBAL_GETTER(PREFIX, NAME, TYPE, { \
		return NAME; \
	})


#define DEFINE_GLOBAL_SETTER(PREFIX, NAME, TYPE, ...) \
	DEFINE_FUNCTION(PREFIX, NAME##_set, void, (TYPE NAME), __VA_ARGS__)


#define DEFINE_GLOBAL_SETTER_AUTOMATIC(PREFIX, NAME, TYPE) \
	EXTERNC void PREFIX##_##NAME##_set(TYPE NAME##_) { \
		NAME = NAME##_; \
	}


#define DEFINE_GLOBAL_ACCESSOR(PREFIX, NAME, TYPE, GETTER, ...) \
	DEFINE_GLOBAL_GETTER(PREFIX, NAME, TYPE, GETTER) \
	DEFINE_GLOBAL_SETTER(PREFIX, NAME, TYPE, __VA_ARGS__)


#define DEFINE_GLOBAL_ACCESSOR_AUTOMATIC(PREFIX, NAME, TYPE) \
	DEFINE_GLOBAL_GETTER_AUTOMATIC(PREFIX, NAME, TYPE) \
	DEFINE_GLOBAL_SETTER_AUTOMATIC(PREFIX, NAME, TYPE)


/**************************************
Call macros
*/


#define GLOBAL_CALL(PREFIX, NAME, ...) \
	PREFIX##_##NAME(__VA_ARGS__)


#define GLOBAL_GET(PREFIX, NAME, ...) \
	PREFIX##_##NAME##_get(__VA_ARGS__)


#define GLOBAL_SET(PREFIX, NAME, ...) \
	PREFIX##_##NAME##_set(__VA_ARGS__)


#define CREATE(CLASS, ...) \
	CLASS##_create(__VA_ARGS__)


#define SPECIALIZE(SELF, CLASS, ...) \
	CLASS##_specialize(SELF __VA_OPT__(,) __VA_ARGS__)


#define CLASS_PUSH(SELF, CLASS, DATA) \
	Object_class_push(SELF, &CLASS##_class, DATA)


#define DATA_GET(SELF, CLASS) \
	((CLASS*) Object_data_get(SELF, &CLASS##_class))


#define IS(SELF, CLASS) \
	(Object_data_get(SELF, &CLASS##_class) != NULL)


#define METHOD_PUSH(SELF, SUPERCLASS, CLASS, METHOD) \
	Object_method_push(SELF, (void*) &SUPERCLASS##_##METHOD, (void*) &CLASS##_##METHOD##_mdirect)


#define GETTER_PUSH(SELF, SUPERCLASS, CLASS, PROP) \
	METHOD_PUSH(SELF, SUPERCLASS, CLASS, PROP##_get)


#define SETTER_PUSH(SELF, SUPERCLASS, CLASS, PROP) \
	METHOD_PUSH(SELF, SUPERCLASS, CLASS, PROP##_set)


#define ACCESSOR_PUSH(SELF, SUPERCLASS, CLASS, PROP) \
	GETTER_PUSH(SELF, SUPERCLASS, CLASS, PROP); \
	SETTER_PUSH(SELF, SUPERCLASS, CLASS, PROP)


#define CALL(SELF, CLASS, METHOD, ...) \
	CLASS##_##METHOD(SELF __VA_OPT__(,) __VA_ARGS__)


#define DIRECT_CALL(SELF, CLASS, METHOD, ...) \
	CLASS##_##METHOD##_mdirect(SELF __VA_OPT__(,) __VA_ARGS__)


#define SUPER_CALL(SELF, SUPERCLASS, CLASS, METHOD, ...) \
	((SUPERCLASS##_##METHOD##_m*) Object_supermethod_get(SELF, (void*) &CLASS##_##METHOD##_mdirect))(SELF __VA_OPT__(,) __VA_ARGS__)


#define GET(SELF, CLASS, PROP, ...) \
	CLASS##_##PROP##_get(SELF __VA_OPT__(,) __VA_ARGS__)


#define DIRECT_GET(SELF, CLASS, PROP, ...) \
	CLASS##_##PROP##_get_mdirect(SELF __VA_OPT__(,) __VA_ARGS__)


#define SUPER_GET(SELF, SUPERCLASS, CLASS, PROP, ...) \
	SUPER_CALL(SELF, SUPERCLASS, CLASS, PROP##_get __VA_OPT__(,) __VA_ARGS__)


#define SET(SELF, CLASS, PROP, ...) \
	CLASS##_##PROP##_set(SELF __VA_OPT__(,) __VA_ARGS__)


#define DIRECT_SET(SELF, CLASS, PROP, ...) \
	CLASS##_##PROP##_set_mdirect(SELF __VA_OPT__(,) __VA_ARGS__)


#define SUPER_SET(SELF, SUPERCLASS, CLASS, PROP, ...) \
	SUPER_CALL(SELF, SUPERCLASS, CLASS, PROP##_set __VA_OPT__(,) __VA_ARGS__)


/**************************************
Runtime symbols
*/


EXTERNC_BEGIN


/** The type of all polymorphic objects in this API. */
typedef struct Object Object;

typedef void Object_free_m(Object* self);

typedef struct Class {
	const char* name;
	/** Frees data pointer and its contents.
	May be NULL if the class has no data to free.
	*/
	Object_free_m* free;
	/** Reserved for future fields.
	Must be zero.
	*/
	void* reserved[30];
} Class;


/** Creates an object with no classes.
Reference count is set to 1.
Object must be unreferenced with Object_unref() to prevent a memory leak.
Never returns NULL.
*/
__attribute__((warn_unused_result))
Object* Object_create(void);


/** Increments the object's reference counter.
Use this to share another reference to this object.
Each reference must be unreferenced with Object_unref() to prevent a memory leak.
Thread-safe.
Does nothing if self is NULL.
*/
__attribute__((hot))
void Object_ref(const Object* self);


/** Decrements the object's reference counter.
If no references are left, this frees the object and its internal data for each class.
Object should be considered invalid after calling this function.
Thread-safe.
Does nothing if self is NULL.
*/
__attribute__((hot))
void Object_unref(const Object* self);


/** Returns the number of strong references to the object.
Returns 0 if self is NULL.
Thread-safe.
*/
uint32_t Object_refs_get(const Object* self);


/** Increments the object's weak reference counter.
A weak reference does not prevent the object from being freed, but it keeps the object pointer valid so that Object_weak_lock() can be safely called.
Each weak reference must be unreferenced with Object_weak_unref().
Thread-safe.
Does nothing if self is NULL.
*/
void Object_weak_ref(const Object* self);


/** Decrements the object's weak reference counter.
If no strong or weak references are left, this frees the object shell.
Thread-safe.
Does nothing if self is NULL.
*/
void Object_weak_unref(const Object* self);


/** Returns the number of weak references to the object.
Returns 0 if self is NULL.
Thread-safe.
*/
uint32_t Object_weak_refs_get(const Object* self);


/** Attempts to obtain a strong reference from a weak reference.
If successful, the caller must unreference the strong reference with Object_unref().
Returns false if self is NULL.
Thread-safe.
*/
__attribute__((hot, warn_unused_result))
bool Object_weak_lock(const Object* self);


/** Sentinel data pointer for classes without per-instance data. Must not be dereferenced. */
#define OBJECT_NO_DATA ((void*) 1)


/** Assigns an object a class type with a data pointer.
data must not be NULL. Pass OBJECT_NO_DATA for classes without per-instance data.
Does nothing if self, cls, or data is NULL, or if the class is already present.
Not thread-safe with accessing classes or calling methods.
*/
void Object_class_push(Object* self, const Class* cls, void* data);


/** Returns the data pointer for self's class cls, or NULL if self is not of class cls.
Returns NULL if self is NULL.
Not thread-safe with accessing classes or calling methods.
*/
__attribute__((pure, hot))
void* Object_data_get(const Object* self, const Class* cls);


/** Removes a class and all classes above it from an object.
For each class in reverse order, this reverts its method overrides, calls free(), and removes its data.
Does nothing if self is NULL or the class is not found.
Not thread-safe with accessing classes or calling methods.
*/
void Object_class_remove(Object* self, const Class* cls);


/** Overrides a method dispatched by the `dispatcher` function pointer.
Not thread-safe with accessing methods or calling methods.
*/
void Object_method_push(Object* self, void* dispatcher, void* method);


/** Returns the direct method for the given dispatch method.
Returns NULL if no method has been pushed for the dispatcher.
Returns NULL if self is NULL.
Not thread-safe with accessing methods or calling methods.
*/
__attribute__((pure, hot))
void* Object_method_get(const Object* self, void* dispatcher);


/** Returns the method that was overridden by the given method.
Returns NULL if the method is the first in the chain, or does not exist.
Returns NULL if self is NULL.
Not thread-safe with accessing methods or calling methods.
*/
__attribute__((pure, hot))
void* Object_supermethod_get(const Object* self, void* method);


/** Generates a string listing all type names and data pointers of an object in order of specialization.
Returns NULL if self is NULL.
Caller must free() the returned string.
Not thread-safe with accessing classes.
*/
char* Object_inspect(const Object* self);


/** Returns the number of objects currently alive.
Useful for leak detection and debugging.
*/
uint64_t Object_alive_get(void);


/** Returns the number of schema nodes, the internal records shared by objects with the same class and method push history.
Useful for profiling and debugging.
*/
uint64_t Object_schemaNodes_count_get(void);


EXTERNC_END
