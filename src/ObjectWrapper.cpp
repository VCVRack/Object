#include <Object/ObjectWrapper.h>
#include <assert.h>
#include <unordered_map>


struct WrapperData {
	void* wrapper;
	ObjectWrapper_destructor_f* destructor;
};


struct ObjectWrapper {
	std::unordered_map<const void*, WrapperData> wrappers;
};


DEFINE_CLASS(ObjectWrapper, (), (), {
	ObjectWrapper* data = new ObjectWrapper;
	PUSH_CLASS(self, ObjectWrapper, data);
}, {
	while (!data->wrappers.empty()) {
		auto it = data->wrappers.begin();
		it->second.destructor(it->second.wrapper);
	}
	delete data;
})


DEFINE_METHOD(ObjectWrapper, wrapper_set, void, (const void* context, void* wrapper, ObjectWrapper_destructor_f* destructor), VOID, {
	if (wrapper) {
		auto result = data->wrappers.insert({context, {wrapper, destructor}});
		// Key must not already exist
		assert(result.second);
	}
	else {
		data->wrappers.erase(context);
	}
})


DEFINE_METHOD_CONST(ObjectWrapper, wrapper_get, void*, (const void* context), NULL, {
	auto it = data->wrappers.find(context);
	if (it == data->wrappers.end())
		return NULL;
	return it->second.wrapper;
})
