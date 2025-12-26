#include <Object/ObjectWrapper.h>
#include <assert.h>
#include <vector>
#include <unordered_map>


struct WrapperData {
	void* wrapper;
	const void* type;
	ObjectWrapper_destructor_f* destructor;
};


struct ObjectWrapper {
	std::vector<WrapperData> wrappers;
	// type -> wrapper
	std::unordered_map<const void*, void*> wrappersByType;
};


DEFINE_CLASS(ObjectWrapper, (), (), {
	ObjectWrapper* data = new ObjectWrapper;
	PUSH_CLASS(self, ObjectWrapper, data);
}, {
	// Iterate, destroy, and erase wrappers
	// Note that while we delete wrappers with the same type, wrapper_get() will always return the last one, so we are safe to delete old ones first.
	for (const WrapperData& wd : data->wrappers) {
		if (wd.destructor)
			wd.destructor(wd.wrapper);
	}
	delete data;
})


DEFINE_METHOD(ObjectWrapper, wrapper_add, void, (void* wrapper, const void* type, ObjectWrapper_destructor_f* destructor), VOID, {
	if (!wrapper)
		return;
	data->wrappers.push_back({wrapper, type, destructor});
	if (type) {
		data->wrappersByType[type] = wrapper;
	}
})


DEFINE_METHOD(ObjectWrapper, wrapper_remove, void, (void* wrapper), VOID, {
	// Remove from wrappers
	for (auto it = data->wrappers.begin(); it != data->wrappers.end();) {
		if (it->wrapper == wrapper)
			it = data->wrappers.erase(it);
		else
			++it;
	}
	// Remove from wrappersByType
	for (auto it = data->wrappersByType.begin(); it != data->wrappersByType.end();)
		if (it->second == wrapper)
			it = data->wrappersByType.erase(it);
		else
			++it;
})


DEFINE_METHOD_CONST(ObjectWrapper, wrapper_get, void*, (const void* type), NULL, {
	auto it = data->wrappersByType.find(type);
	if (it == data->wrappersByType.end())
		return NULL;
	return it->second;
})
