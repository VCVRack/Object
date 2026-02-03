#pragma once
#include <cstddef>
#include <cstdint>


template<typename K, typename V>
struct FlatMap {
	struct Entry {
		K key;
		V value;
	};

	Entry* table;
	uint16_t capacity = 4;
	uint16_t mask = 3;
	uint16_t size = 0;

	FlatMap() {
		table = new Entry[capacity]();
	}

	~FlatMap() {
		delete[] table;
	}

	uintptr_t hash(const K& key) const {
		return (uintptr_t(key) * 0x9E3779B97F4A7C15ULL) & mask;
	}

	void rehash(uint16_t newCapacity) {
		uint16_t oldCapacity = capacity;
		Entry* oldTable = table;

		capacity = newCapacity;
		mask = newCapacity - 1;
		table = new Entry[capacity]();
		size = 0;

		for (size_t i = 0; i < oldCapacity; i++) {
			if (oldTable[i].key)
				insert(oldTable[i].key, oldTable[i].value);
		}
		delete[] oldTable;
	}

	void insert(const K& key, const V& value) {
		// Grow at 50% load factor.
		if (size * 2 >= capacity)
			rehash(capacity * 2);

		size_t i = hash(key);
		while (table[i].key && table[i].key != key)
			i = (i + 1) & mask;

		if (!table[i].key)
			size++;
		table[i].key = key;
		table[i].value = value;
	}

	V* find(const K& key) const {
		size_t i = hash(key);
		while (table[i].key) {
			if (table[i].key == key)
				return &table[i].value;
			i = (i + 1) & mask;
		}
		return NULL;
	}

	bool empty() const {
		return size == 0;
	}

	void erase(const K& key) {
		size_t i = hash(key);
		while (table[i].key) {
			if (table[i].key == key) {
				table[i].key = K{};
				table[i].value = V{};
				size--;
				rehash(capacity);
				return;
			}
			i = (i + 1) & mask;
		}
	}
};
