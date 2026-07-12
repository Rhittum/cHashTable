#ifndef SWAP_H
#define SWAP_H

#define SWAP(x, y) do {         \
	_Static_assert(__builtin_types_compatible_p(__typeof__(x), __typeof__(y)), "SWAP Error: Elements must have identical data types!");                         \
	__auto_type _tmp = (x); \
	(x) = (y);              \
	(y) = _tmp;             \
} while (0)

#endif
