#pragma once

#include "Object.h"


/** Stores references to (potentially) multiple wrapper objects in (potential) multiple language environments.
*/
CLASS(ObjectWrapper, ());

typedef void ObjectWrapper_destructor_f(void* wrapper);
/** When a wrapper is created for this object, this associates it with the Object.
If `destructor` is non-null, it is called when the Object is freed, unless removed with wrapper_remove().
`type` should typically be a memory address to ensure uniqueness across languages.
*/
METHOD(ObjectWrapper, wrapper_add, void, (void* wrapper, const void* type, ObjectWrapper_destructor_f* destructor));
/** Remove by wrapper pointer. */
METHOD(ObjectWrapper, wrapper_remove, void, (void* wrapper));
/** Get wrapper pointer by type. */
METHOD_CONST(ObjectWrapper, wrapper_get, void*, (const void* type));
