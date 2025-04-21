#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <Object.h>


/** Reference-counted shared object.

Call SharedObject_obtain() to claim shared ownership of the object.
When the last remaining owner gives up its ownership by calling SharedObject_release(), the object is freed.

There is no need to call SharedObject_specialize().
SharedObject_obtain() automatically specializes the Object and increments its reference counter.
*/
DECLARE_CLASS(SharedObject, ());
DECLARE_FUNCTION_CONST(SharedObject, obtain, void, ());
DECLARE_FUNCTION_CONST(SharedObject, release, void, ());
DECLARE_FUNCTION_CONST(SharedObject, count_get, uint64_t, ());
