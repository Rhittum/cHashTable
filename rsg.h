#ifndef RSG_H
#define RSG_H

#include <stddef.h>

/**
 * @brief Fills a buffer with cryptographically secure random bytes from the host OS.
 * * Automatically handles API calls for Windows, Linux, macOS, and BSD systems.
 * * @param buf Pointer to the memory buffer to fill.
 * @param nbytes The number of random bytes to generate.
 * @return int Returns 0 on success, or -1 on catastrophic failure.
 */
int get_secure_random_bytes(void *buf, size_t nbytes);

#endif
