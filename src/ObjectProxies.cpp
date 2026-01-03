#include <Object/ObjectProxies.h>
#include <assert.h>
#include <vector>
#include <unordered_map>


struct ProxyData {
	void* proxy;
	const void* type;
	ObjectProxies_destructor_f* destructor;
};


struct ObjectProxies {
	std::vector<ProxyData> proxies;
	// type -> proxy
	std::unordered_map<const void*, void*> proxiesByType;
};


DEFINE_CLASS(ObjectProxies, (), (), {
	ObjectProxies* data = new ObjectProxies;
	PUSH_CLASS(self, ObjectProxies, data);
}, {
	// Iterate, destroy, and erase proxies
	// Note that while we delete proxies with the same type, get() will always return the last one, so we are safe to delete old ones first.
	for (const ProxyData& wd : data->proxies) {
		if (wd.destructor)
			wd.destructor(wd.proxy);
	}
	delete data;
})


DEFINE_METHOD(ObjectProxies, add, void, (void* proxy, const void* type, ObjectProxies_destructor_f* destructor), VOID, {
	if (!proxy)
		return;
	data->proxies.push_back({proxy, type, destructor});
	if (type) {
		data->proxiesByType[type] = proxy;
	}
})


DEFINE_METHOD(ObjectProxies, remove, void, (void* proxy), VOID, {
	// Remove from proxies
	for (auto it = data->proxies.begin(); it != data->proxies.end();) {
		if (it->proxy == proxy)
			it = data->proxies.erase(it);
		else
			++it;
	}
	// Remove from proxiesByType
	for (auto it = data->proxiesByType.begin(); it != data->proxiesByType.end();)
		if (it->second == proxy)
			it = data->proxiesByType.erase(it);
		else
			++it;
})


DEFINE_METHOD_CONST(ObjectProxies, get, void*, (const void* type), NULL, {
	auto it = data->proxiesByType.find(type);
	if (it == data->proxiesByType.end())
		return NULL;
	return it->second;
})
