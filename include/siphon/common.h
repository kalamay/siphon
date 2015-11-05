#ifndef SIPHON_COMMON_H
#define SIPHON_COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>

#define SP_EXPORT extern __attribute__ ((visibility ("default")))
#define SP_LOCAL  __attribute__ ((visibility ("hidden")))

#define sp_power_of_2(n) __extension__ ({ \
	__typeof (n) x = (n);                 \
	if (x > 0) {                          \
		x--;                              \
		x |= x >> 1;                      \
		x |= x >> 2;                      \
		x |= x >> 4;                      \
		if (sizeof x > 1) {               \
			x |= x >> 8;                  \
			if (sizeof x > 2) {           \
				x |= x >> 16;             \
				if (sizeof x > 4) {       \
					x |= x >> 32;         \
				}                         \
			}                             \
		}                                 \
		x++;                              \
	}                                     \
	x;                                    \
})

#define sp_next_quantum(n, quant) \
	(((((n) - 1) / (quant)) + 1) * (quant))

#define sp_container_of(ptr, type, member) __extension__ ({ \
	const __typeof( ((type *)0)->member ) *__mptr = (ptr);  \
	(type *)( (char *)__mptr - offsetof(type,member) );     \
})

#define sp_len(a) \
	(sizeof (a) / sizeof ((a)[0]))

#define sp_sym_concat(x, y) x ## y
#define sp_sym_make(x, y) sp_sym_concat(x, y)
#define sp_sym(n) sp_sym_make(__##n##_, __LINE__)

#define sp_likely(x) __builtin_expect(!!(x), 1)
#define sp_unlikely(x) __builtin_expect(!!(x), 0)

typedef union {
	struct { uint64_t low, high; } u128;
	uint64_t u64;
	uint32_t u32;
	uint8_t bytes[16];
} SpSeed;

typedef uint64_t (*SpHash)(const void *restrict key, size_t len, const SpSeed *restrict seed);
typedef bool (*SpIsKey)(const void *val, const void *key, size_t len);
typedef void *(*SpCopy)(void *val);
typedef void (*SpFree)(void *val);
typedef void (*SpPrint)(const void *val, FILE *out);

typedef struct {
	SpHash hash;
	SpIsKey iskey;
	SpCopy copy;
	SpFree free;
	SpPrint print;
} SpType;

SP_EXPORT const SpSeed *const restrict SP_SEED_DEFAULT;
SP_EXPORT const SpSeed *const restrict SP_SEED_RANDOM;

SP_EXPORT void
sp_print_ptr (const void *val, FILE *out);

SP_EXPORT void
sp_print_str (const void *val, FILE *out);

#endif

