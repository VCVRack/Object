#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <Object.h>

/*
Reference counter example.
*/


DECLARE_CLASS(Ref, ());
DECLARE_FUNCTION_CONST(Ref, obtain, void, ());
DECLARE_FUNCTION_CONST(Ref, release, void, ());
DECLARE_FUNCTION_CONST(Ref, ref_get, uint64_t, ());
