#include <Object/CppWrapper.h>
#include <stdlib.h> // for calloc and free


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


DEFINE_ACCESSOR_AUTOMATIC(CppWrapper, wrapper, ObjectWrapper*, NULL)
DEFINE_ACCESSOR_AUTOMATIC(CppWrapper, destructor, CppWrapper_destructor_f*, NULL)
