#pragma once

#include "Object.h"


CLASS(ObjectWrapper, ());
typedef void ObjectWrapper_destructor_f(void* wrapper);
METHOD(ObjectWrapper, wrapper_set, void, (const void* context, void* wrapper, ObjectWrapper_destructor_f* destructor));
METHOD_CONST(ObjectWrapper, wrapper_get, void*, (const void* context));

