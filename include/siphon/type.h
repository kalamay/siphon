#ifndef SIPHON_TYPE_H
#define SIPHON_TYPE_H

#include "common.h"
#include "seed.h"

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

#endif

