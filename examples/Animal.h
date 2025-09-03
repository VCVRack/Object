#pragma once

#include <Object/Object.h>

/*
This file defines the entire ABI of your library.

If it's not written here, it's not part of your ABI.
For example, class hierarchy is not specified here, so it can be changed any time in your implementation without breaking your ABI.
However it's a good idea to document the hierarchy for humans to read.
*/


/** An organic beast, no matter how small. */
DECLARE_CLASS(Animal, ());
DECLARE_METHOD_VIRTUAL_CONST(Animal, speak, void, ());
DECLARE_ACCESSOR_VIRTUAL(Animal, legs, int);
DECLARE_METHOD_CONST(Animal, pet, void, ());


/** A domesticated wolf.
Inherits Animal.
*/
DECLARE_CLASS(Dog, ());
DECLARE_METHOD_CONST_OVERRIDE(Dog, speak, void, ());
DECLARE_ACCESSOR_OVERRIDE(Dog, legs, int);
DECLARE_ACCESSOR_VIRTUAL(Dog, name, const char*);
