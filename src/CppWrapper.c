#include <Object/CppWrapper.h>
#include <stdlib.h> // for calloc and free
#include <assert.h>


struct CppWrapper {
	ObjectWrapper* wrapper;
	CppWrapper_destructor_f* destructor;
};


DEFINE_CLASS(CppWrapper, (), (), {
	CppWrapper* data = calloc(1, sizeof(CppWrapper));
	PUSH_CLASS(self, CppWrapper, data);
}, {
	// `wrapper` will be NULL if it was already deleted
	if (data->destructor && data->wrapper)
		data->destructor(data->wrapper);
	free(data);
})


DEFINE_ACCESSOR(CppWrapper, wrapper, ObjectWrapper*, NULL, {
	return data->wrapper;
}, {
	// Must not overwrite existing wrapper
	assert(!(wrapper && data->wrapper));
	data->wrapper = wrapper;
})


DEFINE_ACCESSOR_AUTOMATIC(CppWrapper, destructor, CppWrapper_destructor_f*, NULL)
