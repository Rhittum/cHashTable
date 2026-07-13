# C Hashmap/HashTable Library with UX in mind

A minimalist, blazingly fast, and cache-friendly **Robin Hood Hash Map** written in pure C. Designed for modern CPU architectures with optimized linear probing and zero-allocation lookup mechanics.
The cHashmap is based on a combination of SipHash and MurmurHash3's hash mix, for a secure and blazingly fast Hashmap/HashTable.

## Motivation:
I wanted to make this because I counldn't find a single good user friendly Hashmap.


## Features

- **Robin Hood Hashing:** Displaces richer elements to minimize average probe sequence length (PSL), ensuring incredibly tight upper bounds for lookup times.
- **Dynamic Resizing:** Automatically expands using power-of-two boundaries when exceeding a strict $70\%$ load factor.
- **SipHash-1-3 + Murmur3 Avalanching:** Hardened against hash collision DOS attacks via random OS-level cryptographic seeding (`rsg.h`) combined with a fast 64-bit finalizer mix.
- **Compile-Time Type Safety:** Features an ironclad `SWAP()` macro engine that utilizes modern static assertions (`_Static_assert` + `__builtin_types_compatible_p`) to block accidental type mismatch corruptions before compilation finishes.
- **Zero Memory Leaks:** 100% clean Valgrind profile under brutal churn and stress testing.

## API Reference

```c
// Lifecycle Management
HashMap* createTable(int capacity);
void map_destroy(HashMap *map);
void map_clear(HashMap *map);

// Core CRUD Operations
void map_insert(HashMap *map, const char *key, void* value);
void* map_get(HashMap *map, const char *key);
bool map_delete(HashMap *map, const char *key);
bool map_contains(HashMap *map, const char *key);

// Metrics Inspection
int map_getSize(HashMap *map);
int map_getCapacity(HashMap *map);
double map_getLoadFactor(HashMap *map);
```

## Quick Start
```c
#include <stdio.h>
#include "hashmap.h"

int main() {
    // Initializes map with power-of-two rounding
    HashMap *map = createTable(20); 

    // Insert payloads cast to void*
    map_insert(map, "Rin", (void*)"Developer");
    map_insert(map, "Status", (void*)200);

    // Blazingly fast lookup
    char *role = (char*)map_get(map, "Rin");
    printf("Rin's Role: %s\n", role);

    // Clean up all internal heap footprints safely
    map_destroy(map);
    return 0;
}
```

## Operations

### Lifecycle Management

#### `createMap(capacity)`
Allocates and returns a new `HashMap` with the given initial capacity. The capacity is rounded up to the nearest power of two internally. If you pass `0` or a negative value, it defaults to 32 slots. Returns `NULL` on allocation failure.

```c
HashMap *map = createMap(16);  // Creates map with 16 slots
HashMap *map = createMap(20);  // Rounded up to 32 slots (next power of 2)
HashMap *map = createMap(0);   // Defaults to 32 slots
```

#### `map_destroy(map)`
Frees all memory held by the map, including all internally duplicated keys and the bucket array. Safe to call on `NULL`.

```c
map_destroy(map);  // Frees all keys, buckets, and the map struct itself
```

#### `map_clear(map)`
Removes all entries from the map and frees their keys, but keeps the bucket array allocated. The map can be reused immediately after clearing.

```c
map_clear(map);  // Empties the map; capacity stays the same
```

---

### Core CRUD Operations

#### `map_insert(map, key, value)`
Inserts a key-value pair into the map. The key string is duplicated internally via `strdup`, so you don't need to keep the original string alive. If the key already exists, its value is overwritten. Automatically triggers a resize (2x) when the load factor exceeds 70%.

```c
map_insert(map, "name", (void*)"Alice");
map_insert(map, "age", (void*)30);

// Overwriting an existing key
map_insert(map, "name", (void*)"Bob");  // "name" now maps to "Bob"

// Inserting a pointer to heap data
int *data = malloc(sizeof(int));
*data = 42;
map_insert(map, "counter", (void*)data);
```

#### `map_get(map, key)`
Returns the `void*` value associated with the key, or `NULL` if the key is not found. Uses the Robin Hood probe sequence to find the key efficiently.

```c
char *name = (char*)map_get(map, "name");    // Returns "Bob"
int *count = (int*)map_get(map, "counter");  // Returns pointer to 42
void *missing = map_get(map, "nope");        // Returns NULL
```

#### `map_delete(map, key)`
Removes the entry for the given key and frees its internally stored key string. Uses backward-shifting to maintain the Robin Hood invariant. Returns `true` if the key was found and deleted, `false` if it didn't exist.

```c
bool removed = map_delete(map, "name");  // Returns true; entry is gone
bool removed = map_delete(map, "nope");  // Returns false; key not found
```

#### `map_contains(map, key)`
Returns `true` if the key exists in the map, `false` otherwise. This is a convenience wrapper around `map_get`.

```c
if (map_contains(map, "name")) {
    printf("name exists\n");
}
```

---

### Metrics Inspection

#### `map_getSize(map)`
Returns the number of entries currently stored in the map.

```c
int count = map_getSize(map);  // e.g. returns 2
```

#### `map_getCapacity(map)`
Returns the total number of buckets (slots) in the map. This is always a power of two.

```c
int cap = map_getCapacity(map);  // e.g. returns 32
```

#### `map_getLoadFactor(map)`
Returns the current load factor as a `double` (size / capacity). The map auto-resizes when this exceeds 0.7.

```c
double lf = map_getLoadFactor(map);  // e.g. returns 0.0625
```

---

## Building & Testing

To compile the implementation alongside your project components:
Bash

```bash
gcc -O3 hashmap.c rsg.c your_project.c -o your_project
```

To run the built-in structural stress tests and verify memory hygiene:
Bash

```bash
gcc -g hashmap.c rsg.c stress_test.c -o stress_test
valgrind --leak-check=full --show-leak-kinds=all ./stress_test
```
