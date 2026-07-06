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
	ObjectProxies* slot = new ObjectProxies;
	CLASS_PUSH(self, ObjectProxies, slot);
}, {
	// Destroy proxies in reverse order to allow re-entrant remove() calls
	while (!slot->proxies.empty()) {
		ProxyData p = slot->proxies.back();
		// Remove proxy before calling destructor
		slot->proxies.pop_back();
		if (p.destructor) {
			p.destructor(p.proxy);
		}
	}
	// Destroy bound proxy if exists
	ProxyData& bp = slot->boundProxy;
	if (bp.proxy && bp.destructor) {
		bp.destructor(bp.proxy);
	}
	delete slot;
})


DEFINE_METHOD(ObjectProxies, add, void, (void* proxy, const void* type, ObjectProxies_destructor_f* destructor), VOID, {
	if (!proxy)
		return;
	slot->proxies.push_back({proxy, type, destructor});
	if (type) {
		slot->proxiesByType[type] = proxy;
	}
})


DEFINE_METHOD(ObjectProxies, remove, void, (void* proxy), VOID, {
	// Remove from proxies
	for (auto it = slot->proxies.begin(); it != slot->proxies.end();) {
		if (it->proxy == proxy)
			it = slot->proxies.erase(it);
		else
			++it;
	}
	// Remove from proxiesByType
	for (auto it = slot->proxiesByType.begin(); it != slot->proxiesByType.end();)
		if (it->second == proxy)
			it = slot->proxiesByType.erase(it);
		else
			++it;
})


DEFINE_METHOD_CONST(ObjectProxies, get, void*, (const void* type), NULL, {
	auto it = slot->proxiesByType.find(type);
	if (it == slot->proxiesByType.end())
		return NULL;
	return it->second;
})


DEFINE_METHOD_CONST(ObjectProxies, bound_get, void*, (const void** type), NULL, {
	const ProxyData& bp = slot->boundProxy;
	if (type)
		*type = bp.type;
	return bp.proxy;
})

DEFINE_METHOD(ObjectProxies, bound_set, void, (void* proxy, const void* type, ObjectProxies_destructor_f* destructor), VOID, {
	ProxyData& bp = slot->boundProxy;
	bp.proxy = proxy;
	bp.type = type;
	bp.destructor = destructor;
})
