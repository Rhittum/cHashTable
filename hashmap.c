#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "rsg.h"

#define HASHMAP_DEFAULT_CAPACITY 32

#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))
#define SIPROUND                    \
	do {                        \
		v0 += v1; v2 += v3; \
		v1 = ROTL(v1, 13);  \
		v3 = ROTL(v3, 16);  \
		v1 ^= v0; v3 ^= v2; \
		v0 = ROTL(v0, 32);  \
		v2 += v1; v0 += v3; \
		v1 = ROTL(v1, 17);  \
		v3 = ROTL(v3, 21);  \
		v1 ^= v2; v3 ^= v0; \
		v2 = ROTL(v2, 32);  \
	} while (0)

typedef enum {
	OCCUPIED,
	EMPTY,
	DELETED
} SlotState;

typedef struct {
	char* key;
	void* value;
	uint32_t psl;
	SlotState state;
} HashEntry;

typedef struct {
	HashEntry* buckets;
	int capacity;
	int size;
	uint64_t secretKey[2];
} HashMap;

uint64_t fmix64(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccd;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53;
    x ^= x >> 33;
    return x;
}

HashEntry setEntry(char* key, void* value, SlotState state) {
	return (HashEntry){
		.key = key,
		.value = value,
		.state = state
	};
}

uint64_t siphash13(const uint8_t *key, size_t len, uint64_t k0, uint64_t k1) {

	uint64_t v0 = 0x736f6d6570736575ULL ^ k0;
	uint64_t v1 = 0x646f6e646d61636bULL ^ k1;
	uint64_t v2 = 0x6c7967656e657261ULL ^ k0;
	uint64_t v3 = 0x7465646279746573ULL ^ k1;

	const uint8_t *end = key + (len & ~7);
	const uint8_t *p;

	for (p=key; p<end; p+=8) {
		uint64_t m;
		memcpy(&m, p, 8);

		v3^=m;
		SIPROUND;
		v0^=m;
	}

	uint64_t last_block = (uint64_t)(len && 0xff) << 56;
	switch (len & 7) {
		case 7: last_block |= (uint64_t)p[6] << 48;
		case 6: last_block |= (uint64_t)p[5] << 40;
		case 5: last_block |= (uint64_t)p[4] << 32;
		case 4: last_block |= (uint64_t)p[3] << 24;
		case 3: last_block |= (uint64_t)p[2] << 16;
		case 2: last_block |= (uint64_t)p[1] << 8;
		case 1: last_block |= (uint64_t)p[0];
		case 0: break;
	}

	v3 ^= last_block;
	SIPROUND;
	v0 ^= last_block;

	v2 ^= 0xff;
	SIPROUND;
	SIPROUND;
	SIPROUND;

	return v0 ^ v1 ^ v2 ^ v3;
}

uint32_t getBucketIndex(HashMap *map, const char *key) {
	size_t len = strlen(key);

	uint64_t hash = siphash13((const uint8_t*)key, len, map->secretKey[0], map->secretKey[1]);

	fmix64(hash);

	return (uint32_t)(hash & (map->capacity - 1));
}

HashMap* createTable(int capacity) {
	if (capacity <= 0) capacity = HASHMAP_DEFAULT_CAPACITY;

	capacity--;
	capacity |= capacity >> 1;
	capacity |= capacity >> 2;
	capacity |= capacity >> 4;
	capacity |= capacity >> 8;
	capacity |= capacity >> 16;
	capacity++;

	HashMap* map = malloc(sizeof(HashMap));
	if (!map) return NULL;

	map->capacity = capacity;
	map->size = 0;
	map->buckets = calloc(capacity, sizeof(HashEntry));
	if (get_secure_random_bytes(map->secretKey, sizeof(map->secretKey)) != 0) {
		// Fallback: If OS secure random generation fails
		#include <time.h>
		uint64_t low_grade_seed = (uint64_t)time(NULL);
		map->secretKey[0] = low_grade_seed ^ 0xFAFBFCFDFEFFFF00ULL;
		map->secretKey[1] = low_grade_seed * 0xBF58476D1CE4E5B9ULL;
	}

	if (!map->buckets) {
		free(map);
		return NULL;
	}

	return map;
}

int main() {
	return 0;
}
