#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "rsg.h"
#include "generic_swap.h"

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
	EMPTY = 0,
	OCCUPIED,
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

static void resize(HashMap *map, uint32_t new_capacity);

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

	uint64_t last_block = (uint64_t)(len & 0xff) << 56;
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

	hash = fmix64(hash);

	return (uint32_t)(hash & (map->capacity - 1));
}

HashMap* createMap(int capacity) {
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
	if (!map->buckets) {
		free(map);
		return NULL;
	}
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

void map_insert(HashMap *map, const char *key, void* value) {
	assert(map!=NULL);

	if ((map->size + 1)*10 >= map->capacity*7) {
		resize(map, map->capacity*2);
	}

	uint32_t bucketIdx = getBucketIndex(map, key);

	char* current_key = strdup(key);
	void* current_value = value;
	uint32_t current_psl = 0;

	while (true) {
		HashEntry *slot = &map->buckets[bucketIdx];

		if (slot->state == EMPTY) {
			slot->key = current_key;
			slot->value = current_value;
			slot->state = OCCUPIED;
			slot->psl = current_psl;

			map->size++;
			return;
		}

		if (slot->state == OCCUPIED && strcmp(slot->key, current_key)==0) {
			free(current_key);
			slot->value = current_value;
			return;
		}

		if (current_psl > slot->psl) {
			SWAP(slot->key, current_key);
			SWAP(slot->value, current_value);
			SWAP(slot->psl, current_psl);
		}

		bucketIdx = (bucketIdx + 1) & (map->capacity - 1);
		current_psl++;
	}
}

bool map_delete(HashMap *map, const char* key) {
	assert(map!=NULL);

	uint32_t bucketIdx = getBucketIndex(map, key);
	uint32_t current_psl = 0;

	while (true) {
		HashEntry *slot = &map->buckets[bucketIdx];

		if (slot->state==EMPTY && current_psl > slot->psl) {
			return false;
		}

		if (slot->state==OCCUPIED && strcmp(slot->key, key)==0) {
			free(slot->key);
			map->size--;

			uint32_t nextIdx = (bucketIdx+1) & (map->capacity-1);

			while (true) {
				HashEntry *nextSlot = &map->buckets[nextIdx];

				if (nextSlot->state==EMPTY || nextSlot->psl==0) {
					slot->key = NULL;
					slot->value = NULL;
					slot->state = EMPTY;
					slot->psl = 0;
					break;
				}

				slot->key = nextSlot->key;
				slot->value = nextSlot->value;
				slot->state = nextSlot->state;
				slot->psl = nextSlot->psl-1;

				slot = nextSlot;

				nextIdx = (nextIdx+1) & (map->capacity-1);
			}

			return true;
		}

		bucketIdx = (bucketIdx+1) & (map->capacity-1);
		current_psl++;
	}
}

void* map_get(HashMap *map, const char *key) {
	assert(map!=NULL);

	uint32_t bucketIdx = getBucketIndex(map, key);
	uint32_t current_psl = 0;

	while (true) {
		HashEntry *slot = &map->buckets[bucketIdx];

		if (slot->state == EMPTY) {
			return NULL;
		}

		if (current_psl > slot->psl) {
			return NULL;
		}

		if (slot->state == OCCUPIED && strcmp(slot->key, key)==0) {
			return slot->value;
		}
		bucketIdx = (bucketIdx+1)&(map->capacity-1);
		current_psl++;
	}
}

void resize(HashMap *map, uint32_t new_capacity) {
	uint32_t old_capacity = map->capacity;
	HashEntry *old_buckets = map->buckets;

	HashEntry *new_buckets = calloc(new_capacity, sizeof(HashEntry));
	if (!new_buckets) return;

	map->buckets = new_buckets;
	map->capacity = new_capacity;
	map->size = 0;

	for (uint32_t i=0; i < old_capacity; i++) {
		if (old_buckets[i].state == OCCUPIED) {
			map_insert(map, old_buckets[i].key, old_buckets[i].value);
			free(old_buckets[i].key);
		}
	}
}

void map_destroy(HashMap *map) {
	if (!map) return;

	for (int i=0; i<map->capacity; i++) {
		if (map->buckets[i].state==OCCUPIED) {
			free(map->buckets[i].key);
		}
	}

	free(map->buckets);
	free(map);
}

void map_clear(HashMap *map) {
	assert(map!=NULL);

	for (int i=0; i<map->capacity; i++) {
		if (map->buckets[i].state==OCCUPIED) {
			free(map->buckets[i].key);
		}
		map->buckets[i].key = NULL;
		map->buckets[i].value = NULL;
		map->buckets[i].state = EMPTY;
		map->buckets[i].psl = 0;
	}

	map->size = 0;
}

bool map_contains(HashMap *map, const char *key) {
	return map_get(map, key)!=NULL;
}

int map_getSize(HashMap *map) {
	assert(map!=NULL);
	return map->size;
}
int map_getCapacity(HashMap *map) {
	assert(map!=NULL);
	return map->capacity;
}
double map_getLoadFactor(HashMap *map) {
	assert(map!=NULL);
	if (map->capacity==0) return 0.0;
	return (double)map->size/map->capacity;
}

int main() {
	HashMap *map = createMap(20);

	map_insert(map, "Rin", "1");
	map_insert(map, "Apple", "2");
	map_insert(map, "Bees", "33");

	char *value = (char*)map_get(map, "Apple");
	printf("value: %s\n",value);
	value = (char*)map_get(map, "Bees");
	printf("value: %s\n",value);

	printf("Did key delete?: %s\n",map_delete(map, "Apple")?"Yes":"No");
	printf("Checking for Apple element's existence:\n");
	value = (char*)map_get(map, "Apple");
	printf("value: %s\n",value);
	return 0;
}
