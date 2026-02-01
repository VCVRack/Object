#pragma once

#include "Object.h"


/** Stores references to (potentially) multiple ObjectProxy objects in (potentially) multiple language environments.
*/
CLASS(ObjectProxies, ());

typedef void ObjectProxies_destructor_f(void* proxy);

/** When a non-bound proxy is created for this object, add it to associate it with the Object.
If `destructor` is non-null, it is called when the Object is freed, unless removed with remove().
`type` should be the derived proxy type (e.g. `&typeid(Animal)` in C++) to allow lookup by specific type. Other languages should use a meaningful memory address to represent the type, to ensure uniqueness of the type across multiple languages.
*/
METHOD(ObjectProxies, add, void, (void* proxy, const void* type, ObjectProxies_destructor_f* destructor));

/** Removes a proxy. */
METHOD(ObjectProxies, remove, void, (void* proxy));

/** Gets proxy pointer by type. */
METHOD_CONST(ObjectProxies, get, void*, (const void* type));

/** Gets the unique bound proxy pointer, if exists.
If `type` is non-NULL, sets it to the bound proxy's type.
*/
METHOD_CONST(ObjectProxies, bound_get, void*, (const void** type));

/** Sets the bound proxy pointer and its type ID.
For the C++ ObjectProxy, you may use `&typeid(ObjectProxy)` as the type, since ObjectProxy::of() uses dynamic_cast on the bound proxy pointer.
*/
METHOD(ObjectProxies, bound_set, void, (void* proxy, const void* type, ObjectProxies_destructor_f* destructor));
