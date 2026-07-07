#pragma once
#include <cstddef>
#include <cstdint>
#include <algorithm>


/** Hash map with a perfect hash function, so every lookup reads exactly one table entry.

build() searches for multiplier seeds that map every key to a distinct table entry.
This follows PTHash (https://arxiv.org/abs/2104.10402), which modernized the constructions of Fox, Chen, and Heath (https://doi.org/10.1145/133160.133209) and the compress-hash-displace algorithm (https://cmph.sourceforge.net/papers/esa09.pdf).
*/
template <typename K, typename V>
struct PerfectHashMap {
	struct Entry {
		K key;
		V value;
	};

	/** Key counts up to this use one seed for the whole key set, and larger counts give each bucket its own seed. */
	static const uint32_t singleSeedKeyLimit = 128;
	/** Seed trials before giving up on a single seed. */
	static const uint32_t singleSeedTrialLimit = 1 << 15;
	/** Seed trials before doubling the table in bucketed mode. */
	static const uint32_t bucketTrialLimit = 1 << 17;

	/** Single multiplier seed for the whole map, or 0 when per-bucket seeds are in use. */
	uint64_t singleSeed = 0;
	/** One multiplier seed per bucket, or NULL in single-seed mode.
	An empty bucket keeps seed 0, which sends lookups to table entry 0 where the key comparison fails.
	*/
	uint64_t* seeds = NULL;
	/** Entry array of power-of-two length. An entry with key 0 is empty. */
	Entry* table = NULL;
	/** Right shift that turns a key hash into a bucket index, equal to `64 - log2(bucketCount)`. */
	uint8_t bucketShift = 0;
	/** Right shift that turns `key * seed` into a table position, equal to `64 - log2(tableLength)`. */
	uint8_t positionShift = 0;
	/** Number of occupied entries in the table. */
	uint32_t size = 0;

	PerfectHashMap() {
		build(NULL, 0);
	}

	~PerfectHashMap() {
		delete[] seeds;
		delete[] table;
	}

	PerfectHashMap(const PerfectHashMap&) = delete;
	PerfectHashMap& operator=(const PerfectHashMap&) = delete;

	/** Returns a pointer to the value for a key, or NULL if not found.
	The pointer stays valid until the next build() or the map's destruction.
	Thread-safe with other lookups on the same map.
	The key must be nonzero, since key 0 marks an empty table entry.
	*/
	const V* find(const K& key) const {
		uint64_t seed = singleSeed;
		if (!seed)
			seed = seeds[hash(key) >> bucketShift];
		uint64_t position = (uint64_t(key) * seed) >> positionShift;
		const Entry& entry = table[position];
		if (entry.key != key)
			return NULL;
		return &entry.value;
	}

	/** Builds a perfect hash table from an array of entries, replacing the previous contents.
	Keys must be distinct, nonzero, and convertible to uint64_t.
	Not thread-safe with lookups on the same map.
	Deterministic, so the same entries always build the same table.
	*/
	void build(const Entry* entries, uint32_t count) {
		delete[] seeds;
		delete[] table;
		seeds = NULL;
		table = NULL;
		size = count;
		singleSeed = 0;
		bucketShift = 0;

		// Scratch array for tentative placements
		uint64_t* positions = new uint64_t[count];
		uint64_t seedState = 0;

		// Search for a whole-map seed, so lookups skip the bucket step
		if (count <= singleSeedKeyLimit) {
			// A trial succeeds with probability about `exp(-count^2 / (2 * capacity))`, so grow the table until the exponent is at most 8
			uint32_t capacity = 2;
			while (capacity < 2 * count || uint64_t(count) * count > 16 * capacity)
				capacity *= 2;
			positionShift = 64 - __builtin_ctz(capacity);
			table = new Entry[capacity]();

			singleSeed = entries_place(entries, count, positions, seedState, singleSeedTrialLimit);
			if (singleSeed) {
				delete[] positions;
				return;
			}
			delete[] table;
			table = NULL;
		}

		// Bucketed mode
		// Power-of-two bucket count averaging at most 4 keys per bucket
		uint64_t bucketCount = 2;
		while (bucketCount * 4 < count)
			bucketCount *= 2;
		bucketShift = 64 - __builtin_ctzll(bucketCount);
		// Power-of-two table size with at most half of the entries filled
		uint64_t capacity = 2;
		while (capacity < uint64_t(2) * count)
			capacity *= 2;
		positionShift = 64 - __builtin_ctzll(capacity);

		// Count the keys in each bucket
		uint32_t* bucketSizes = new uint32_t[bucketCount]();
		for (uint32_t i = 0; i < count; i++)
			bucketSizes[hash(entries[i].key) >> bucketShift]++;

		// Group entries by bucket, decrementing each bucket's offset from its end to its start
		uint32_t* bucketOffsets = new uint32_t[bucketCount];
		uint32_t offset = 0;
		for (uint32_t b = 0; b < bucketCount; b++) {
			offset += bucketSizes[b];
			bucketOffsets[b] = offset;
		}
		Entry* groupedEntries = new Entry[count];
		for (uint32_t i = 0; i < count; i++) {
			uint32_t b = hash(entries[i].key) >> bucketShift;
			groupedEntries[--bucketOffsets[b]] = entries[i];
		}
		// bucketOffsets[b] is now the start of bucket b in groupedEntries

		// Order buckets from largest to smallest, so the biggest buckets are placed while the table is emptiest
		uint32_t* bucketOrder = new uint32_t[bucketCount];
		for (uint32_t b = 0; b < bucketCount; b++)
			bucketOrder[b] = b;
		std::sort(bucketOrder, bucketOrder + bucketCount, [&](uint32_t a, uint32_t b) {
			if (bucketSizes[a] != bucketSizes[b])
				return bucketSizes[a] > bucketSizes[b];
			return a < b;
		});

		// Retry with a doubled table if any bucket exhausts its seed trials, which is vanishingly rare with distinct keys
		while (true) {
			seeds = new uint64_t[bucketCount]();
			table = new Entry[capacity]();

			bool built = true;
			for (uint32_t i = 0; i < bucketCount; i++) {
				uint32_t b = bucketOrder[i];
				// Buckets are ordered by size, so the remaining buckets are also empty
				if (bucketSizes[b] == 0)
					break;
				uint64_t seed = entries_place(&groupedEntries[bucketOffsets[b]], bucketSizes[b], positions, seedState, bucketTrialLimit);
				if (!seed) {
					built = false;
					break;
				}
				seeds[b] = seed;
			}
			if (built)
				break;

			delete[] seeds;
			delete[] table;
			capacity *= 2;
			positionShift--;
		}

		delete[] bucketOrder;
		delete[] groupedEntries;
		delete[] bucketOffsets;
		delete[] bucketSizes;
		delete[] positions;
	}

	/** Searches up to maxTrials seeds for one that places all of the given entries into distinct empty table entries, and commits the placement.
	Returns the seed, or 0 if the trial limit was exhausted with the table left unchanged.
	*/
	uint64_t entries_place(const Entry* entries, uint32_t count, uint64_t* positions, uint64_t& seedState, uint32_t maxTrials) {
		for (uint32_t trial = 0; trial < maxTrials; trial++) {
			// An odd seed multiplies keys bijectively, so no pair of distinct keys collides under every seed
			uint64_t seed = splitmix64(seedState) | 1;
			uint32_t placed = 0;
			for (; placed < count; placed++) {
				const Entry& entry = entries[placed];
				uint64_t position = (uint64_t(entry.key) * seed) >> positionShift;
				// An occupied entry also catches two of the given keys landing in the same position
				if (table[position].key)
					break;
				table[position] = entry;
				positions[placed] = position;
			}
			if (placed == count)
				return seed;
			// Undo the partial placement
			while (placed > 0)
				table[positions[--placed]] = {};
		}
		return 0;
	}

	/** Scrambles a key so its high bits are well distributed, by Fibonacci hashing. */
	static uint64_t hash(const K& key) {
		// key * (2^64 / golden ratio)
		return uint64_t(key) * 0x9E3779B97F4A7C15ULL;
	}

	/** Returns the next value of the splitmix64 pseudorandom sequence and advances the state.
	https://prng.di.unimi.it/splitmix64.c
	*/
	static uint64_t splitmix64(uint64_t& state) {
		state += 0x9E3779B97F4A7C15ULL;
		uint64_t z = state;
		z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
		z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
		return z ^ (z >> 31);
	}
};
