#ifndef SIPHON_TRIE_H
#define SIPHON_TRIE_H

#include "common.h"

typedef struct SpTrie SpTrie;
typedef struct SpTrieNode SpTrieNode;

struct SpTrie {
	const SpType *type;
	SpTrieNode *root;
	size_t count, depth;
};

#define SP_TRIE_MAKE(typ) ((SpTrie){ \
	.type = (typ),                   \
	.root = NULL,                    \
	.count = 0,                      \
	.depth = 0                       \
})

typedef bool (*SpTrieCallback)(
		const void *key,
		size_t len,
		void *val,
		void *data);

typedef bool (*SpTrieMatch)(
		const void *key,
		size_t len,
		size_t off,
		void *val,
		void *data);

SP_EXPORT int
sp_trie_init (SpTrie *self, const SpType *type);

SP_EXPORT void
sp_trie_final (SpTrie *self);

SP_EXPORT void
sp_trie_clear (SpTrie *self);

SP_EXPORT size_t
sp_trie_count (const SpTrie *self);

SP_EXPORT bool
sp_trie_has_key (const SpTrie *self, const void *restrict key, size_t len);

SP_EXPORT bool
sp_trie_has_prefix (const SpTrie *self, const void *restrict key, size_t len,
		size_t *off, int c);

SP_EXPORT bool
sp_trie_has_match (const SpTrie *self, const void *restrict key, size_t len,
		SpTrieMatch cb, void *data);

SP_EXPORT void *
sp_trie_min (const SpTrie *self);

SP_EXPORT void *
sp_trie_get (const SpTrie *self, const void *restrict key, size_t len);

SP_EXPORT void *
sp_trie_prefix (const SpTrie *self, const void *restrict key, size_t len,
		size_t *off, int c);

SP_EXPORT void *
sp_trie_match (const SpTrie *self, const void *restrict key, size_t len,
		SpTrieMatch cb, void *data);

SP_EXPORT int
sp_trie_put (SpTrie *self, const void *restrict key, size_t len, void *val);

SP_EXPORT void **
sp_trie_reserve (SpTrie *self, const void *restrict key, size_t len, bool *isnew);

SP_EXPORT bool
sp_trie_del (SpTrie *self, const void *restrict key, size_t len);

SP_EXPORT void *
sp_trie_steal (SpTrie *self, const void *restrict key, size_t len);

SP_EXPORT bool
sp_trie_each (const SpTrie *self, SpTrieCallback func, void *data);

SP_EXPORT bool
sp_trie_each_prefix (const SpTrie *self, const void *restrict key, size_t len,
		SpTrieCallback func, void *data);

SP_EXPORT void
sp_trie_print (const SpTrie *self, FILE *out);

#endif

