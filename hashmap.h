#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdbool.h>

typedef struct HashMap HashMap;

HashMap* createMap(int capacity);
void map_destroy(HashMap *map);
void map_clear(HashMap *map);

void map_insert(HashMap *map, const char *key, void* value);
void* map_get(HashMap *map, const char *key);
bool map_delete(HashMap *map, const char *key);
bool map_contains(HashMap *map, const char *key);

int map_getSize(HashMap *map);
int map_getCapacity(HashMap *map);
double map_getLoadFactor(HashMap *map);

#endif
