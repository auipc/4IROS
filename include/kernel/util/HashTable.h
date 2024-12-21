#pragma once
#include <kernel/mem/malloc.h>
#include <stddef.h>
#include <stdint.h>

#define INITIAL_CONTAINER_SIZE (1 << 4)
#define INITIAL_CONTAINER_BITS 4

template <typename T> class HashTable {
  public:
	HashTable() {
		m_buckets = new Bucket[INITIAL_CONTAINER_SIZE];
		memset(m_buckets, 0, sizeof(T) * INITIAL_CONTAINER_SIZE);
	}

	~HashTable() {
		for (size_t i = 0; i < m_container_size; i++) {
			Bucket *bucket = &m_buckets[i];
			if (!bucket->filled)
				continue;

			bucket = bucket->next;
			while (bucket) {
				Bucket *next = bucket->next;
				delete bucket;
				bucket = next;
			}
		}
		delete[] m_buckets;
	}

	HashTable(const HashTable &ht) {
		m_container_bits = ht.m_container_bits;
		m_container_size = ht.m_container_size;
		m_buckets = new Bucket[m_container_size];
		memset(m_buckets, 0, sizeof(T) * m_container_size);

		m_true_no = ht.m_true_no;
		for (size_t i = 0; i < m_container_size; i++) {
			Bucket *bucket = &ht.m_buckets[i];
			if (!bucket->filled)
				continue;

			add(m_buckets, bucket->key, bucket->val, m_container_bits);
			bucket = bucket->next;
			while (bucket) {
				Bucket *next = bucket->next;
				add(m_buckets, bucket->key, bucket->val, m_container_bits);
				delete bucket;
				bucket = next;
			}
		}
	}

	void shrinkwrap() {
		size_t old_container_bits = m_container_bits;
		m_container_bits = __builtin_clzll(m_true_no);
		m_container_size = 1 << m_container_bits;
		Bucket *bucket_temp = new Bucket[m_container_size];
		rehash_all(bucket_temp, old_container_bits);
		delete[] m_buckets;
		m_buckets = bucket_temp;
	}

	void accommodate(size_t size) {
		if (size <= m_container_size) {
			return;
		}
		size_t old_container_bits = m_container_bits;
		m_container_bits = __builtin_clzll(size);
		m_container_size = 1 << m_container_bits;
		Bucket *bucket_temp = new Bucket[m_container_size];
		rehash_all(bucket_temp, old_container_bits);
		delete[] m_buckets;
		m_buckets = bucket_temp;
	}

	void copy(const HashTable &ht, bool resolve_conflict = false) {
		m_true_no += ht.m_true_no;
		// accommodate(m_container_size+ht.m_container_size);

		for (size_t i = 0; i < ht.m_container_size; i++) {
			Bucket *bucket = &ht.m_buckets[i];
			if (!bucket->filled)
				continue;

			if (get(bucket->key)) {
				if (!resolve_conflict)
					panic("Help");
				continue;
			}

			add(m_buckets, bucket->key, bucket->val, m_container_bits);
			bucket = bucket->next;
			while (bucket) {
				Bucket *next = bucket->next;
				add(m_buckets, bucket->key, bucket->val, m_container_bits);
				delete bucket;
				bucket = next;
			}
		}
	}

	void copy(const HashTable &ht) {
		m_true_no = ht.m_true_no;
		for (size_t i = 0; i < ht.m_container_size; i++) {
			Bucket *bucket = &ht.m_buckets[i];
			if (!bucket->filled)
				continue;

			add(m_buckets, bucket->key, bucket->val, m_container_bits);
			bucket = bucket->next;
			while (bucket) {
				Bucket *next = bucket->next;
				add(m_buckets, bucket->key, bucket->val, m_container_bits);
				delete bucket;
				bucket = next;
			}
		}
	}

	void push(const uint64_t key, T value) {
		if (m_true_no > m_container_size) {
			size_t old_container_bits = m_container_bits;
			m_container_bits++;
			m_container_size = 1 << m_container_bits;
			Bucket *bucket_temp = new Bucket[m_container_size];
			rehash_all(bucket_temp, old_container_bits);
			delete[] m_buckets;
			m_buckets = bucket_temp;
		}
		uint64_t h = hash((char *)&key, sizeof(key));
		uint64_t map = fib_map(h, m_container_bits);
		Bucket *b = &m_buckets[map];
		if (b->filled) {
			while (b->next) {
				assert(b->key != key);
				b = b->next;
			}
			b->next = new Bucket{true, value, key, nullptr};
		} else {
			m_buckets[map] = Bucket{true, value, key, nullptr};
		}
		m_true_no++;
	}

	void remove(const uint64_t key) {
		uint64_t h = hash((char *)&key, sizeof(key));
		uint64_t map = fib_map(h, m_container_bits);
		if (m_buckets[map].key != key) {
			if (!m_buckets[map].next)
				return;
			Bucket *prev = &m_buckets[map];
			Bucket *b = m_buckets[map].next;
			while (b) {
				if (b->key == key)
					break;
				prev = b;
				b = b->next;
			}
			if (!b)
				return;
			prev->next = b->next;
			delete b;
			m_true_no--;
			return;
		}
		if (m_buckets[map].next) {
			auto n = m_buckets[map].next;
			m_buckets[map] = *n;
			delete n;
		} else
			m_buckets[map].filled = false;
		m_true_no--;
#if 0
		// FIXME use realloc, and don't rehash if the larger map somehow equals the smaller map everything.
		if (m_true_no <= (1<<(m_container_bits-1))) {
			size_t old_container_bits = m_container_bits;
			m_container_bits++;
			m_container_size = 1<<m_container_bits;
			Bucket* bucket_temp = new Bucket[m_container_size];
			rehash_all(bucket_temp, old_container_bits);
			delete[] m_buckets;
			m_buckets = bucket_temp;
		}
#endif
	}

	template <typename F> void iterate(F f) {
		for (size_t i = 0; i < m_container_size; i++) {
			auto b = internal_get(i);
			if (!b)
				continue;
			f(b->key, &b->val);
		}
	}

	T *get(const uint64_t key) {
		auto b = internal_get(key);
		if (!b)
			return nullptr;
		return &b->val;
	}

	uint64_t size() { return m_container_size; }

  private:
	struct Bucket {
		bool filled = false;
		T val;
		uint64_t key = -1;
		Bucket *next = nullptr;
	};

	Bucket *internal_get(const uint64_t key) {
		uint64_t h = hash((char *)&key, sizeof(key));
		uint64_t map = fib_map(h, m_container_bits);
		Bucket *b = &m_buckets[map];
		if (!b->filled) {
			return nullptr;
		}
		while (b->key != key) {
			if (!b->next) {
				return nullptr;
			}
			b = b->next;
		}
		return b;
	}

	// FNV-1a
	static uint64_t hash(char *val, size_t n) {
		uint64_t h = 0xcbf29ce484222325ull;
		for (size_t i = 0; i < n; i++) {
			h ^= val[i];
			h *= 0x100000001b3ull;
		}
		return h;
	}

	// https://probablydance.com/2018/06/16/fibonacci-hashing-the-optimization-that-the-world-forgot-or-a-better-alternative-to-integer-modulo/
	static uint64_t fib_map(uint64_t hash, size_t shift) {
		// hash ^= hash >> (64-shift);
		return (hash * 11400714819323198485llu) >> (64 - shift);
	}

	static void add(Bucket *buckets, const uint64_t key, T value,
					size_t container_bits) {
		uint64_t h = hash((char *)&key, sizeof(key));
		uint64_t map = fib_map(h, container_bits);
		Bucket *b = &buckets[map];
		if (b->filled) {
			while (b->next) {
				assert(b->key != key);
				b = b->next;
			}
			b->next = new Bucket{true, value, key, nullptr};
		} else {
			buckets[map] = Bucket{true, value, key, nullptr};
		}
	}

	void rehash_all(Bucket *buckets, size_t old_container_bits) {
		for (size_t i = 0; i < (1 << old_container_bits); i++) {
			Bucket *bucket = &m_buckets[i];
			if (!bucket->filled)
				continue;
			// uint64_t h = hash((char*)bucket->key, sizeof(bucket->key));
			// uint64_t map = fib_map(h,old_container_bits);

			add(buckets, bucket->key, bucket->val, m_container_bits);
			if (bucket->next) {
				bucket = bucket->next;
				while (bucket) {
					Bucket *next = bucket->next;
					add(buckets, bucket->key, bucket->val, m_container_bits);
					delete bucket;
					bucket = next;
				}
			}
		}
	}

	Bucket *m_buckets;
	size_t m_true_no = 0;
	size_t m_container_size = INITIAL_CONTAINER_SIZE;
	size_t m_container_bits = INITIAL_CONTAINER_BITS;
};
