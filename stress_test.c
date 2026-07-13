#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "hashmap.h"

#define NUM_ITEMS 5000

int main() {
    printf("--- STARTING ROBIN HOOD HASHMAP STRESS TEST ---\n");

    // 1. Initialize a small map to force continuous resizes
    HashMap *map = createMap(16);
    assert(map != NULL);
    printf("Initial capacity: %d\n", map_getCapacity(map));

    // 2. Heavy Insertion: Push thousands of items
    printf("Inserting %d items...\n", NUM_ITEMS);
    for (int i = 0; i < NUM_ITEMS; i++) {
        char key[32];
        sprintf(key, "key_string_%d", i);
        
        // Storing the index integer cast to void* as our payload
        map_insert(map, key, (void*)(uintptr_t)i);
    }
    
    printf("Post-insertion capacity: %d\n", map_getCapacity(map));
    printf("Post-insertion size: %d\n", map_getSize(map));
    assert(map_getSize(map) == NUM_ITEMS);

    // 3. Verification & Lookups
    printf("Verifying all inserted items...\n");
    for (int i = 0; i < NUM_ITEMS; i++) {
        char key[32];
        sprintf(key, "key_string_%d", i);
        
        uintptr_t val = (uintptr_t)map_get(map, key);
        if (val != (uintptr_t)i) {
            printf("Error: Key '%s' expected %d, got %lu\n", key, i, val);
            exit(1);
        }
    }

    // 4. Churn & Deletion: Delete every even-indexed key
    printf("Deleting even-indexed keys to test backward-shifting...\n");
    int deleted_count = 0;
    for (int i = 0; i < NUM_ITEMS; i += 2) {
        char key[32];
        sprintf(key, "key_string_%d", i);
        
        if (map_delete(map, key)) {
            deleted_count++;
        }
    }
    printf("Successfully deleted %d items.\n", deleted_count);
    printf("Current map size: %d\n", map_getSize(map));
    assert(map_getSize(map) == NUM_ITEMS - deleted_count);

    // 5. Final verification of remaining keys
    printf("Verifying remaining odd-indexed keys...\n");
    for (int i = 0; i < NUM_ITEMS; i++) {
        char key[32];
        sprintf(key, "key_string_%d", i);
        
        bool contains = map_contains(map, key);
        if (i % 2 == 0) {
            assert(!contains); // Evicted elements should be missing
        } else {
            assert(contains);  // Survived elements must still be present
        }
    }

    // 6. Complete Destruction
    printf("Destroying map and cleaning up allocations...\n");
    map_destroy(map);

    printf("--- STRESS TEST PASSED SUCCESSFULLY ---\n");
    return 0;
}
