#pragma once
#include <cstddef>
#include <cstdint>


template<typename K, typename V, typename SizeT = uint16_t>
struct FlatMap {
	struct Entry {
		K key;
		V value;
	};

	Entry* table = NULL;
	SizeT capacity = 0;
	SizeT mask = 0;
	SizeT size = 0;

	FlatMap() {
		clear();
	}

	~FlatMap() {
		delete[] table;
	}

	void clear() {
		if (capacity == 4) {
			// Clear table
			for (SizeT i = 0; i < capacity; i++) {
				table[i] = {};
			}
		}
		else {
			// Reallocate table
			delete[] table;
			capacity = 4;
			mask = capacity - 1;
			table = new Entry[capacity]();
		}
		size = 0;
	}

	SizeT hash(const K& key) const {
		// key * (2^64 / golden ratio), modulo capacity
		return (uint64_t(key) * 0x9E3779B97F4A7C15ULL) & mask;
	}

	void rehash(SizeT newCapacity) {
		SizeT oldCapacity = capacity;
		Entry* oldTable = table;

		capacity = newCapacity;
		mask = newCapacity - 1;
		table = new Entry[capacity]();
		size = 0;

		for (SizeT i = 0; i < oldCapacity; i++) {
			if (oldTable[i].key)
				insert(oldTable[i].key, oldTable[i].value);
		}
		delete[] oldTable;
	}

	/** Inserts or updates a key-value pair.
	Key must be nonzero.
	*/
	void insert(const K& key, const V& value) {
		// Grow at 50% load factor
		if (size * 2 >= capacity)
			rehash(capacity * 2);

		// Probe until we find the key or an empty slot
		SizeT i = hash(key);
		while (table[i].key && table[i].key != key) {
			i = (i + 1) & mask;
		}

		if (!table[i].key)
			size++;
		table[i] = {key, value};
	}

	/** Returns a pointer to the value for a key, or NULL if not found.
	The pointer can be used to update the value in place.
	*/
	V* find(const K& key) const {
		SizeT i = hash(key);
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

	/** Removes a key-value pair.
	Does nothing if the key is not found.
	*/
	void erase(const K& key) {
		// Find the entry
		for (SizeT i = hash(key); table[i].key; i = (i + 1) & mask) {
			if (table[i].key == key) {
				size--;
				// Shift later entries backward if they belong before their current position
				for (SizeT j = (i + 1) & mask; table[j].key; j = (j + 1) & mask) {
					SizeT home = hash(table[j].key);
					// Check if the gap at i is between home and j (cyclically)
					if (((i - home) & mask) < ((j - home) & mask)) {
						table[i] = table[j];
						i = j;
					}
				}
				// Zero the slot after the cluster
				table[i] = {};
				return;
			}
		}
	}
};
