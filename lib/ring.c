#include "../../include/siphon/ring.h"
#include "../../include/siphon/alloc.h"
#include "../../include/siphon/hash.h"
#include "../../include/siphon/fmt.h"

#include <errno.h>
#include <assert.h>

static bool
node_iskey (const void *val, const void *key, size_t len)
{
	const SpRingNode *node = val;
	return node->keylen == len && memcmp (node->key, key, len) == 0;
}

static void
node_free (void *val)
{
	SpRingNode *node = val;
	sp_free (node, sizeof *node + node->keylen);
}

static const SpType map_type = {
	.hash = sp_siphash,
	.iskey = node_iskey,
	.free = node_free
};

int
sp_ring_init (SpRing *self, SpHash fn)
{
	assert (self != NULL);
	assert (fn != NULL);

	self->replicas = NULL;
	self->hash = fn;
	return sp_map_init (&self->nodes, 65, 0.75, &map_type);
}

void
sp_ring_final (SpRing *self)
{
	if (self != NULL) {
		sp_map_final (&self->nodes);
		sp_vec_free (self->replicas);
	}
}

static SpRingNode *
create_node (const void *restrict key, size_t len, int avail)
{
	SpRingNode *node = sp_malloc (sizeof *node + len);
	if (node == NULL) return NULL;

	node->keylen = len;
	node->avail = avail;
	memcpy (node->key, key, len);
	node->key[len] = '\0';

	return node;
}

static int
cmp_replica (const void *a, const void *b)
{
	uint64_t ah = ((const SpRingReplica *)a)->hash;
	uint64_t bh = ((const SpRingReplica *)b)->hash;
	if (ah < bh) { return -1; }
	if (ah > bh) { return 1; }
	return 0;
}

static void
replicate (SpRing *self, SpRingNode *node,
		const void *restrict key, size_t len,
		unsigned replicas)
{
	char buf[1024], *p;
	if (len > sizeof buf - 12) {
		len = sizeof buf - 12;
	}

	memcpy (buf, key, len);
	p = buf + len;

	size_t remain = sizeof buf - len;

	for (unsigned i = 0; i < replicas; i++) {
		int n = snprintf (p, remain, "-%d", i);
		sp_vec_push (self->replicas, ((SpRingReplica) {
			.hash = self->hash (buf, len + n, SP_SEED_DEFAULT),
			.node = node
		}));
	}

	sp_vec_sort (self->replicas, cmp_replica);
}

int
sp_ring_put (SpRing *self,
		const void *restrict key, size_t len,
		unsigned replicas, int avail)
{
	int rc;
	bool new;
	void **pos;

	rc = sp_vec_ensure (self->replicas, replicas);
	if (rc < 0) { return rc; }

	pos = sp_map_reserve (&self->nodes, key, len, &new);
	if (pos == NULL) { return -errno; }
	if (!new) { return -EINVAL; } // TODO: better error code

	SpRingNode *node = create_node (key, len, avail);
	if (node == NULL) {
		int rc = -errno;
		sp_map_del (&self->nodes, key, len);
		return rc;
	}

	*pos = node;

	replicate (self, node, key, len, replicas);

	return 0;
}

const SpRingNode *
sp_ring_get (const SpRing *self, const void *restrict key, size_t len)
{
	return sp_map_get (&self->nodes, key, len);
}

bool
sp_ring_del (SpRing *self, const void *restrict key, size_t len)
{
	assert (self != NULL);
	assert (key != NULL);

	SpRingNode *node = sp_map_steal (&self->nodes, key, len);
	if (node == NULL) return false;

	size_t n = sp_vec_count (self->replicas);
	for (size_t i = 0; i < n; i++) {
		if (self->replicas[i].node == node) {
			sp_vec_remove (self->replicas, i, i+1);
			n--;
		}
	}

	node_free (node);
	return true;
}

const SpRingReplica *
sp_ring_find (const SpRing *self, const void *restrict val, size_t len)
{
	assert (self != NULL);
	assert (val != NULL);

	uint64_t hash = self->hash (val, len, SP_SEED_DEFAULT);
	size_t n = sp_vec_count (self->replicas);
	SpRingReplica *base = self->replicas;

	if (hash < base->hash) {
		return base + (n-1);
	}

	while (n > 0) {
		SpRingReplica *rep = base + (n/2);
		if (hash == rep->hash) return rep;
		else if (n == 1) break;
		else if (hash < rep->hash)
			n /= 2;
		else {
			base = rep;
			n -= n/2;
		}
	}
	return base;
}

const SpRingReplica *
sp_ring_next (const SpRing *self, const SpRingReplica *rep)
{
	assert (self != NULL);

	if (rep == NULL) {
		return self->replicas;
	}

	const SpRingReplica *end = self->replicas + sp_vec_count (self->replicas);
	assert (rep >= self->replicas && rep < end);

	rep = rep + 1;
	if (rep == end) {
		rep = self->replicas;
	}
	return rep;
}

const SpRingNode *
sp_ring_reserve (const SpRing *self, const SpRingReplica *rep)
{
	assert (self != NULL);
	assert (rep != NULL);
	assert (rep >= self->replicas && rep < self->replicas + sp_vec_count (self->replicas));

	const SpRingReplica *start = rep;
	const SpRingReplica *end = self->replicas + sp_vec_count (self->replicas);
	while (rep->node->avail <= 0) {
		rep++;
		if (rep == end) {
			rep = self->replicas;
		}
		if (rep == start) {
			return NULL;
		}
	}
	rep->node->avail--;
	return rep->node;
}

void
sp_ring_restore (const SpRing *self, const SpRingNode *node)
{
	assert (self != NULL);
	assert (sp_map_get (&self->nodes, node->key, node->keylen) == node);
	(void)self;

	((SpRingNode *)node)->avail++;
}

void
sp_ring_print (const SpRing *self, FILE *out)
{
	if (out == NULL) {
		out = stderr;
	}

	if (self == NULL) {
		fprintf (out, "#<SpRing:(null)>\n");
	}
	else {
		flockfile (out);
		fprintf (out, "#<SpRing:%p nodes=%zu, replicas=%zu> {\n",
				(void *)self,
				sp_map_count (&self->nodes),
				sp_vec_count (self->replicas));

		size_t i;
		sp_vec_each (self->replicas, i) {
			SpRingReplica *r = &self->replicas[i];
			fprintf (out, "    %3zu %016" PRIx64 ": ", i, r->hash);
			sp_fmt_str (out, r->node->key, r->node->keylen, true);
			fprintf (out, " (%d)\n", r->node->avail);
		}
		fprintf (out, "}\n");
		funlockfile (out);
	}
}

