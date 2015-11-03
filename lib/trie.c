#include "../include/siphon/trie.h"
#include "../include/siphon/fmt.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define PREFIX_MAX (sizeof ((SpTrieBranch *)0)->prefix)
#define LEAF(nn) sp_container_of (nn, SpTrieLeaf, node)
#define BRANCH(nn) sp_container_of (nn, SpTrieBranch, node)
#define IS_LEAF(nn) ((nn)->max == 0)
#define IS_BRANCH(nn) ((nn)->max != 0)
#define SET_LEAF(ln) ((ln)->node.max = 0)
#define CAPACITY(bn) ((bn)->node.max + 1)
#define SET_CAPACITY(bn, mx) ((bn)->node.max = (mx) - 1)

struct SpTrieNode {
	uint8_t max;
};

typedef struct {
	SpTrieNode node; // must be first
	uint32_t key_len;
	void *value;
	uint8_t key[];
} SpTrieLeaf;

typedef struct {
	SpTrieNode node; // must be first
	uint8_t prefix_len, prefix[21], offset;
	SpTrieLeaf *leaf;
	SpTrieNode *children[];
} SpTrieBranch;

static inline uint32_t
min (uint32_t a, uint32_t b)
{
	return a < b ? a : b;
}

static inline uint32_t
clamp (uint32_t v)
{
	return min (v, PREFIX_MAX);
}

static inline size_t
common_prefix (const uint8_t *k, size_t klen, const uint8_t *v, size_t vlen)
{
	uint32_t len = min ((uint32_t)klen, (uint32_t)vlen), i = 0;
	for (; i < len && k[i] == v[i]; i++) {}
	return i;
}

static inline size_t
common_prefix_clamp (const uint8_t *k, size_t klen, const uint8_t *v, size_t vlen)
{
	uint32_t len = clamp (min ((uint32_t)klen, (uint32_t)vlen)), i = 0;
	for (; i < len && k[i] == v[i]; i++) {}
	return i;
}

static size_t
leaf_prefix (const SpTrieLeaf *l, const uint8_t *k, size_t klen, size_t off)
{
	assert (off <= klen);

	if (off > l->key_len) {
		return off;
	}
	return common_prefix (k+off, klen-off, l->key+off, l->key_len-off) + off;
}

static size_t
leaf_prefix_clamp (const SpTrieLeaf *l, const uint8_t *k, size_t klen, size_t off)
{
	assert (off <= klen);

	if (off > l->key_len) {
		return off;
	}
	return common_prefix_clamp (k+off, klen-off, l->key+off, l->key_len-off) + off;
}

static size_t
branch_prefix_clamp (const SpTrieBranch *b, const uint8_t *k, size_t klen, size_t off)
{
	assert (off <= klen);

	return common_prefix_clamp (k+off, klen-off, b->prefix, b->prefix_len) + off;
}

int
sp_trie_init (SpTrie *self, const SpType *type)
{
	assert (self != NULL);

	*self = SP_TRIE_MAKE (type);
	return 0;
}

void
sp_trie_final (SpTrie *self)
{
	assert (self != NULL);

	sp_trie_clear (self);
	*self = SP_TRIE_MAKE (self->type);
}

size_t
sp_trie_count (const SpTrie *self)
{
	assert (self != NULL);

	return self->count;
}

static void
clear (SpTrieNode *n, SpFree func)
{
	if (n == NULL) {
		return;
	}

	if (IS_LEAF (n)) {
		if (func) {
			SpTrieLeaf *l = LEAF (n);
			func (l->value);
		}
	}
	else {
		SpTrieBranch *b = BRANCH (n);
		clear (&b->leaf->node, func);
		for (int i = 0; i < CAPACITY (b); i++) {
			clear (b->children[i], func);
		}
	}

	free (n);
}

void
sp_trie_clear (SpTrie *self)
{
	assert (self != NULL);

	clear (self->root, self->type->free);
	self->root = NULL;
	self->count = 0;
}

static SpTrieLeaf **
get (SpTrieNode **par,
		const void *restrict key, size_t len,
		SpTrieBranch ***path, size_t *depth)
{
	assert (path == NULL || depth != NULL);
	assert (depth == NULL || *depth == 0);

	size_t offset = 0;
	while (*par) {
		if (IS_LEAF (*par)) {
			SpTrieLeaf *l = LEAF (*par);
			offset = leaf_prefix (l, key, len, offset);
			if (offset == len && l->key_len == len) {
				return (SpTrieLeaf **)par;
			}
			break;
		}
		else {
			SpTrieBranch *b = BRANCH (*par);
			if (path != NULL) {
				path[(*depth)++] = (SpTrieBranch **)par;
			}
			offset = branch_prefix_clamp (b, key, len, offset);
			if (offset == len) {
				if (b->leaf && b->leaf->key_len == len) {
					return &b->leaf;
				}
				break;
			}
			uint8_t c = ((uint8_t *)key)[offset++];
			if (c < b->offset || c > b->offset + CAPACITY (b)) {
				break;
			}
			par = &b->children[c - b->offset];
		}
	}

	if (path != NULL) {
		*depth = 0;
	}
	return NULL;
}

static SpTrieLeaf *
match (const SpTrie *self, const void *restrict key, size_t len, size_t *off, SpTrieMatch cb, void *data)
{
	// TODO check for mis-match
	assert (self != NULL);
	assert (key != NULL);

	SpTrieNode *n = self->root;
	size_t offset = 0;
	SpTrieLeaf *leaf = NULL;
	while (n && offset <= len) {
		if (IS_LEAF (n)) {
			SpTrieLeaf *l = LEAF (n);
			offset = leaf_prefix (l, key, len, offset);
			if (offset == l->key_len && cb (key, len, offset, l->value, data)) {
				if (off) {
					*off = offset;
				}
				leaf = l;
			}
			break;
		}
		else {
			SpTrieBranch *b = BRANCH (n);
			size_t end = branch_prefix_clamp (b, key, len, offset);
			if (b->leaf &&
					(end - offset) == b->prefix_len &&
					cb (key, len, end, b->leaf->value, data)) {
				if (off) {
					*off = end;
				}
				leaf = b->leaf;
			}
			if (end == len) {
				break;
			}
			offset = end + 1;
			int s = ((uint8_t *)key)[end];
			if (s < b->offset || s > b->offset + CAPACITY (b)) {
				break;
			}
			n = b->children[s - b->offset];
		}
	}

	return leaf;
}

static bool
cmp (const void *key, size_t len, size_t off, void *val, void *data)
{
	(void)val;
	int c = *(int *)data;
	return (off == len || c < 0 || ((uint8_t *)key)[off] == (uint8_t)c);
}

static SpTrieLeaf *
prefix (const SpTrie *self, const void *restrict key, size_t len, size_t *off, int c)
{
	return match (self, key, len, off, cmp, &c);
}

bool
sp_trie_has_key (const SpTrie *self, const void *restrict key, size_t len)
{
	assert (self != NULL);
	assert (key != NULL);

	return get ((SpTrieNode **)&self->root, key, len, NULL, NULL) != NULL;
}

bool
sp_trie_has_prefix (const SpTrie *self, const void *restrict key, size_t len, size_t *off, int c)
{
	assert (self != NULL);
	assert (key != NULL);

	return prefix (self, key, len, off, c) != NULL;
}

bool
sp_trie_has_match (const SpTrie *self, const void *restrict key, size_t len, size_t *off, SpTrieMatch cb, void *data)
{
	assert (self != NULL);
	assert (key != NULL);

	return match (self, key, len, off, cb, data) != NULL;
}

void *
sp_trie_get (const SpTrie *self, const void *restrict key, size_t len)
{
	assert (self != NULL);
	assert (key != NULL);

	SpTrieLeaf **par = get ((SpTrieNode **)&self->root, key, len, NULL, NULL);
	return par ? (*par)->value : NULL;
}

void *
sp_trie_prefix (const SpTrie *self, const void *restrict key, size_t len, size_t *off, int c)
{
	assert (self != NULL);
	assert (key != NULL);

	SpTrieLeaf *l = prefix (self, key, len, off, c);
	return l ? l->value : NULL;
}

void *
sp_trie_match (const SpTrie *self, const void *restrict key, size_t len, size_t *off, SpTrieMatch cb, void *data)
{
	assert (self != NULL);
	assert (key != NULL);

	SpTrieLeaf *l = match (self, key, len, off, cb, data);
	return l ? l->value : NULL;
}

int
sp_trie_put (SpTrie *self, const void *restrict key, size_t len, void *val)
{
	assert (self != NULL);
	assert (key != NULL);

	bool new = true;
	void **pos = sp_trie_reserve (self, key, len, &new);
	if (pos == NULL) {
		return -errno;
	}
	if (!new && self->type->free) {
		self->type->free (*pos);
	}
	*pos = self->type->copy ? self->type->copy (val) : val;
	return !new;
}

/**
 * Return the node position for 'c', resizing the branch if necessary
 */
static SpTrieNode **
reserve (SpTrieBranch **ref, uint8_t c)
{
	assert (ref != NULL);
	assert (*ref != NULL);

	SpTrieBranch *b = *ref;

	uint32_t low = 0, hi = 0, off = b->offset, len = CAPACITY (b);
	if (c < off) {
		low = off - c;
		if (low >= len) {
			off = c;
		}
		else {
			// pad to reduce resizes, grow by approximate factor of 2
			low = min (len, off);
			off -= low;
		}
	}
	else if (c >= off + len) {
		hi = c - (off + len - 1);
		if (hi < len) {
			// pad to reduce resizes, grow by approximate factor of 2
			hi = min (len, 256 - (len + off));
		}
	}

	// need to resize branch
	if (low || hi) {
		int cap = len + low + hi;
		assert (cap <= 256);
		assert (cap > 1);
		assert (off <= c);

		b = realloc (b, sizeof *b + cap * sizeof b->children[0]);
		if (b == NULL) return NULL;

		memmove (b->children + low, b->children, len * sizeof b->children[0]);
		for (size_t i = 0; i < low; i++) {
			b->children[i] = NULL;
		}
		for (size_t i = len + low, e = i + hi; i < e; i++) {
			b->children[i] = NULL;
		}

		SET_CAPACITY (b, cap);
		b->offset = off;
		*ref = b;
	}

	assert (c >= b->offset);
	assert (c - b->offset <= CAPACITY (b));
	return &b->children[c - b->offset];
}

static SpTrieBranch *
branch_create (uint8_t c1, uint8_t c2, const void *key, size_t len)
{
	assert (len <= PREFIX_MAX);

	int cap, off;
	if (c1 == c2) {
		cap = 2;
		off = c1 % 2 ? c1 - 1 : c1;
	}
	else {
		cap = abs ((int)c2 - c1) + 1;
		off = min (c1, c2);
	}

	SpTrieBranch *b = malloc (sizeof *b + cap * sizeof b->children[0]);
	if (b == NULL) return NULL;

	SET_CAPACITY (b, cap);
	b->offset = off;
	b->prefix_len = len;
	memcpy (b->prefix, key, b->prefix_len);

	b->leaf = NULL;
	for (int i = 0; i < cap; i++) {
		b->children[i] = NULL;
	}

	return b;
}

void **
sp_trie_reserve (SpTrie *self, const void *restrict key, size_t len, bool *isnew)
{
	assert (self != NULL);
	assert (key != NULL);
	assert (isnew != NULL);
	assert (len <= UINT32_MAX);

	SpTrieNode **par = &self->root;
	size_t offset = 0, depth = 1;
	while (*par != NULL) {
		depth++;
		if (IS_LEAF (*par)) {
			SpTrieLeaf *l = LEAF (*par);
			size_t end = leaf_prefix_clamp (l, key, len, offset);
			uint8_t c1, c2;

			// if matching end of key, leaf belongs at position
			if (end == len) {
				// if lengths match, use the current leaf value
				if (l->key_len == len) {
					*isnew = false;
					return &l->value;
				}

				// branch with new leaf shared in branch, old leaf becomes child
				c1 = c2 = l->key[end];
			}
			else {
				// branch with old leaf shared in branch, new leaf becomes child
				c1 = ((uint8_t *)key)[end];
				c2 = end == l->key_len ? c1 : l->key[end];
			}

			SpTrieBranch *b = branch_create (c1, c2, (uint8_t *)key + offset, end - offset);
			if (b == NULL) return NULL;

			if (end == len) {
				// set old leaf as child
				SpTrieNode **n = reserve (&b, c2);
				if (n == NULL) return NULL;
				*n = &l->node;

				// set branch and shared leaf as new parent
				*par = &b->node;
				par = (SpTrieNode **)&b->leaf;
				break;
			}
			else {
				if (end == l->key_len) {
					// set old leaf as shared
					b->leaf = l;
				}
				else {
					// set old leaf as child
					SpTrieNode **n = reserve (&b, c2);
					if (n == NULL) return NULL;
					*n = &l->node;
				}

				// set new branch as parent
				*par = &b->node;
				par = reserve ((SpTrieBranch **)par, c1);
			}

			// if offset reaches end of key then stop, else keep branching
			offset = end + 1;
			if (offset >= len) {
				break;
			}
		}
		else {
			SpTrieBranch *b = BRANCH (*par);
			size_t end = branch_prefix_clamp (b, key, len, offset);

			// if matching end of key, leaf belongs at branch
			// and branch needs to be split
			if (end == len) {
				size_t tail = end - offset;
				if (tail < b->prefix_len) {
					uint8_t c = b->prefix[tail];

					// create new branch by splitting the prefix
					SpTrieBranch *nb = branch_create (c, c, b->prefix, tail);
					if (nb == NULL) return NULL;

					// shorten old branch
					memmove (b->prefix, b->prefix + tail + 1, b->prefix_len - tail - 1);
					b->prefix_len = b->prefix_len - tail - 1;

					// set old branch as child of new branch
					SpTrieNode **n = reserve (&nb, c);
					if (n == NULL) return NULL;
					*n = (SpTrieNode *)b;

					// set new branch as current branch
					b = nb;
					*par = &b->node;
				}
				else if (b->leaf != NULL) {
					// matched full key to branch with shared leaf
					*isnew = false;
					return &b->leaf->value;
				}
				par = (SpTrieNode **)&b->leaf;
				break;
			}

			// move parent to the child position in the branch
			par = reserve ((SpTrieBranch **)par, ((uint8_t *)key)[end]);
			if (par == NULL) return NULL;
			offset = end + 1;
		}
	}

	SpTrieLeaf *leaf = malloc (sizeof *leaf + len);
	if (leaf == NULL) return NULL;

	SET_LEAF (leaf);
	leaf->key_len = (uint32_t)len;
	memcpy (leaf->key, key, len);

	*par = &leaf->node;
	self->count++;
	if (depth > self->depth) {
		self->depth = depth;
	}
	*isnew = true;
	return &leaf->value;
}

static bool
compact (SpTrieBranch **b)
{
	// find the only child node otherwise we can't compact
	SpTrieNode **child = (*b)->leaf == NULL ? NULL : (SpTrieNode **)&(*b)->leaf;
	int key = -1;
	for (size_t i = 0, cap = CAPACITY (*b); i < cap; i++) {
		if ((*b)->children[i] != NULL) {
			// more than one child so return without compacting
			if (child != NULL) {
				return false;
			}
			// capture child address and branch child key character
			child = &(*b)->children[i];
			key = i + (*b)->offset;
		}
	}

	// when a child is found it will replace the parent branch
	if (child != NULL) {
		// if child is a branch then join the two key prefixes plus the key character
		if (IS_BRANCH (*child)) {
			SpTrieBranch *new = *(SpTrieBranch **)child;

			// old branch length plus key character
			size_t insert_len = (*b)->prefix_len + 1;
			// new length of compacted prefix
			size_t full_len = new->prefix_len + insert_len;

			// cannot compact due to prefix length limit
			if (full_len > PREFIX_MAX) {
				return false;
			}

			// move existing prefix over and insert old branch prefix and child key
			memmove (new->prefix+insert_len, new->prefix, new->prefix_len);
			memcpy (new->prefix, (*b)->prefix, (*b)->prefix_len);
			new->prefix[(*b)->prefix_len] = (uint8_t)key;
			new->prefix_len = full_len;
		}
		// move child into branch position
		SpTrieNode *tmp = *child;
		*child = NULL;
		free (*b);
		*(SpTrieNode **)b = tmp;
	}
	// if the branch is empty just free and clear it
	else {
		free (*b);
		*b = NULL;
	}

	return true;
}

bool
sp_trie_del (SpTrie *self, const void *restrict key, size_t len)
{
	assert (self != NULL);
	assert (key != NULL);

	void *val = sp_trie_steal (self, key, len);
	if (val != NULL) {
		if (self->type->free) {
			self->type->free (val);
		}
		return true;
	}
	else {
		return false;
	}
}

void *
sp_trie_steal (SpTrie *self, const void *restrict key, size_t len)
{
	assert (self != NULL);
	assert (key != NULL);

	void *value = NULL;
	size_t depth = 0;
	SpTrieBranch **path[self->depth];
	SpTrieLeaf **par = get ((SpTrieNode **)&self->root, key, len, path, &depth);

	if (par != NULL) {
		SpTrieLeaf *l = *par;
		*par = NULL; // remove leaf from its position

		// compact each path item bottom up
		for (size_t i = depth; i > 0 && compact (path[i-1]); i--);

		value = l->value;
		free (l);
		self->count--;
	}
	return value;
}

static bool
each (const SpTrieNode *n, SpTrieCallback func, void *data)
{
	if (n == NULL) {
		return true;
	}
	if (IS_LEAF (n)) {
		SpTrieLeaf *l = LEAF (n);
		return func (l->key, l->key_len, l->value, data);
	}
	else {
		SpTrieBranch *b = BRANCH (n);
		if (b->leaf) {
			if (!each (&b->leaf->node, func, data)) {
				return false;
			}
		}
		for (int i = 0, e = CAPACITY (b); i < e; i++) {
			if (!each (b->children[i], func, data)) {
				return false;
			}
		}
	}
	return true;
}

bool
sp_trie_each (const SpTrie *self, SpTrieCallback func, void *data)
{
	assert (self != NULL);
	assert (func != NULL);

	return each (self->root, func, data);
}

bool
sp_trie_each_prefix (const SpTrie *self,
		const void *restrict key, size_t len,
		SpTrieCallback func, void *data)
{
	assert (self != NULL);
	assert (key != NULL);
	assert (func != NULL);

	SpTrieNode *node = self->root;

	size_t offset = 0;
	while (node) {
		if (IS_LEAF (node)) {
			SpTrieLeaf *l = LEAF (node);
			offset = leaf_prefix (l, key, len, offset);
			if (offset != len) {
				node = NULL;
			}
			break;
		}
		else {
			SpTrieBranch *b = BRANCH (node);
			offset = branch_prefix_clamp (b, key, len, offset);
			if (offset == len) {
				break;
			}
			uint8_t c = ((uint8_t *)key)[offset++];
			if (c < b->offset || c > b->offset + CAPACITY (b)) {
				node = NULL;
				break;
			}
			node = b->children[c - b->offset];
		}
	}

	return each (node, func, data);
}

static void
print_leaf (const SpTrieLeaf *l, FILE *out, SpPrint print)
{
	sp_fmt_str (out, l->key, l->key_len, true);
	fprintf (out, " ");
	print (l->value, out);
	fprintf (out, "\n");
}

static void
print_branch (const SpTrieBranch *b, FILE *out, SpPrint print)
{
	sp_fmt_str (out, b->prefix, b->prefix_len, true);
	fprintf (out, " [%u-%u]", b->offset, CAPACITY (b) - 1 + b->offset);
	if (b->leaf) {
		fprintf (out, ", ");
		print_leaf (b->leaf, out, print);
	}
	else {
		fprintf (out, "\n");
	}
}

static void
print_list (const SpTrieNode *n, FILE *out, int c, bool *stack, int top, SpPrint print)
{
	if (n == NULL) return;

	fprintf (out, "    ");

	if (top > 0) {
		int i;
		for (i = 0; i < top - 1; i++) {
			if (stack[i]) {
				fprintf (out, "    ");
			}
			else {
				fprintf (out, "│   ");
			}
		}
		if (stack[i]) {
			fprintf (out, "└── ");
		}
		else {
			fprintf (out, "├── ");
		}
	}

	if (c >= 0) {
		sp_fmt_char (out, c);
		fprintf (out, " = ");
	}
	
	if (IS_LEAF (n)) {
		print_leaf (LEAF (n), out, print);
	}
	else {
		const SpTrieBranch *b = BRANCH (n);
		print_branch (b, out, print);
		int end = CAPACITY (b) - 1;
		while (b->children[end] == NULL) {
			end--;
		}
		for (int i = 0; i <= end; i++) {
			stack[top] = (i == end);
			print_list (b->children[i], out, i + b->offset, stack, top+1, print);
		}
	}
}

void
sp_trie_print (const SpTrie *self, FILE *out)
{
	if (out == NULL) {
		out = stderr;
	}

	SpPrint print = self->type->print;
	if (print == NULL) {
		print = sp_print_ptr;
	}

	if (self == NULL) {
		fprintf (out, "#<SpTrie:(null)>\n");
	}
	else {
		flockfile (out);
		bool stack[256] = { 0 };
		fprintf (out, "#<SpTrie:%p count=%zu depth=%zu> {\n", (void *)self, self->count, self->depth);
		print_list (self->root, out, -1, stack, 0, print);
		fprintf (out, "}\n");
		funlockfile (out);
	}
}

