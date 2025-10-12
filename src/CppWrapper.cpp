#include <Object/CppWrapper.h>
#include <assert.h>
#include <unordered_map>


struct WrapperData {
	void* wrapper;
	CppWrapper_destructor_f* destructor;
};


struct CppWrapper {
	std::unordered_map<const void*, WrapperData> wrappers;
};


DEFINE_CLASS(CppWrapper, (), (), {
	CppWrapper* data = new CppWrapper;
	PUSH_CLASS(self, CppWrapper, data);
}, {
	while (!data->wrappers.empty()) {
		auto it = data->wrappers.begin();
		it->second.destructor(it->second.wrapper);
	}
	delete data;
})


DEFINE_METHOD(CppWrapper, wrapper_set, void, (const void* context, void* wrapper, CppWrapper_destructor_f* destructor), VOID, {
	if (wrapper) {
		auto result = data->wrappers.insert({context, {wrapper, destructor}});
		// Key must not already exist
		assert(result.second);
	}
	else {
		data->wrappers.erase(context);
	}
})


DEFINE_METHOD_CONST(CppWrapper, wrapper_get, void*, (const void* context), NULL, {
	auto it = data->wrappers.find(context);
	if (it == data->wrappers.end())
		return NULL;
	return it->second.wrapper;
})
