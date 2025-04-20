#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


// Object.h


/** Converts `EXPAND (1, 2, 3)` to `1, 2, 3`
and `EXPAND ()` to ``.
*/
#define EXPAND(...) __VA_ARGS__

/** Converts `0 COMMA_EXPAND (1, 2, 3)` to `0, 1, 2, 3`
and `0 COMMA_EXPAND ()` to `0`
*/
#define COMMA_EXPAND(...) __VA_OPT__(,) __VA_ARGS__


// Declaration macros for public headers


/** Declares a class Class struct and constructor function.
*/
#define DECLARE_CLASS(CLASS, INITARGS) \
	extern const Class CLASS##_class; \
	Object* CLASS##_create(EXPAND INITARGS); \
	void CLASS##_specialize(Object* self COMMA_EXPAND INITARGS)


#define DECLARE_FUNCTION(CLASS, METHOD, RETTYPE, ARGTYPES) \
	RETTYPE CLASS##_##METHOD(Object* self COMMA_EXPAND ARGTYPES)


/** Declares a virtual method for a class.

CLASS is the class name such as `Animal`.
METHOD is the method name such as `speak`.
RETTYPE is the return value such as `void`.
ARGTYPES are the arguments such as `(const char* name, int loudness)`.

Example:
	DECLARE_METHOD(Animal, speak, void, (const char* name))

declares the following functions.

Virtual dispatch call:
	void Animal_speak(Object* self, const char* name, int loudness);

Method getter:
	Animal_speak_m Animal_speak_mget(const Object* self);

Method setter:
	void Animal_speak_mset(const Object* self, Animal_speak_m m);

Non-virtual (direct) call:
	void Animal_speak_direct(Object* self, const char* name, int loudness);
*/
#define DECLARE_METHOD(CLASS, METHOD, RETTYPE, ARGTYPES) \
	typedef RETTYPE (*CLASS##_##METHOD##_m)(Object* self COMMA_EXPAND ARGTYPES); \
	RETTYPE CLASS##_##METHOD(Object* self COMMA_EXPAND ARGTYPES); \
	CLASS##_##METHOD##_m CLASS##_##METHOD##_mget(const Object* self); \
	void CLASS##_##METHOD##_mset(Object* self, CLASS##_##METHOD##_m m); \
	RETTYPE CLASS##_##METHOD##_mdirect(Object* self COMMA_EXPAND ARGTYPES)


/** Declares a virtual method with a const `self` argument.
*/
#define DECLARE_METHOD_CONST(CLASS, METHOD, RETTYPE, ARGTYPES) \
	typedef RETTYPE (*CLASS##_##METHOD##_m)(const Object* self COMMA_EXPAND ARGTYPES); \
	RETTYPE CLASS##_##METHOD(const Object* self COMMA_EXPAND ARGTYPES); \
	CLASS##_##METHOD##_m CLASS##_##METHOD##_mget(const Object* self); \
	void CLASS##_##METHOD##_mset(Object* self, CLASS##_##METHOD##_m m); \
	RETTYPE CLASS##_##METHOD##_mdirect(const Object* self COMMA_EXPAND ARGTYPES)


/** Declares a virtual getter and setter for a class.

CLASS is the class name such as `Animal`.
PROP is the property name such as `name`.
TYPE is the type of the property such as `const char*`.

Example:
	DECLARE_ACCESSORS(Animal, name, const char*)

Creates the virtual methods:
	const char* Animal_name_get(const Object* self);
	void Animal_name_set(const Object* self, const char* name);

along with method getters, setters, and direct functions for both accessors.
*/
#define DECLARE_ACCESSORS(CLASS, PROP, TYPE) \
	DECLARE_METHOD_CONST(CLASS, PROP##_get, TYPE, ()); \
	DECLARE_METHOD(CLASS, PROP##_set, void, (TYPE PROP))


#define DECLARE_METHOD_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES) \
	RETTYPE CLASS##_##METHOD##_mdirect(Object* self COMMA_EXPAND ARGTYPES)


#define DECLARE_METHOD_CONST_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES) \
	RETTYPE CLASS##_##METHOD##_mdirect(const Object* self COMMA_EXPAND ARGTYPES)


#define DECLARE_ACCESSORS_OVERRIDE(CLASS, PROP, TYPE) \
	DECLARE_METHOD_CONST_OVERRIDE(CLASS, PROP##_get, TYPE, ()); \
	DECLARE_METHOD_OVERRIDE(CLASS, PROP##_set, void, (TYPE PROP))


// Definition macros for source implementation files


/** Represents the "value" of a void return type.
Example:
	void f() {
		return VOID;
	}

Example:
	DEFINE_FUNCTION(Class, Method, void, VOID, (), {})
*/
#define VOID


#define DEFINE_CLASS(CLASS, INITARGS, INITARGNAMES, INIT, FREE) \
	Object* CLASS##_create(EXPAND INITARGS) { \
		Object* self = Object_create(); \
		CLASS##_specialize(self COMMA_EXPAND INITARGNAMES); \
		return self; \
	} \
	void CLASS##_specialize(Object* self COMMA_EXPAND INITARGS) { \
		if (!self) \
			return; \
		if (Object_checkClass(self, &CLASS##_class, NULL)) \
			return; \
		INIT \
	} \
	static void CLASS##_free(Object* self) { \
		if (!self) \
			return; \
		CLASS* data = NULL; \
		if (!Object_checkClass(self, &CLASS##_class, (void**) &data)) \
			return; \
		FREE \
	} \
	const Class CLASS##_class = { \
		#CLASS, \
		CLASS##_free, \
		NULL, \
		{} \
	};


/** Class functions (aka non-virtual methods) cannot be overridden.
Upgrading a class function to a class method creates linker symbols and does not break the ABI.
Downgrading a method to a function removes linker symbols and therefore breaks the ABI.
*/
#define DEFINE_FUNCTION(CLASS, METHOD, RETTYPE, DEFAULT, ARGTYPES, CODE) \
	RETTYPE CLASS##_##METHOD(Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data = NULL; \
		if (!Object_checkClass(self, &CLASS##_class, (void**) &data)) \
			return DEFAULT; \
		CODE \
	}


#define DEFINE_FUNCTION_CONST(CLASS, METHOD, RETTYPE, DEFAULT, ARGTYPES, CODE) \
	RETTYPE CLASS##_##METHOD(const Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data = NULL; \
		if (!Object_checkClass(self, &CLASS##_class, (void**) &data)) \
			return DEFAULT; \
		CODE \
	}


#define DEFINE_METHOD(CLASS, METHOD, RETTYPE, DEFAULT, ARGTYPES, ARGNAMES, CODE) \
	/* Method getter */ \
	CLASS##_##METHOD##_m CLASS##_##METHOD##_mget(const Object* self) { \
		CLASS* data = NULL; \
		if (!Object_checkClass(self, &CLASS##_class, (void**) &data)) \
			return NULL; \
		if (!data) \
			return NULL; \
		return data->METHOD; \
	} \
	/* Method setter */ \
	void CLASS##_##METHOD##_mset(Object* self, CLASS##_##METHOD##_m m) { \
		CLASS* data = NULL; \
		if (!Object_checkClass(self, &CLASS##_class, (void**) &data)) \
			return; \
		if (!data) \
			return; \
		data->METHOD = m; \
	} \
	/* Virtual dispatch */ \
	RETTYPE CLASS##_##METHOD(Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS##_##METHOD##_m m = CLASS##_##METHOD##_mget(self); \
		if (!m) \
			return DEFAULT; \
		return m(self COMMA_EXPAND ARGNAMES); \
	} \
	/* Non-virtual call */ \
	RETTYPE CLASS##_##METHOD##_mdirect(Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data = NULL; \
		if (!Object_checkClass(self, &CLASS##_class, (void**) &data)) \
			return DEFAULT; \
		CODE \
	}


#define DEFINE_METHOD_CONST(CLASS, METHOD, RETTYPE, DEFAULT, ARGTYPES, ARGNAMES, CODE) \
	/* Method getter */ \
	CLASS##_##METHOD##_m CLASS##_##METHOD##_mget(const Object* self) { \
		CLASS* data = NULL; \
		if (!Object_checkClass(self, &CLASS##_class, (void**) &data)) \
			return NULL; \
		if (!data) \
			return NULL; \
		return data->METHOD; \
	} \
	/* Method setter */ \
	void CLASS##_##METHOD##_mset(Object* self, CLASS##_##METHOD##_m m) { \
		CLASS* data = NULL; \
		if (!Object_checkClass(self, &CLASS##_class, (void**) &data)) \
			return; \
		if (!data) \
			return; \
		data->METHOD = m; \
	} \
	/* Virtual dispatch */ \
	RETTYPE CLASS##_##METHOD(const Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS##_##METHOD##_m m = CLASS##_##METHOD##_mget(self); \
		if (!m) \
			return DEFAULT; \
		return m(self COMMA_EXPAND ARGNAMES); \
	} \
	/* Non-virtual call */ \
	RETTYPE CLASS##_##METHOD##_mdirect(const Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data = NULL; \
		if (!Object_checkClass(self, &CLASS##_class, (void**) &data)) \
			return DEFAULT; \
		CODE \
	}


#define DEFINE_ACCESSORS(CLASS, NAME, TYPE, DEFAULT, GETTER, SETTER) \
	DEFINE_METHOD_CONST(CLASS, NAME##_get, TYPE, DEFAULT, (), (), GETTER) \
	DEFINE_METHOD(CLASS, NAME##_set, void, VOID, (TYPE NAME), (NAME), SETTER)


#define DEFINE_ACCESSORS_AUTOMATIC(CLASS, NAME, TYPE, DEFAULT) \
	DEFINE_ACCESSORS(CLASS, NAME, TYPE, DEFAULT, { \
		if (!data) \
			return DEFAULT; \
		return data->NAME; \
	}, { \
		if (!data) \
			return; \
		data->NAME = NAME; \
	})


#define DEFINE_METHOD_OVERRIDE(CLASS, METHOD, RETTYPE, DEFAULT, ARGTYPES, CODE) \
	RETTYPE CLASS##_##METHOD##_mdirect(Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data = NULL; \
		if (!Object_checkClass(self, &CLASS##_class, (void**) &data)) \
			return DEFAULT; \
		CODE \
	}


#define DEFINE_METHOD_CONST_OVERRIDE(CLASS, METHOD, RETTYPE, DEFAULT, ARGTYPES, CODE) \
	RETTYPE CLASS##_##METHOD##_mdirect(const Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data = NULL; \
		if (!Object_checkClass(self, &CLASS##_class, (void**) &data)) \
			return DEFAULT; \
		CODE \
	}


#define DEFINE_ACCESSORS_OVERRIDE(CLASS, PROP, TYPE, DEFAULT, GETTER, SETTER) \
	DEFINE_METHOD_CONST_OVERRIDE(CLASS, PROP##_get, TYPE, DEFAULT, (), GETTER) \
	DEFINE_METHOD_OVERRIDE(CLASS, PROP##_set, void, VOID, (TYPE PROP), SETTER)


// TODO Consider renaming these
#define STORE_METHOD(CLASS, METHOD) \
	CLASS##_##METHOD##_m METHOD


#define STORE_ACCESSORS(CLASS, PROP) \
	STORE_METHOD(CLASS, PROP##_get); \
	STORE_METHOD(CLASS, PROP##_set)


// Call macros


// TODO Change name
#define OVERRIDE_METHOD(SELF, CLASS, SUPERCLASS, METHOD) \
	SUPERCLASS##_##METHOD##_mset(SELF, CLASS##_##METHOD##_mdirect)


// TODO Change name
#define OVERRIDE_ACCESSORS(SELF, CLASS, SUPERCLASS, PROP) \
	OVERRIDE_METHOD(SELF, CLASS, SUPERCLASS, PROP##_get); \
	OVERRIDE_METHOD(SELF, CLASS, SUPERCLASS, PROP##_set)


#define SPECIALIZE(SELF, CLASS) \
	CLASS##_specialize(SELF)


#define PUSH_CLASS(SELF, CLASS, DATA) \
	Object_pushClass(SELF, &CLASS##_class, DATA)


#define CALL_METHOD(SELF, CLASS, METHOD, ...) \
	CLASS##_##METHOD(SELF COMMA_EXPAND __VA_ARGS__)


#define CALL_METHOD_DIRECT(SELF, CLASS, METHOD, ...) \
	CLASS##_##METHOD##_mdirect(SELF COMMA_EXPAND __VA_ARGS__)


#define GET_PROPERTY(SELF, CLASS, PROP) \
	CLASS##_##PROP##_get(SELF)


#define GET_PROPERTY_DIRECT(SELF, CLASS, PROP) \
	CLASS##_##PROP##_get_mdirect(SELF)


#define SET_PROPERTY(SELF, CLASS, PROP, VALUE) \
	CLASS##_##PROP##_set(SELF, VALUE)


#define SET_PROPERTY_DIRECT(SELF, CLASS, PROP, VALUE) \
	CLASS##_##PROP##_set_mdirect(SELF, VALUE)


// Runtime symbols


/** The type of all polymorphic objects in this API. */
typedef struct Object Object;

typedef void (*Object_free_m)(Object* self);
typedef void (*Object_finalize_m)(Object* self);

typedef struct Class {
	const char* name;
	/** Frees data pointer and its contents, if non-NULL.
	Must not call virtual methods.
	Called by Object_free() for each class in reverse order.
	*/
	Object_free_m free;
	/** Prepares object to be freed, if non-NULL.
	Can call virtual methods.
	Called by Object_free() for each class in reverse order.
	*/
	Object_finalize_m finalize;
	// Reserved for future fields. Must be zero.
	// Possibly `getDataSize`
	void* reserved[29];
} Class;


/** Creates an object with no classes */
Object* Object_create();


/** Deletes the object and its internal data for each class.
Calls finalize() for each class in reverse order, and then free() in reverse order.
*/
void Object_free(Object* self);


/** Assigns an object a class type with a data pointer.
*/
void Object_pushClass(Object* self, const Class* cls, void* data);


/** Returns true if an object has a class.
If so, and dataOut is non-NULL, sets the data pointer.
*/
bool Object_checkClass(const Object* self, const Class* cls, void** dataOut);


void Object_debug(const Object* self);


#ifdef __cplusplus
}
#endif
