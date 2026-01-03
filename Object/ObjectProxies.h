#pragma once

#include "Object.h"


/** Stores references to (potentially) multiple ObjectProxy objects in (potentially) multiple language environments.
*/
CLASS(ObjectProxies, ());

typedef void ObjectProxies_destructor_f(void* proxy);
/** When a proxy is created for this object, add it to associate it with the Object.
If `destructor` is non-null, it is called when the Object is freed, unless removed with remove().
`type` should typically be a memory address (such as `&typeid(ObjectProxy)` in C++) to ensure uniqueness across languages.
*/
METHOD(ObjectProxies, add, void, (void* proxy, const void* type, ObjectProxies_destructor_f* destructor));
/** Remove by proxy pointer. */
METHOD(ObjectProxies, remove, void, (void* proxy));
/** Get proxy pointer by type. */
METHOD_CONST(ObjectProxies, get, void*, (const void* type));
