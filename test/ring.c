#include "../include/siphon/ring.h"
#include "mu/mu.h"

#define FIND(r, s) sp_ring_find ((r), (s), sizeof (s) - 1)
#define RESERVE(r, s) sp_ring_reserve ((r), FIND (r, s))

static void
test_find (void)
{
	SpRing ring;
	sp_ring_init (&ring, sp_siphash);

	sp_ring_put (&ring, "test1", 5, 3, 2);
	sp_ring_put (&ring, "test2", 5, 3, 2);
	sp_ring_put (&ring, "test3", 5, 3, 2);

	const SpRingReplica *r;

	r = FIND (&ring, "/some/path");
	mu_fassert_ptr_ne (r, NULL);
	mu_assert_str_eq (r->node->key, "test2");

	r = FIND (&ring, "/another/thing/with/values");
	mu_fassert_ptr_ne (r, NULL);
	mu_assert_str_eq (r->node->key, "test3");

	r = FIND (&ring, "/short");
	mu_fassert_ptr_ne (r, NULL);
	mu_assert_str_eq (r->node->key, "test2");

	r = FIND (&ring, "/");
	mu_fassert_ptr_ne (r, NULL);
	mu_assert_str_eq (r->node->key, "test3");

	r = FIND (&ring, "/x");
	mu_fassert_ptr_ne (r, NULL);
	mu_assert_str_eq (r->node->key, "test1");

	sp_ring_final (&ring);
}

static void
test_reserve (void)
{
	SpRing ring;
	sp_ring_init (&ring, sp_siphash);

	sp_ring_put (&ring, "test1", 5, 3, 2);
	sp_ring_put (&ring, "test2", 5, 3, 2);
	sp_ring_put (&ring, "test3", 5, 3, 2);

	const SpRingNode *n;
	const SpRingReplica *r;
	
	n = RESERVE (&ring, "/a");
	mu_assert_ptr_ne (n, NULL);
	if (n) mu_assert_str_eq (n->key, "test3");
	
	n = RESERVE (&ring, "/baz");
	mu_assert_ptr_ne (n, NULL);
	if (n) mu_assert_str_eq (n->key, "test3");
	
	r = FIND (&ring, "/");
	mu_assert_ptr_ne (r, NULL);
	if (r) {
		mu_assert_str_eq (r->node->key, "test3");
		mu_assert_int_eq (r->node->avail, 0);
	}
	
	n = RESERVE (&ring, "/");
	mu_assert_ptr_ne (n, NULL);
	if (n) mu_assert_str_eq (n->key, "test2");

	sp_ring_final (&ring);
}

static void
test_restore (void)
{
	SpRing ring;
	sp_ring_init (&ring, sp_siphash);

	sp_ring_put (&ring, "test1", 5, 3, 2);
	sp_ring_put (&ring, "test2", 5, 3, 2);
	sp_ring_put (&ring, "test3", 5, 3, 2);

	const SpRingNode *n;
	const SpRingReplica *r;
	
	n = RESERVE (&ring, "/a");
	mu_assert_ptr_ne (n, NULL);
	if (n) mu_assert_str_eq (n->key, "test3");
	
	n = RESERVE (&ring, "/baz");
	mu_assert_ptr_ne (n, NULL);
	if (n) mu_assert_str_eq (n->key, "test3");
	
	r = FIND (&ring, "/");
	mu_assert_ptr_ne (r, NULL);
	if (r) {
		mu_assert_str_eq (r->node->key, "test3");
		mu_assert_int_eq (r->node->avail, 0);
	}

	sp_ring_restore (&ring, n);
	
	n = RESERVE (&ring, "/");
	mu_assert_ptr_ne (n, NULL);
	if (n) mu_assert_str_eq (n->key, "test3");

	sp_ring_final (&ring);
}

static void
test_next (void)
{
	SpRing ring;
	sp_ring_init (&ring, sp_siphash);

	sp_ring_put (&ring, "test1", 5, 3, 2);
	sp_ring_put (&ring, "test2", 5, 3, 2);
	sp_ring_put (&ring, "test3", 5, 3, 2);

	const SpRingReplica *r;
	r = FIND (&ring, "/");
	mu_assert_str_eq (r->node->key, "test3");
	r = sp_ring_next (&ring, r);
	mu_assert_str_eq (r->node->key, "test2");
	r = sp_ring_next (&ring, r);
	mu_assert_str_eq (r->node->key, "test1");
	r = sp_ring_next (&ring, r);
	mu_assert_str_eq (r->node->key, "test2");
	r = sp_ring_next (&ring, r);
	mu_assert_str_eq (r->node->key, "test3");
	r = sp_ring_next (&ring, r);
	mu_assert_str_eq (r->node->key, "test1");

	sp_ring_final (&ring);
}

static void
test_del (void)
{
	SpRing ring;
	sp_ring_init (&ring, sp_siphash);

	sp_ring_put (&ring, "test1", 5, 3, 2);
	sp_ring_put (&ring, "test2", 5, 3, 2);
	sp_ring_put (&ring, "test3", 5, 3, 2);

	mu_assert_int_eq (sp_vec_count (ring.replicas), 9);
	mu_assert (sp_ring_del (&ring, "test2", 5));
	mu_assert_int_eq (sp_vec_count (ring.replicas), 6);
	mu_assert_ptr_ne (sp_ring_get (&ring, "test1", 5), NULL);
	mu_assert_ptr_eq (sp_ring_get (&ring, "test2", 5), NULL);
	mu_assert_ptr_ne (sp_ring_get (&ring, "test3", 5), NULL);

	sp_ring_final (&ring);
}

int
main (void)
{
	mu_init ("ring");

	test_find ();
	test_reserve ();
	test_restore ();
	test_next ();
	test_del ();

	return 0;
}

