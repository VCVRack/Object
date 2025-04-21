#include "CppWrapper.h"


DEFINE_CLASS(CppWrapper, (void* ptr), (ptr), {
	// Since ptr's lifetime equals this Object's lifetime, store it directly as data instead of wrapping in a struct.
	PUSH_CLASS(self, CppWrapper, ptr);
}, {
	ObjectWrapper* ow = (ObjectWrapper*) data;
	if (!ow)
		return;
	// If C++ class's `self` reference is NULL, this signals that it will be deleted after Object_free() returns.
	if (!ow->self)
		return;
	// Request to not call Object_free() again since we are already doing it.
	ow->self = NULL;
	delete ow;
})


DEFINE_FUNCTION_CONST(CppWrapper, ptr_get, void*, NULL, (), {
	return data;
})
