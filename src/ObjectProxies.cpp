#include <Object/ObjectProxies.h>
#include <assert.h>
#include <vector>
#include <unordered_map>


struct ProxyData {
	void* proxy = NULL;
	const void* type = NULL;
	ObjectProxies_destructor_f* destructor = NULL;
};


struct ObjectProxies {
	ProxyData boundProxy;
	std::vector<ProxyData> proxies;
	// type -> proxy
	std::unordered_map<const void*, void*> proxiesByType;
};


DEFINE_CLASS(ObjectProxies, (), (), {
	ObjectProxies* data = new ObjectProxies;
	PUSH_CLASS(self, ObjectProxies, data);
}, {
	// Destroy proxies in reverse order to allow re-entrant remove() calls
	while (!data->proxies.empty()) {
		ProxyData p = data->proxies.back();
		// Remove proxy before calling destructor
		data->proxies.pop_back();
		if (p.destructor) {
			p.destructor(p.proxy);
		}
	}
	// Destroy bound proxy if exists
	ProxyData& bp = data->boundProxy;
	if (bp.proxy && bp.destructor) {
		bp.destructor(bp.proxy);
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


DEFINE_METHOD_CONST(ObjectProxies, bound_get, void*, (const void** type), NULL, {
	const ProxyData& bp = data->boundProxy;
	if (type)
		*type = bp.type;
	return bp.proxy;
})

DEFINE_METHOD(ObjectProxies, bound_set, void, (void* proxy, const void* type, ObjectProxies_destructor_f* destructor), VOID, {
	ProxyData& bp = data->boundProxy;
	bp.proxy = proxy;
	bp.type = type;
	bp.destructor = destructor;
})
