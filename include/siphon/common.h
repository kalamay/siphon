#ifndef SIPHON_COMMON_H
#define SIPHON_COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>

#define SP_EXPORT extern __attribute__ ((visibility ("default")))
#define SP_LOCAL  __attribute__ ((visibility ("hidden")))

#define sp_sym_concat(x, y) x ## y
#define sp_sym_make(x, y) sp_sym_concat(x, y)
#define sp_sym(n) sp_sym_make(sym__##n##_, __LINE__)

#define sp_likely(x) __builtin_expect(!!(x), 1)
#define sp_unlikely(x) __builtin_expect(!!(x), 0)

#define sp_power_of_2(n) __extension__ ({ \
	__typeof (n) tmp = (n);               \
	if (tmp > 0) {                        \
		tmp--;                            \
		tmp |= tmp >> 1;                  \
		tmp |= tmp >> 2;                  \
		tmp |= tmp >> 4;                  \
		if (sizeof tmp > 1) {             \
			tmp |= tmp >> 8;              \
			if (sizeof tmp > 2) {         \
				tmp |= tmp >> 16;         \
				if (sizeof tmp > 4) {     \
					tmp |= tmp >> 32;     \
				}                         \
			}                             \
		}                                 \
		tmp++;                            \
	}                                     \
	tmp;                                  \
})

#define sp_power_of_2_prime(n) __extension__ ({                            \
	__typeof (n) tmp = (n);                                                \
	tmp > 0 ? POWER_OF_2_PRIMES[63 - __builtin_clzll (tmp)] : (uint64_t)0; \
})

#define sp_next_quantum(n, quant) __extension__ ({ \
	__typeof (n) tmp = (quant);                    \
	(((((n) - 1) / tmp) + 1) * tmp);               \
})

#define sp_container_of(ptr, type, member) __extension__ ({ \
	const __typeof( ((type *)0)->member ) *__mptr = (ptr);  \
	(type *)( (char *)__mptr - offsetof(type,member) );     \
})

#define sp_len(a) \
	(sizeof (a) / sizeof ((a)[0]))

#define sp_swap(a, b) do {  \
	__typeof (a) tmp = (a); \
	(a) = (b);              \
	(b) = tmp;              \
} while (0)

SP_EXPORT const uint64_t POWER_OF_2_PRIMES[64];

typedef enum {
	SP_DESCENDING,
	SP_ASCENDING
} SpOrder;

SP_EXPORT void
sp_print_ptr (const void *val, FILE *out);

SP_EXPORT void
sp_print_str (const void *val, FILE *out);

#endif

