#pragma once

#include <Object/Object.h>

/*
This file defines the entire ABI of your library.

If it's not written here, it's not part of your ABI.
For example, class hierarchy is not specified here, so it can be changed any time in your implementation without breaking your ABI.
However it's a good idea to document the hierarchy for humans to read.
*/


/** An organic beast, no matter how small. */
CLASS(Animal, ());
METHOD_VIRTUAL_CONST(Animal, speak, void, ());
ACCESSOR_VIRTUAL(Animal, legs, int);
METHOD_CONST(Animal, pet, void, ());


/** A domesticated wolf.
Inherits Animal.
*/
CLASS(Dog, ());
METHOD_CONST_OVERRIDE(Dog, speak, void, ());
ACCESSOR_OVERRIDE(Dog, legs, int);
ACCESSOR_VIRTUAL(Dog, name, const char*);
