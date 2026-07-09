#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define HASHMAP_DEFAULT_CAPACITY 32

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

HashEntry setEntry(char* key, void* value, SlotState state) {
	return (HashEntry){
		.key = key,
		.value = value,
		.state = state
	};
}

unsigned int sipHash(const char* key, int capacity) {
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

	if (!map->buckets) {
		free(map);
		return NULL;
	}

	return map;
}

int main() {
	return 0;
}
