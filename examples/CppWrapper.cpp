#include "CppWrapper.h"
// #include <stdio.h>


struct CppWrapper {
	ObjectWrapper* wrapper;
};


DEFINE_CLASS(CppWrapper, (ObjectWrapper* wrapper), (wrapper), {
	CppWrapper* data = new CppWrapper;
	data->wrapper = wrapper;
	PUSH_CLASS(self, CppWrapper, data);
}, {
	// printf("bye CppWrapper\n");
	// Will be NULL if ObjectWrapper was already deleted
	delete data->wrapper;
	delete data;
})


DEFINE_ACCESSORS_FUNCTION(CppWrapper, wrapper, ObjectWrapper*, NULL, {
	return data->wrapper;
}, {
	data->wrapper = wrapper;
})
