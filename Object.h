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


// Declaration macros


#define DECLARE_CLASS(CLASS, INITARGS) \
	extern const Type CLASS##_type; \
	Object* CLASS##_create(EXPAND INITARGS); \
	void CLASS##_init(Object* self COMMA_EXPAND INITARGS);


#define DECLARE_FUNCTION(CLASS, METHOD, RETTYPE, ARGTYPES) \
	RETTYPE CLASS##_##METHOD(Object* self COMMA_EXPAND ARGTYPES);


/**
CLASS is the class name such as Animal.
METHOD is the method name such as speak.
RETTYPE is the return value such as void.
ARGTYPES are the arguments such as (const char* name, int loudness).

Example:
	DECLARE_METHOD(Animal, speak, void, (const char* name))

Creates the following functions:
	Animal_speak_m Animal_speak_mget(const Object* self);
	void Animal_speak_mset(const Object* self, Animal_speak_m m);
	void Animal_speak(const Object* self, const char* name, int loudness);
	void Animal_speak_mdirect(const Object* self, const char* name, int loudness);
*/
#define DECLARE_METHOD(CLASS, METHOD, RETTYPE, ARGTYPES) \
	/* Method function pointer */ \
	typedef RETTYPE (*CLASS##_##METHOD##_m)(Object* self COMMA_EXPAND ARGTYPES); \
	/* Method getter */ \
	CLASS##_##METHOD##_m CLASS##_##METHOD##_mget(const Object* self); \
	/* Method setter */ \
	void CLASS##_##METHOD##_mset(Object* self, CLASS##_##METHOD##_m m); \
	/* Virtual dispatch */ \
	RETTYPE CLASS##_##METHOD(Object* self COMMA_EXPAND ARGTYPES); \
	/* Non-virtual call */ \
	RETTYPE CLASS##_##METHOD##_mdirect(Object* self COMMA_EXPAND ARGTYPES);


#define DECLARE_METHOD_CONST(CLASS, METHOD, RETTYPE, ARGTYPES) \
	typedef RETTYPE (*CLASS##_##METHOD##_m)(const Object* self COMMA_EXPAND ARGTYPES); \
	CLASS##_##METHOD##_m CLASS##_##METHOD##_mget(const Object* self); \
	void CLASS##_##METHOD##_mset(Object* self, CLASS##_##METHOD##_m m); \
	RETTYPE CLASS##_##METHOD(const Object* self COMMA_EXPAND ARGTYPES); \
	RETTYPE CLASS##_##METHOD##_mdirect(const Object* self COMMA_EXPAND ARGTYPES);


/** Declares two methods called *_get and *_set
*/
#define DECLARE_PROPERTY(CLASS, PROP, TYPE) \
	DECLARE_METHOD_CONST(CLASS, PROP##_get, TYPE, ()) \
	DECLARE_METHOD(CLASS, PROP##_set, void, (TYPE PROP))


#define DECLARE_METHOD_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES) \
	RETTYPE CLASS##_##METHOD##_mdirect(Object* self COMMA_EXPAND ARGTYPES);


#define DECLARE_METHOD_CONST_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES) \
	RETTYPE CLASS##_##METHOD##_mdirect(const Object* self COMMA_EXPAND ARGTYPES);


#define DECLARE_PROPERTY_OVERRIDE(CLASS, PROP, TYPE) \
	DECLARE_METHOD_CONST_OVERRIDE(CLASS, PROP##_get, TYPE, ()) \
	DECLARE_METHOD_OVERRIDE(CLASS, PROP##_set, void, (TYPE PROP))


#define STORE_METHOD(CLASS, METHOD) \
	CLASS##_##METHOD##_m METHOD


#define STORE_PROPERTY(CLASS, PROP) \
	STORE_METHOD(CLASS, PROP##_get); \
	STORE_METHOD(CLASS, PROP##_set)


// Definition macros


#define DEFINE_CLASS(CLASS, INITARGS, INITARGNAMES, INIT, FREE) \
	Object* CLASS##_create(EXPAND INITARGS) { \
		Object* self = Object_create(); \
		CLASS##_init(self COMMA_EXPAND INITARGNAMES); \
		return self; \
	} \
	void CLASS##_init(Object* self COMMA_EXPAND INITARGS) { \
		if (!self || Object_isType(self, &CLASS##_type)) \
			return; \
		INIT \
	} \
	void CLASS##_free(Object* self) { \
		if (!self) \
			return; \
		CLASS* data __attribute__((unused)) = (CLASS*) Object_getData(self, &CLASS##_type); \
		FREE \
	} \
	const Type CLASS##_type = { \
		#CLASS, \
		CLASS##_free, \
		NULL, \
		{} \
	};


/** Class functions (aka non-virtual methods) cannot be overridden.
Upgrading a class function to a class method creates linker symbols and does not break the ABI.
Downgrading a method to a function removes linker symbols and therefore breaks the ABI.
*/
#define DEFINE_FUNCTION(CLASS, METHOD, RETTYPE, ARGTYPES, CODE) \
	RETTYPE CLASS##_##METHOD(Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data __attribute__((unused)) = (CLASS*) Object_getData(self, &CLASS##_type); \
		CODE \
	}


#define DEFINE_FUNCTION_CONST(CLASS, METHOD, RETTYPE, ARGTYPES, CODE) \
	RETTYPE CLASS##_##METHOD(const Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data __attribute__((unused)) = (CLASS*) Object_getData(self, &CLASS##_type); \
		CODE \
	}


#define DEFINE_METHOD(CLASS, METHOD, RETTYPE, ARGTYPES, ARGNAMES, CODE) \
	/* Method getter */ \
	CLASS##_##METHOD##_m CLASS##_##METHOD##_mget(const Object* self) { \
		CLASS* data = (CLASS*) Object_getData(self, &CLASS##_type); \
		if (!data) \
			return NULL; \
		return data->METHOD; \
	} \
	/* Method setter */ \
	void CLASS##_##METHOD##_mset(Object* self, CLASS##_##METHOD##_m m) { \
		CLASS* data = (CLASS*) Object_getData(self, &CLASS##_type); \
		if (!data) \
			return; \
		data->METHOD = m; \
	} \
	/* Virtual dispatch */ \
	RETTYPE CLASS##_##METHOD(Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS##_##METHOD##_m m = CLASS##_##METHOD##_mget(self); \
		return m(self COMMA_EXPAND ARGNAMES); \
	} \
	/* Non-virtual call */ \
	RETTYPE CLASS##_##METHOD##_mdirect(Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data __attribute__((unused)) = (CLASS*) Object_getData(self, &CLASS##_type); \
		CODE \
	}


#define DEFINE_METHOD_CONST(CLASS, METHOD, RETTYPE, ARGTYPES, ARGNAMES, CODE) \
	CLASS##_##METHOD##_m CLASS##_##METHOD##_mget(const Object* self) { \
		CLASS* data = (CLASS*) Object_getData(self, &CLASS##_type); \
		if (!data) \
			return NULL; \
		return data->METHOD; \
	} \
	void CLASS##_##METHOD##_mset(Object* self, CLASS##_##METHOD##_m m) { \
		CLASS* data = (CLASS*) Object_getData(self, &CLASS##_type); \
		if (!data) \
			return; \
		data->METHOD = m; \
	} \
	RETTYPE CLASS##_##METHOD(const Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS##_##METHOD##_m m = CLASS##_##METHOD##_mget(self); \
		return m(self COMMA_EXPAND ARGNAMES); \
	} \
	RETTYPE CLASS##_##METHOD##_mdirect(const Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data __attribute__((unused)) = (CLASS*) Object_getData(self, &CLASS##_type); \
		CODE \
	}


#define DEFINE_PROPERTY(CLASS, NAME, TYPE, GETTER, SETTER) \
	DEFINE_METHOD_CONST(CLASS, NAME##_get, TYPE, (), (), GETTER) \
	DEFINE_METHOD(CLASS, NAME##_set, void, (TYPE NAME), (NAME), SETTER)


#define DEFINE_PROPERTY_AUTOMATIC(CLASS, NAME, TYPE, DEFAULT) \
	DEFINE_METHOD_CONST(CLASS, NAME##_get, TYPE, (), (), { \
		if (!data) \
			return DEFAULT; \
		return data->NAME; \
	}) \
	DEFINE_METHOD(CLASS, NAME##_set, void, (TYPE NAME), (NAME), { \
		if (!data) \
			return; \
		data->NAME = NAME; \
	})


// TODO Write DEFINE_PROPERTY_AUTOMATIC which gets/sets from a C/C++ struct

#define DEFINE_METHOD_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES, CODE) \
	RETTYPE CLASS##_##METHOD##_mdirect(Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data __attribute__((unused)) = (CLASS*) Object_getData(self, &CLASS##_type); \
		CODE \
	}


#define DEFINE_METHOD_CONST_OVERRIDE(CLASS, METHOD, RETTYPE, ARGTYPES, CODE) \
	RETTYPE CLASS##_##METHOD##_mdirect(const Object* self COMMA_EXPAND ARGTYPES) { \
		CLASS* data __attribute__((unused)) = (CLASS*) Object_getData(self, &CLASS##_type); \
		CODE \
	}


#define DEFINE_PROPERTY_OVERRIDE(CLASS, PROP, TYPE, GETTER, SETTER) \
	DEFINE_METHOD_CONST_OVERRIDE(CLASS, PROP##_get, TYPE, (), GETTER) \
	DEFINE_METHOD_OVERRIDE(CLASS, PROP##_set, void, (TYPE PROP), SETTER)


// Call macros


// TODO Change name
#define OVERRIDE_METHOD(SELF, CLASS, SUPERCLASS, METHOD) \
	SUPERCLASS##_##METHOD##_mset(SELF, CLASS##_##METHOD##_mdirect)


// TODO Change name
#define OVERRIDE_PROPERTY(SELF, CLASS, SUPERCLASS, PROP) \
	OVERRIDE_METHOD(SELF, CLASS, SUPERCLASS, PROP##_get); \
	OVERRIDE_METHOD(SELF, CLASS, SUPERCLASS, PROP##_set)


#define INIT_CLASS(SELF, CLASS) \
	CLASS##_init(SELF)


#define PUSH_TYPE(SELF, CLASS, DATA) \
	Object_pushType(SELF, &CLASS##_type, DATA)


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

typedef struct Type {
	const char* name;
	/** Frees data pointer and its contents, if non-NULL.
	Must not call virtual methods.
	Called by Object_free() for each type in reverse order.
	*/
	Object_free_m free;
	/** Prepares object to be freed, if non-NULL.
	Called by Object_free() for each type in reverse order.
	*/
	Object_finalize_m finalize;
	// Reserved for future fields. Must be zero.
	// Possibly `getDataSize`
	void* reserved[29];
} Type;


/** Creates an object with no type */
Object* Object_create();

/** Deletes the object and its internal data for each type.
Calls finalize() for each type in reverse order, and then free() in reverse order.
*/
void Object_free(Object* self);

void Object_pushType(Object* self, const Type* type, void* data);

/**
Pure so it can be optimized out if return value is not used.
*/
__attribute__((pure))
bool Object_isType(const Object* self, const Type* type);

__attribute__((pure))
void* Object_getData(const Object* self, const Type* type);


#ifdef __cplusplus
}
#endif
