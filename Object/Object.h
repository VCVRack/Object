#pragma once

#include <stdlib.h>
#include <stdbool.h>


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
	DEFINE_METHOD(Class, Method, void, VOID, (), {})
*/
#define VOID


/**************************************
Declaration macros for public headers
*/


/** Declares a class Class struct and constructor function.

CLASS is the class name such as `Animal`.
INITARGS are the arguments of the create/specialize functions such as `(const char* name, int legs)`.

Superclasses are defined in your implementation, not in your header.

Example:
	DECLARE_CLASS(Animal, (const char* name, int legs))

declares the following functions.

Constructor:
	Object* Animal_create(Object* self, const char* name, int legs);

Specialization function:
	void Animal_specialize(Object* self, const char* name, int legs);

Type checking function:
	bool Animal_is(const Object* self);
*/
#define DECLARE_CLASS(CLASS, INITARGS) \
	extern const Class CLASS##_class; \
	EXTERNC Object* CLASS##_create(EXPAND INITARGS); \
	EXTERNC void CLASS##_specialize(Object* self COMMA_EXPAND INITARGS); \
	EXTERNC bool CLASS##_is(const Object* self)


/** Declares a non-virtual method for a class.
This method cannot be overridden by specialized classes (subclasses).
Similar to DECLARE_METHOD_VIRTUAL but doesn't define method getters/setters.
*/
#define DECLARE_METHOD(CLASS, METHOD, RETTYPE, ARGTYPES) \
	EXTERNC RETTYPE CLASS##_##METHOD(Object* self COMMA_EXPAND ARGTYPES)


/** Declares a non-virtual method for a class with a `const Object*` argument.
*/
#define DECLARE_METHOD_CONST(CLASS, METHOD, RETTYPE, ARGTYPES) \
	EXTERNC RETTYPE CLASS##_##METHOD(const Object* self COMMA_EXPAND ARGTYPES)


/** Declares a virtual method for a class.

CLASS is the class name such as `Animal`.
METHOD is the method name such as `speak`.
RETTYPE is the return value such as `void`.
ARGTYPES are the arguments such as `(const char* name, int loudness)`.

Example:
	DECLARE_METHOD_VIRTUAL(Animal, speak, void, (const char* name))

Declares the following functions.

Virtual dispatch call:
	void Animal_speak(Object* self, const char* name, int loudness);

Non-virtual (direct) call:
	void Animal_speak_mdirect(Object* self, const char* name, int loudness);

Method getter:
	Animal_speak_m Animal_speak_mget(const Object* self);

Method setter:
	void Animal_speak_mset(Object* self, Animal_speak_m m);
*/
#define DECLARE_METHOD_VIRTUAL(CLASS, METHOD, RETTYPE, ARGTYPES) \
	typedef RETTYPE CLASS##_##METHOD##_m(Object* self COMMA_EXPAND ARGTYPES); \
	EXTERNC RETTYPE CLASS##_##METHOD(Object* self COMMA_EXPAND ARGTYPES); \
	EXTERNC RETTYPE CLASS##_##METHOD##_mdirect(Object* self COMMA_EXPAND ARGTYPES); \
	EXTERNC CLASS##_##METHOD##_m* CLASS##_##METHOD##_mget(const Object* self); \
	EXTERNC void CLASS##_##METHOD##_mset(Object* self, CLASS##_##METHOD##_m* m)


/** Declares a method that overrides a different class's virtual method.
*/
#define DECLARE_METHOD_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES) \
	EXTERNC RETTYPE CLASS##_##METHOD##_mdirect(Object* self COMMA_EXPAND ARGTYPES)


#define DECLARE_METHOD_VIRTUAL_CONST(CLASS, METHOD, RETTYPE, ARGTYPES) \
	typedef RETTYPE CLASS##_##METHOD##_m(const Object* self COMMA_EXPAND ARGTYPES); \
	EXTERNC RETTYPE CLASS##_##METHOD(const Object* self COMMA_EXPAND ARGTYPES); \
	EXTERNC RETTYPE CLASS##_##METHOD##_mdirect(const Object* self COMMA_EXPAND ARGTYPES); \
	EXTERNC CLASS##_##METHOD##_m* CLASS##_##METHOD##_mget(const Object* self); \
	EXTERNC void CLASS##_##METHOD##_mset(Object* self, CLASS##_##METHOD##_m* m)


#define DECLARE_METHOD_CONST_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES) \
	EXTERNC RETTYPE CLASS##_##METHOD##_mdirect(const Object* self COMMA_EXPAND ARGTYPES)


/** Declares a non-virtual getter method for a class.

CLASS is the class name such as `Animal`.
PROP is the property name such as `name`.
TYPE is the type of the property such as `const char*`.

Example:
	DECLARE_GETTER(Animal, name, const char*)

Declares the function:
	const char* Animal_name_get(const Object* self);
*/
#define DECLARE_GETTER(CLASS, PROP, TYPE) \
	DECLARE_METHOD_CONST(CLASS, PROP##_get, TYPE, ())


/** Declares a virtual getter method for a class.
Same arguments as DECLARE_GETTER().

Example:
	DECLARE_GETTER_VIRTUAL(Animal, name, const char*)

Declares the functions:
	const char* Animal_name_get(const Object* self);
	const char* Animal_name_get_mdirect(Object* self);
	Animal_name_get_m Animal_name_get_mget(const Object* self);
	void Animal_name_get_mset(Object* self, Animal_name_get_m m);
*/
#define DECLARE_GETTER_VIRTUAL(CLASS, PROP, TYPE) \
	DECLARE_METHOD_VIRTUAL_CONST(CLASS, PROP##_get, TYPE, ())


/** Declares a getter method that overrides a different class's virtual getter.
*/
#define DECLARE_GETTER_OVERRIDE(CLASS, PROP, TYPE) \
	DECLARE_METHOD_CONST_OVERRIDE(CLASS, PROP##_get, TYPE, ())


/** Declares a non-virtual setter method for a class.
Same arguments as DECLARE_GETTER().

Example:
	DECLARE_SETTER(Animal, name, const char*)

Declares the function:
	void Animal_name_set(const Object* self, const char* name);
*/
#define DECLARE_SETTER(CLASS, PROP, TYPE) \
	DECLARE_METHOD(CLASS, PROP##_set, void, (TYPE PROP))


/** Declares a virtual setter method for a class.
Same arguments as DECLARE_GETTER().

Example:
	DECLARE_SETTER_VIRTUAL(Animal, name, const char*)

Declares the functions:
	void Animal_name_set(const Object* self, const char* name);
	void Animal_name_set_mdirect(Object* self, const char* name);
	Animal_name_set_m Animal_name_set_mget(const Object* self);
	void Animal_name_set_mset(Object* self, Animal_name_set_m m);
*/
#define DECLARE_SETTER_VIRTUAL(CLASS, PROP, TYPE) \
	DECLARE_METHOD_VIRTUAL(CLASS, PROP##_set, void, (TYPE PROP))


#define DECLARE_SETTER_OVERRIDE(CLASS, PROP, TYPE) \
	DECLARE_METHOD_OVERRIDE(CLASS, PROP##_set, void, (TYPE PROP))


/** Declares a non-virtual getter/setter method pair for a class.
Same arguments as DECLARE_GETTER().

Example:
	DECLARE_ACCESSOR(Animal, name, const char*)
*/
#define DECLARE_ACCESSOR(CLASS, PROP, TYPE) \
	DECLARE_GETTER(CLASS, PROP, TYPE); \
	DECLARE_SETTER(CLASS, PROP, TYPE)


#define DECLARE_ACCESSOR_VIRTUAL(CLASS, PROP, TYPE) \
	DECLARE_GETTER_VIRTUAL(CLASS, PROP, TYPE); \
	DECLARE_SETTER_VIRTUAL(CLASS, PROP, TYPE)


#define DECLARE_ACCESSOR_OVERRIDE(CLASS, PROP, TYPE) \
	DECLARE_GETTER_OVERRIDE(CLASS, PROP, TYPE); \
	DECLARE_SETTER_OVERRIDE(CLASS, PROP, TYPE)


/**************************************
Definition macros for source implementation files
*/


#define DEFINE_CLASS_FUNCTIONS(CLASS, INITARGS, INITARGNAMES, INIT) \
	EXTERNC void CLASS##_specialize(Object* self COMMA_EXPAND INITARGS) { \
		if (!self) \
			return; \
		if (Object_class_check(self, &CLASS##_class, NULL)) \
			return; \
		INIT \
	} \
	EXTERNC Object* CLASS##_create(EXPAND INITARGS) { \
		Object* self = Object_create(); \
		CLASS##_specialize(self COMMA_EXPAND INITARGNAMES); \
		return self; \
	} \
	EXTERNC bool CLASS##_is(const Object* self) { \
		return Object_class_check(self, &CLASS##_class, NULL); \
	}


#define DEFINE_CLASS_FREE(CLASS, CODE) \
	static void CLASS##_free(Object* self) { \
		if (!self) \
			return; \
		CLASS* data = NULL; \
		if (!Object_class_check(self, &CLASS##_class, (void**) &data)) \
			return; \
		CODE \
	}


#define DEFINE_CLASS_FINALIZE(CLASS, CODE) \
	static void CLASS##_finalize(Object* self) { \
		if (!self) \
			return; \
		CLASS* data = NULL; \
		if (!Object_class_check(self, &CLASS##_class, (void**) &data)) \
			return; \
		CODE \
	}


#define DEFINE_CLASS(CLASS, INITARGS, INITARGNAMES, INIT, FREE) \
	extern const Class CLASS##_class; \
	typedef struct CLASS CLASS; \
	DEFINE_CLASS_FUNCTIONS(CLASS, INITARGS, INITARGNAMES, INIT) \
	DEFINE_CLASS_FREE(CLASS, FREE) \
	const Class CLASS##_class = { \
		#CLASS, \
		CLASS##_free, \
		NULL, \
		{} \
	};


/** A class's non-virtual methods cannot be overridden.
Upgrading a non-virtual method to a virtual method creates linker symbols and does not break the ABI.
Downgrading a virtual method to a non-virtual method removes linker symbols and therefore breaks the ABI.
*/
#define DEFINE_METHOD(CLASS, METHOD, RETTYPE, DEFAULT, ARGTYPES, CODE) \
	EXTERNC RETTYPE CLASS##_##METHOD(Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data = NULL; \
		if (!Object_class_check(self, &CLASS##_class, (void**) &data)) \
			return DEFAULT; \
		CODE \
	}


#define DEFINE_METHOD_CONST(CLASS, METHOD, RETTYPE, DEFAULT, ARGTYPES, CODE) \
	EXTERNC RETTYPE CLASS##_##METHOD(const Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data = NULL; \
		if (!Object_class_check(self, &CLASS##_class, (void**) &data)) \
			return DEFAULT; \
		CODE \
	}


#define DEFINE_METHOD_VIRTUAL(CLASS, METHOD, RETTYPE, DEFAULT, ARGTYPES, ARGNAMES, CODE) \
	typedef RETTYPE CLASS##_##METHOD##_m(Object* self COMMA_EXPAND ARGTYPES); \
	/* Virtual dispatch */ \
	EXTERNC RETTYPE CLASS##_##METHOD(Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS##_##METHOD##_m* m = CLASS##_##METHOD##_mget(self); \
		if (!m) \
			return DEFAULT; \
		return m(self COMMA_EXPAND ARGNAMES); \
	} \
	/* Non-virtual call */ \
	EXTERNC RETTYPE CLASS##_##METHOD##_mdirect(Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data = NULL; \
		if (!Object_class_check(self, &CLASS##_class, (void**) &data)) \
			return DEFAULT; \
		CODE \
	} \
	/* Method getter */ \
	EXTERNC CLASS##_##METHOD##_m* CLASS##_##METHOD##_mget(const Object* self) { \
		CLASS* data = NULL; \
		if (!Object_class_check(self, &CLASS##_class, (void**) &data)) \
			return NULL; \
		if (!data) \
			return NULL; \
		return data->METHOD; \
	} \
	/* Method setter */ \
	EXTERNC void CLASS##_##METHOD##_mset(Object* self, CLASS##_##METHOD##_m* m) { \
		CLASS* data = NULL; \
		if (!Object_class_check(self, &CLASS##_class, (void**) &data)) \
			return; \
		if (!data) \
			return; \
		data->METHOD = m; \
	}


#define DEFINE_METHOD_OVERRIDE(CLASS, METHOD, RETTYPE, DEFAULT, ARGTYPES, CODE) \
	EXTERNC RETTYPE CLASS##_##METHOD##_mdirect(Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data = NULL; \
		if (!Object_class_check(self, &CLASS##_class, (void**) &data)) \
			return DEFAULT; \
		CODE \
	}


#define DEFINE_METHOD_VIRTUAL_CONST(CLASS, METHOD, RETTYPE, DEFAULT, ARGTYPES, ARGNAMES, CODE) \
	typedef RETTYPE CLASS##_##METHOD##_m(const Object* self COMMA_EXPAND ARGTYPES); \
	/* Virtual dispatch */ \
	EXTERNC RETTYPE CLASS##_##METHOD(const Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS##_##METHOD##_m* m = CLASS##_##METHOD##_mget(self); \
		if (!m) \
			return DEFAULT; \
		return m(self COMMA_EXPAND ARGNAMES); \
	} \
	/* Non-virtual call */ \
	EXTERNC RETTYPE CLASS##_##METHOD##_mdirect(const Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data = NULL; \
		if (!Object_class_check(self, &CLASS##_class, (void**) &data)) \
			return DEFAULT; \
		CODE \
	} \
	/* Method getter */ \
	EXTERNC CLASS##_##METHOD##_m* CLASS##_##METHOD##_mget(const Object* self) { \
		CLASS* data = NULL; \
		if (!Object_class_check(self, &CLASS##_class, (void**) &data)) \
			return NULL; \
		if (!data) \
			return NULL; \
		return data->METHOD; \
	} \
	/* Method setter */ \
	EXTERNC void CLASS##_##METHOD##_mset(Object* self, CLASS##_##METHOD##_m* m) { \
		CLASS* data = NULL; \
		if (!Object_class_check(self, &CLASS##_class, (void**) &data)) \
			return; \
		if (!data) \
			return; \
		data->METHOD = m; \
	}


#define DEFINE_METHOD_CONST_OVERRIDE(CLASS, METHOD, RETTYPE, DEFAULT, ARGTYPES, CODE) \
	EXTERNC RETTYPE CLASS##_##METHOD##_mdirect(const Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data = NULL; \
		if (!Object_class_check(self, &CLASS##_class, (void**) &data)) \
			return DEFAULT; \
		CODE \
	}


#define DEFINE_GETTER(CLASS, PROP, TYPE, DEFAULT, CODE) \
	DEFINE_METHOD_CONST(CLASS, PROP##_get, TYPE, DEFAULT, (), CODE)


#define DEFINE_GETTER_AUTOMATIC(CLASS, PROP, TYPE, DEFAULT) \
	DEFINE_GETTER(CLASS, PROP, TYPE, DEFAULT, { \
		if (!data) \
			return DEFAULT; \
		return data->PROP; \
	})


#define DEFINE_GETTER_VIRTUAL(CLASS, PROP, TYPE, DEFAULT, CODE) \
	DEFINE_METHOD_VIRTUAL_CONST(CLASS, PROP##_get, TYPE, DEFAULT, (), (), CODE)


/** Defines a getter method that returns the property from the data struct.
*/
#define DEFINE_GETTER_VIRTUAL_AUTOMATIC(CLASS, PROP, TYPE, DEFAULT) \
	DEFINE_GETTER_VIRTUAL(CLASS, PROP, TYPE, DEFAULT, { \
		if (!data) \
			return DEFAULT; \
		return data->PROP; \
	})


#define DEFINE_GETTER_OVERRIDE(CLASS, PROP, TYPE, DEFAULT, CODE) \
	DEFINE_METHOD_CONST_OVERRIDE(CLASS, PROP##_get, TYPE, DEFAULT, (), CODE)


#define DEFINE_SETTER(CLASS, PROP, TYPE, CODE) \
	DEFINE_METHOD(CLASS, PROP##_set, void, VOID, (TYPE PROP), CODE)


#define DEFINE_SETTER_AUTOMATIC(CLASS, PROP, TYPE) \
	DEFINE_SETTER(CLASS, PROP, TYPE, { \
		if (!data) \
			return; \
		data->PROP = PROP; \
	})


#define DEFINE_SETTER_VIRTUAL(CLASS, PROP, TYPE, CODE) \
	DEFINE_METHOD_VIRTUAL(CLASS, PROP##_set, void, VOID, (TYPE PROP), (PROP), CODE)


/** Defines a setter method that sets the property to the data struct.
*/
#define DEFINE_SETTER_VIRTUAL_AUTOMATIC(CLASS, PROP, TYPE) \
	DEFINE_SETTER_VIRTUAL(CLASS, PROP, TYPE, { \
		if (!data) \
			return; \
		data->PROP = PROP; \
	})


#define DEFINE_SETTER_OVERRIDE(CLASS, PROP, TYPE, CODE) \
	DEFINE_METHOD_OVERRIDE(CLASS, PROP##_set, void, VOID, (TYPE PROP), CODE)


#define DEFINE_ACCESSOR(CLASS, PROP, TYPE, DEFAULT, GETTER, SETTER) \
	DEFINE_GETTER(CLASS, PROP, TYPE, DEFAULT, GETTER) \
	DEFINE_SETTER(CLASS, PROP, TYPE, SETTER)


#define DEFINE_ACCESSOR_AUTOMATIC(CLASS, PROP, TYPE, DEFAULT) \
	DEFINE_GETTER_AUTOMATIC(CLASS, PROP, TYPE, DEFAULT) \
	DEFINE_SETTER_AUTOMATIC(CLASS, PROP, TYPE)


#define DEFINE_ACCESSOR_VIRTUAL(CLASS, PROP, TYPE, DEFAULT, GETTER, SETTER) \
	DEFINE_GETTER_VIRTUAL(CLASS, PROP, TYPE, DEFAULT, GETTER) \
	DEFINE_SETTER_VIRTUAL(CLASS, PROP, TYPE, SETTER)


#define DEFINE_ACCESSOR_VIRTUAL_AUTOMATIC(CLASS, PROP, TYPE, DEFAULT) \
	DEFINE_GETTER_VIRTUAL_AUTOMATIC(CLASS, PROP, TYPE, DEFAULT) \
	DEFINE_SETTER_VIRTUAL_AUTOMATIC(CLASS, PROP, TYPE)


#define DEFINE_ACCESSOR_OVERRIDE(CLASS, PROP, TYPE, DEFAULT, GETTER, SETTER) \
	DEFINE_GETTER_OVERRIDE(CLASS, PROP, TYPE, DEFAULT, GETTER) \
	DEFINE_SETTER_OVERRIDE(CLASS, PROP, TYPE, SETTER)


#define STORE_METHOD(CLASS, METHOD) \
	CLASS##_##METHOD##_m* METHOD


#define STORE_GETTER(CLASS, PROP) \
	STORE_METHOD(CLASS, PROP##_get)


#define STORE_SETTER(CLASS, PROP) \
	STORE_METHOD(CLASS, PROP##_set)


#define STORE_ACCESSOR(CLASS, PROP) \
	STORE_GETTER(CLASS, PROP); \
	STORE_SETTER(CLASS, PROP)


/**************************************
Call macros
*/


#define CREATE(CLASS, ...) \
	CLASS##_create(__VA_ARGS__)


#define SPECIALIZE(SELF, CLASS, ...) \
	CLASS##_specialize(SELF __VA_OPT__(,) __VA_ARGS__)


#define IS(SELF, CLASS) \
	CLASS##_is(SELF)


#define SET_METHOD(SELF, CLASS, SUPERCLASS, METHOD) \
	SUPERCLASS##_##METHOD##_mset(SELF, CLASS##_##METHOD##_mdirect)


#define SET_GETTER(SELF, CLASS, SUPERCLASS, PROP) \
	SET_METHOD(SELF, CLASS, SUPERCLASS, PROP##_get); \


#define SET_SETTER(SELF, CLASS, SUPERCLASS, PROP) \
	SET_METHOD(SELF, CLASS, SUPERCLASS, PROP##_set)


#define SET_ACCESSOR(SELF, CLASS, SUPERCLASS, PROP) \
	SET_GETTER(SELF, CLASS, SUPERCLASS, PROP); \
	SET_SETTER(SELF, CLASS, SUPERCLASS, PROP)


#define PUSH_CLASS(SELF, CLASS, DATA) \
	Object_class_push(SELF, &CLASS##_class, DATA)


#define CALL(SELF, CLASS, METHOD, ...) \
	CLASS##_##METHOD(SELF __VA_OPT__(,) __VA_ARGS__)


#define CALL_DIRECT(SELF, CLASS, METHOD, ...) \
	CLASS##_##METHOD##_mdirect(SELF __VA_OPT__(,) __VA_ARGS__)


#define GET(SELF, CLASS, PROP, ...) \
	CLASS##_##PROP##_get(SELF __VA_OPT__(,) __VA_ARGS__)


#define GET_DIRECT(SELF, CLASS, PROP, ...) \
	CLASS##_##PROP##_get_mdirect(SELF __VA_OPT__(,) __VA_ARGS__)


#define SET(SELF, CLASS, PROP, ...) \
	CLASS##_##PROP##_set(SELF __VA_OPT__(,) __VA_ARGS__)


#define SET_DIRECT(SELF, CLASS, PROP, ...) \
	CLASS##_##PROP##_set_mdirect(SELF __VA_OPT__(,) __VA_ARGS__)


/**************************************
Runtime symbols
*/


EXTERNC_BEGIN


/** The type of all polymorphic objects in this API. */
typedef struct Object Object;

typedef void Object_free_m(Object* self);
typedef void Object_finalize_m(Object* self);

typedef struct Class {
	const char* name;
	/** Frees data pointer and its contents, if non-NULL.
	Must not call virtual methods.
	Called by Object_free() for each class in reverse order.
	*/
	Object_free_m* free;
	/** Prepares object to be freed, if non-NULL.
	Can call virtual methods.
	Called by Object_free() for each class in reverse order.
	*/
	Object_finalize_m* finalize;
	// Reserved for future fields. Must be zero.
	// Possibly `getDataSize`
	void* reserved[29];
} Class;


/** Creates an object with no classes.
Reference count is set to 1.
Object must be released with Object_release() to prevent a memory leak.
Never returns NULL.
*/
Object* Object_create();


/** Increments the object's reference counter.
Use this to share another reference to this object.
Each reference must be released with Object_release() to prevent a memory leak.
Thread-safe.
Does nothing if self is NULL.
*/
void Object_obtain(Object* self);


/** Decrements the object's reference counter.
If no references are left, this frees the object and its internal data for each class.
Object should be considered invalid after calling this function.

Calls finalize() for each class in reverse order of specialization, and then free() in reverse order.
Thread-safe.
Does nothing if self is NULL.
*/
void Object_release(Object* self);


/** Returns the number of shared references to the object.
*/
size_t Object_refs_get(const Object* self);


/** Assigns an object a class type with a data pointer.
Does nothing if self is NULL.
*/
void Object_class_push(Object* self, const Class* cls, void* data);


/** Returns true if an object has a class.
If so, and dataOut is non-NULL, sets the data pointer.
Returns false if self is NULL.
*/
bool Object_class_check(const Object* self, const Class* cls, void** dataOut);


/** Prints all types of an object in order of specialization.
TODO: Change to `char* Object_inspect(self)` or something.
*/
void Object_debug(const Object* self);


EXTERNC_END
