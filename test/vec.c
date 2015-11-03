#include "../include/siphon/vec.h"
#include "mu/mu.h"

#include <errno.h>

static void
test_free (void)
{
	int *vec = NULL;

	sp_vec_free (vec);
	mu_assert_ptr_eq (vec, NULL);

	sp_vec_push (vec, 1);
	mu_assert_ptr_ne (vec, NULL);

	sp_vec_free (vec);
	mu_assert_ptr_eq (vec, NULL);
}

static void
test_ensure (void)
{
	int *vec = NULL;

	mu_assert_int_eq (sp_vec_count (vec), 0);
	mu_assert_int_eq (sp_vec_capacity (vec), 0);
	mu_assert_int_eq (sp_vec_ensure (vec, 5), 0);
	mu_assert_int_eq (sp_vec_count (vec), 0);
	mu_assert_int_ge (sp_vec_capacity (vec), 5);

	sp_vec_free (vec);
}

static void
test_push (void)
{
	int *vec = NULL;
	mu_assert_uint_eq (sp_vec_count (vec), 0);

	sp_vec_push (vec, 1);
	mu_assert_uint_eq (sp_vec_count (vec), 1);

	sp_vec_push (vec, 2);
	mu_assert_uint_eq (sp_vec_count (vec), 2);

	sp_vec_push (vec, 'c');
	mu_assert_uint_eq (sp_vec_count (vec), 3);

	int vals[] = { 10, 11, 12 };
	sp_vec_pushn (vec, vals, sp_len (vals));
	mu_assert_uint_eq (sp_vec_count (vec), 6);

	mu_assert_int_eq (vec[0], 1);
	mu_assert_int_eq (vec[1], 2);
	mu_assert_int_eq (vec[2], 'c');
	mu_assert_int_eq (vec[3], 10);
	mu_assert_int_eq (vec[4], 11);
	mu_assert_int_eq (vec[5], 12);

	sp_vec_free (vec);
	mu_assert_uint_eq (sp_vec_count (vec), 0);
}

static void
test_grow (void)
{
	int *vec = NULL;

	int grows = 0;
	for (int i = 1; i < 100; i++) {
		size_t before = sp_vec_capacity (vec);
		sp_vec_push (vec, i);
		size_t after = sp_vec_capacity (vec);
		if (after > before) grows++;
	}

	mu_assert_uint_eq (sp_vec_count (vec), 99);
	mu_assert_int_eq (grows, 4);
}

static void
test_pop (void)
{
	int *vec = NULL;

	int vals[] = { 1, 2, 3 };
	sp_vec_pushn (vec, vals, sp_len (vals));

	int val;

	val = sp_vec_pop (vec, -1);
	mu_assert_int_eq (val, 3);

	val = sp_vec_pop (vec, -1);
	mu_assert_int_eq (val, 2);

	val = sp_vec_pop (vec, -1);
	mu_assert_int_eq (val, 1);

	val = sp_vec_pop (vec, -1);
	mu_assert_int_eq (val, -1);

	mu_assert_uint_eq (sp_vec_count (vec), 0);
	sp_vec_free (vec);
	mu_assert_uint_eq (sp_vec_count (vec), 0);
}

static void
test_popn (void)
{
	int *vec = NULL;

	int vals[] = { 1, 2, 3, 4 };
	sp_vec_pushn (vec, vals, sp_len (vals));

	int out[3], rc;

	rc = sp_vec_popn (vec, out, sp_len (out));
	mu_assert_int_eq (rc, 0);
	mu_assert_int_eq (out[0], 2);
	mu_assert_int_eq (out[1], 3);
	mu_assert_int_eq (out[2], 4);
	mu_assert_uint_eq (sp_vec_count (vec), 1);

	rc = sp_vec_popn (vec, out, sp_len (out));
	mu_assert_int_eq (rc, -ERANGE);

	sp_vec_free (vec);
	mu_assert_uint_eq (sp_vec_count (vec), 0);
}

static void
test_shift (void)
{
	int *vec = NULL;

	int vals[] = { 1, 2, 3 };
	sp_vec_pushn (vec, vals, sp_len (vals));

	int val;

	val = sp_vec_shift (vec, -1);
	mu_assert_int_eq (val, 1);
	mu_assert_int_eq (vec[0], 2);
	mu_assert_int_eq (vec[1], 3);

	val = sp_vec_shift (vec, -1);
	mu_assert_int_eq (val, 2);
	mu_assert_int_eq (vec[0], 3);

	val = sp_vec_shift (vec, -1);
	mu_assert_int_eq (val, 3);

	val = sp_vec_shift (vec, -1);
	mu_assert_int_eq (val, -1);

	mu_assert_uint_eq (sp_vec_count (vec), 0);
	sp_vec_free (vec);
	mu_assert_uint_eq (sp_vec_count (vec), 0);
}

static void
test_shiftn (void)
{
	int *vec = NULL;

	int vals[] = { 1, 2, 3, 4 };
	sp_vec_pushn (vec, vals, sp_len (vals));

	int out[3], rc;

	rc = sp_vec_shiftn (vec, out, sp_len (out));
	mu_assert_int_eq (rc, 0);
	mu_assert_int_eq (out[0], 1);
	mu_assert_int_eq (out[1], 2);
	mu_assert_int_eq (out[2], 3);
	mu_assert_uint_eq (sp_vec_count (vec), 1);

	rc = sp_vec_shiftn (vec, out, sp_len (out));
	mu_assert_int_eq (rc, -ERANGE);

	sp_vec_free (vec);
	mu_assert_uint_eq (sp_vec_count (vec), 0);
}

static void
test_splice (void)
{
	int values[] = { 1, 2, 3 };
	int *vec = NULL;

	sp_vec_splice (vec, 0, 0, values, sp_len (values));

	mu_assert_int_eq (sp_vec_count (vec), 3);
	mu_assert_int_eq (vec[0], 1);
	mu_assert_int_eq (vec[1], 2);
	mu_assert_int_eq (vec[2], 3);

	sp_vec_free (vec);
}

static void
test_splice_offset (void)
{
	int values[] = { 1, 2, 3 };
	int *vec = NULL;

	int rc = sp_vec_splice (vec, 2, 2, values, sp_len (values));

	mu_assert_int_eq (rc, -ERANGE);

	sp_vec_free (vec);
}

static void
test_splice_remove (void)
{
	int first[] = { 1, 2, 3, 4 };
	int second[] = { 10, 20, 30 };
	int *vec = NULL;

	int rc = sp_vec_splice (vec, 0, -1, NULL, 0);
	mu_assert_int_eq (rc, -ERANGE);

	sp_vec_splice (vec, 0, 0, first, sp_len (first));
	sp_vec_splice (vec, 0, 2, second, sp_len (second));

	mu_assert_int_eq (vec[0], 10);
	mu_assert_int_eq (vec[1], 20);
	mu_assert_int_eq (vec[2], 30);
	mu_assert_int_eq (vec[3], 3);
	mu_assert_int_eq (vec[4], 4);

	rc = sp_vec_splice (vec, -3, -1, first, 0);
	mu_assert_int_eq (rc, 0);
	mu_assert_uint_eq (sp_vec_count (vec), 2);
	mu_assert_int_eq (vec[0], 10);
	mu_assert_int_eq (vec[1], 20);

	rc = sp_vec_splice (vec, 0, -1, first, 0);
	mu_assert_int_eq (rc, 0);
	mu_assert_uint_eq (sp_vec_count (vec), 0);

	sp_vec_free (vec);
}

static void
test_splice_remove_offset (void)
{
	int first[] = { 1, 2, 3, 4 };
	int second[] = { 10, 20, 30 };
	int *vec = NULL;

	sp_vec_splice (vec, 0, 0, first, sp_len (first));
	sp_vec_splice (vec, 1, 3, second, sp_len (second));

	mu_assert_int_eq (vec[0], 1);
	mu_assert_int_eq (vec[1], 10);
	mu_assert_int_eq (vec[2], 20);
	mu_assert_int_eq (vec[3], 30);
	mu_assert_int_eq (vec[4], 4);

	sp_vec_free (vec);
}

static void
test_splice_remove_end (void)
{
	int first[] = { 1, 2, 3, 4 };
	int second[] = { 10, 20, 30 };
	int *vec = NULL;

	sp_vec_splice (vec, 0, 0, first, sp_len (first));
	sp_vec_splice (vec, -2, -1, second, sp_len (second));

	mu_assert_int_eq (vec[0], 1);
	mu_assert_int_eq (vec[1], 2);
	mu_assert_int_eq (vec[2], 10);
	mu_assert_int_eq (vec[3], 20);
	mu_assert_int_eq (vec[4], 30);

	sp_vec_free (vec);
}

static void
test_splice_after (void)
{
	int first[] = { 1, 2, 3, 4 };
	int second[] = { 10, 20, 30 };
	int *vec = NULL;

	sp_vec_splice (vec, 0, 0, first, sp_len (first));
	int rc = sp_vec_splice (vec, 8, 10, second, sp_len (second));

	mu_assert_int_eq (rc, -ERANGE);

	sp_vec_free (vec);
}

static void
test_reverse (void)
{
	int *vec = NULL;

	sp_vec_reverse (vec);
	mu_assert_uint_eq (sp_vec_count (vec), 0);

	int vals[] = { 1, 2, 3, 4, 5 };
	sp_vec_pushn (vec, vals, sp_len (vals));
	mu_assert_uint_eq (sp_vec_count (vec), 5);

	sp_vec_reverse (vec);
	mu_assert_int_eq (vec[0], 5);
	mu_assert_int_eq (vec[1], 4);
	mu_assert_int_eq (vec[2], 3);
	mu_assert_int_eq (vec[3], 2);
	mu_assert_int_eq (vec[4], 1);
	mu_assert_uint_eq (sp_vec_count (vec), 5);

	sp_vec_free (vec);
}

int
main (void)
{
	mu_init ("vec");

	test_free ();
	test_ensure ();
	test_push ();
	test_grow ();
	test_pop ();
	test_popn ();
	test_shift ();
	test_shiftn ();
	test_splice ();
	test_splice_offset ();
	test_splice_remove ();
	test_splice_remove_offset ();
	test_splice_remove_end ();
	test_splice_after ();
	test_reverse ();
}

