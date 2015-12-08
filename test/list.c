#include "../include/siphon/list.h"
#include "../include/siphon/alloc.h"
#include "mu.h"

typedef struct {
	int value;
	SpList h;
} Thing;

static void
test_get (void)
{
	SpList list, *entry;
	Thing a, b, c, *t;

	a.value = 1;
	b.value = 2;
	c.value = 3;

	sp_list_init (&list);
	sp_list_add (&list, &a.h, SP_ASCENDING);
	sp_list_add (&list, &b.h, SP_ASCENDING);
	sp_list_add (&list, &c.h, SP_ASCENDING);

	entry = sp_list_get (&list, 0, SP_ASCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, a.value);

	entry = sp_list_get (&list, 1, SP_ASCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, b.value);

	entry = sp_list_get (&list, 2, SP_ASCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, c.value);

	entry = sp_list_get (&list, 3, SP_ASCENDING);
	mu_assert_ptr_eq (entry, NULL);

	entry = sp_list_get (&list, 0, SP_DESCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, c.value);

	entry = sp_list_get (&list, 1, SP_DESCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, b.value);

	entry = sp_list_get (&list, 2, SP_DESCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, a.value);

	entry = sp_list_get (&list, 3, SP_DESCENDING);
	mu_assert_ptr_eq (entry, NULL);
}

static void
test_ascending (void)
{
	SpList list, *entry;
	Thing a, b, c, *t;

	a.value = 1;
	b.value = 2;
	c.value = 3;

	sp_list_init (&list);
	sp_list_add (&list, &a.h, SP_ASCENDING);
	sp_list_add (&list, &b.h, SP_ASCENDING);
	sp_list_add (&list, &c.h, SP_ASCENDING);

	entry = sp_list_first (&list, SP_ASCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, a.value);

	mu_assert (sp_list_has_next (&list, entry, SP_ASCENDING));
	mu_assert (!sp_list_has_next (&list, entry, SP_DESCENDING));

	entry = sp_list_next (&list, entry, SP_ASCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, b.value);

	entry = sp_list_next (&list, entry, SP_ASCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, c.value);

	entry = sp_list_next (&list, entry, SP_ASCENDING);
	mu_assert_ptr_eq (entry, NULL);

	entry = sp_list_first (&list, SP_DESCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, c.value);

	mu_assert (!sp_list_has_next (&list, entry, SP_ASCENDING));
	mu_assert (sp_list_has_next (&list, entry, SP_DESCENDING));

	entry = sp_list_next (&list, entry, SP_DESCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, b.value);

	entry = sp_list_next (&list, entry, SP_DESCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, a.value);

	entry = sp_list_next (&list, entry, SP_DESCENDING);
	mu_assert_ptr_eq (entry, NULL);
}

static void
test_descending (void)
{
	SpList list, *entry;
	Thing a, b, c, *t;

	a.value = 1;
	b.value = 2;
	c.value = 3;

	sp_list_init (&list);
	sp_list_add (&list, &a.h, SP_DESCENDING);
	sp_list_add (&list, &b.h, SP_DESCENDING);
	sp_list_add (&list, &c.h, SP_DESCENDING);

	entry = sp_list_first (&list, SP_ASCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, c.value);

	mu_assert (sp_list_has_next (&list, entry, SP_ASCENDING));
	mu_assert (!sp_list_has_next (&list, entry, SP_DESCENDING));

	entry = sp_list_next (&list, entry, SP_ASCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, b.value);

	entry = sp_list_next (&list, entry, SP_ASCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, a.value);

	entry = sp_list_next (&list, entry, SP_ASCENDING);
	mu_assert_ptr_eq (entry, NULL);

	entry = sp_list_first (&list, SP_DESCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, a.value);

	mu_assert (!sp_list_has_next (&list, entry, SP_ASCENDING));
	mu_assert (sp_list_has_next (&list, entry, SP_DESCENDING));

	entry = sp_list_next (&list, entry, SP_DESCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, b.value);

	entry = sp_list_next (&list, entry, SP_DESCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, c.value);

	entry = sp_list_next (&list, entry, SP_DESCENDING);
	mu_assert_ptr_eq (entry, NULL);
}

static void
test_mixed (void)
{
	SpList list, *entry;
	Thing a, b, c;

	a.value = 1;
	b.value = 2;
	c.value = 3;

	sp_list_init (&list);
	sp_list_add (&list, &a.h, SP_ASCENDING);
	sp_list_add (&list, &b.h, SP_ASCENDING);
	sp_list_add (&list, &c.h, SP_DESCENDING);

	entry = sp_list_first (&list, SP_ASCENDING);
	mu_assert_int_eq (c.value, sp_container_of (entry, Thing, h)->value);
	entry = sp_list_next (&list, entry, SP_ASCENDING);
	mu_assert_int_eq (a.value, sp_container_of (entry, Thing, h)->value);
	entry = sp_list_next (&list, entry, SP_ASCENDING);
	mu_assert_int_eq (b.value, sp_container_of (entry, Thing, h)->value);
}

static void
test_shuffle (void)
{
	SpList list, *pos;
	Thing a, b, c;

	a.value = 1;
	b.value = 2;
	c.value = 3;

	sp_list_init (&list);
	sp_list_add (&list, &a.h, SP_ASCENDING);
	sp_list_add (&list, &b.h, SP_ASCENDING);
	sp_list_add (&list, &c.h, SP_ASCENDING);

	pos = sp_list_first (&list, SP_ASCENDING);
	mu_assert_int_eq (a.value, sp_container_of (pos, Thing, h)->value);

	sp_list_add (&list, pos, SP_DESCENDING);

	pos = sp_list_first (&list, SP_ASCENDING);
	mu_assert_int_eq (a.value, sp_container_of (pos, Thing, h)->value);
}

static void
test_insert (void)
{
	SpList list, *pos;
	Thing a, b, c;

	a.value = 1;
	b.value = 2;
	c.value = 3;

	sp_list_init (&list);
	sp_list_add (&list, &a.h, SP_ASCENDING);
	sp_list_add (&list, &b.h, SP_ASCENDING);
	sp_list_insert (&c.h, &a.h);

	pos = sp_list_first (&list, SP_ASCENDING);
	mu_assert_int_eq (c.value, sp_container_of (pos, Thing, h)->value);
	pos = sp_list_next (&list, pos, SP_ASCENDING);
	mu_assert_int_eq (a.value, sp_container_of (pos, Thing, h)->value);
	pos = sp_list_next (&list, pos, SP_ASCENDING);
	mu_assert_int_eq (b.value, sp_container_of (pos, Thing, h)->value);

	pos = sp_list_first (&list, SP_DESCENDING);
	mu_assert_int_eq (b.value, sp_container_of (pos, Thing, h)->value);
	pos = sp_list_next (&list, pos, SP_DESCENDING);
	mu_assert_int_eq (a.value, sp_container_of (pos, Thing, h)->value);
	pos = sp_list_next (&list, pos, SP_DESCENDING);
	mu_assert_int_eq (c.value, sp_container_of (pos, Thing, h)->value);
}

static void
test_pop (void)
{
	SpList list, *entry;
	Thing a, b, c, *t;

	a.value = 1;
	b.value = 2;
	c.value = 3;

	sp_list_init (&list);
	sp_list_add (&list, &a.h, SP_ASCENDING);
	sp_list_add (&list, &b.h, SP_ASCENDING);
	sp_list_add (&list, &c.h, SP_ASCENDING);

	entry = sp_list_pop (&list, SP_ASCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, a.value);

	entry = sp_list_pop (&list, SP_DESCENDING);
	t = sp_container_of (entry, Thing, h);
	mu_assert_int_eq (t->value, c.value);
}

static void
test_replace (void)
{
	SpList list1, list2, *entry;
	Thing a, b, c;

	a.value = 1;
	b.value = 2;
	c.value = 3;

	sp_list_init (&list1);
	sp_list_add (&list1, &a.h, SP_ASCENDING);
	sp_list_add (&list1, &b.h, SP_ASCENDING);
	sp_list_add (&list1, &c.h, SP_ASCENDING);

	sp_list_replace (&list2, &list1);

	mu_assert (sp_list_is_empty (&list1));
	mu_assert (!sp_list_is_empty (&list2));

	int i = 1;
	sp_list_each (&list2, entry, SP_ASCENDING) {
		Thing *t = sp_container_of (entry, Thing, h);
		mu_assert_int_eq (t->value, i);
		i++;
	}
	mu_assert_int_eq (i, 4);
}

static void
test_swap (void)
{
	SpList list1, list2, *entry;
	Thing a, b, c, x, y, z;
	int i;

	a.value = 1;
	b.value = 2;
	c.value = 3;

	sp_list_init (&list1);
	sp_list_add (&list1, &a.h, SP_ASCENDING);
	sp_list_add (&list1, &b.h, SP_ASCENDING);
	sp_list_add (&list1, &c.h, SP_ASCENDING);

	x.value = 7;
	y.value = 8;
	z.value = 9;

	sp_list_init (&list2);
	sp_list_add (&list2, &x.h, SP_ASCENDING);
	sp_list_add (&list2, &y.h, SP_ASCENDING);
	sp_list_add (&list2, &z.h, SP_ASCENDING);

	sp_list_swap (&list1, &list2);

	mu_assert (!sp_list_is_empty (&list1));
	mu_assert (!sp_list_is_empty (&list2));

	i = 7;
	sp_list_each (&list1, entry, SP_ASCENDING) {
		Thing *t = sp_container_of (entry, Thing, h);
		mu_assert_int_eq (t->value, i);
		i++;
	}
	mu_assert_int_eq (i, 10);

	i = 1;
	sp_list_each (&list2, entry, SP_ASCENDING) {
		Thing *t = sp_container_of (entry, Thing, h);
		mu_assert_int_eq (t->value, i);
		i++;
	}
	mu_assert_int_eq (i, 4);
}

static void
test_splice_asc (void)
{
	SpList list1, list2, *entry;
	Thing a, b, c, x, y, z;

	a.value = 1;
	b.value = 2;
	c.value = 3;

	sp_list_init (&list1);
	sp_list_add (&list1, &a.h, SP_ASCENDING);
	sp_list_add (&list1, &b.h, SP_ASCENDING);
	sp_list_add (&list1, &c.h, SP_ASCENDING);

	x.value = 7;
	y.value = 8;
	z.value = 9;

	sp_list_init (&list2);
	sp_list_add (&list2, &x.h, SP_ASCENDING);
	sp_list_add (&list2, &y.h, SP_ASCENDING);
	sp_list_add (&list2, &z.h, SP_ASCENDING);

	sp_list_splice (&list1, &list2, SP_ASCENDING);

	mu_assert (!sp_list_is_empty (&list1));
	mu_assert (sp_list_is_empty (&list2));

	Thing *expect[] = { &a, &b, &c, &x, &y, &z, NULL };
	size_t i = 0;

	sp_list_each (&list1, entry, SP_ASCENDING) {
		Thing *t = sp_container_of (entry, Thing, h);
		mu_assert_int_eq (t->value, expect[i]->value);
		i++;
	}
	mu_assert_uint_eq (i, sp_len (expect) - 1);
}

static void
test_splice_desc (void)
{
	SpList list1, list2, *entry;
	Thing a, b, c, x, y, z;

	a.value = 1;
	b.value = 2;
	c.value = 3;

	sp_list_init (&list1);
	sp_list_add (&list1, &a.h, SP_ASCENDING);
	sp_list_add (&list1, &b.h, SP_ASCENDING);
	sp_list_add (&list1, &c.h, SP_ASCENDING);

	x.value = 7;
	y.value = 8;
	z.value = 9;

	sp_list_init (&list2);
	sp_list_add (&list2, &x.h, SP_ASCENDING);
	sp_list_add (&list2, &y.h, SP_ASCENDING);
	sp_list_add (&list2, &z.h, SP_ASCENDING);

	sp_list_splice (&list1, &list2, SP_DESCENDING);

	mu_assert (!sp_list_is_empty (&list1));
	mu_assert (sp_list_is_empty (&list2));

	Thing *expect[] = { &x, &y, &z, &a, &b, &c, NULL };
	size_t i = 0;

	sp_list_each (&list1, entry, SP_ASCENDING) {
		Thing *t = sp_container_of (entry, Thing, h);
		mu_assert_int_eq (t->value, expect[i]->value);
		i++;
	}
	mu_assert_uint_eq (i, sp_len (expect) - 1);
}

static void
test_splice_inner_asc (void)
{
	SpList list1, list2, *entry;
	Thing a, b, c, x, y, z;

	a.value = 1;
	b.value = 2;
	c.value = 3;

	sp_list_init (&list1);
	sp_list_add (&list1, &a.h, SP_ASCENDING);
	sp_list_add (&list1, &b.h, SP_ASCENDING);
	sp_list_add (&list1, &c.h, SP_ASCENDING);

	x.value = 7;
	y.value = 8;
	z.value = 9;

	sp_list_init (&list2);
	sp_list_add (&list2, &x.h, SP_ASCENDING);
	sp_list_add (&list2, &y.h, SP_ASCENDING);
	sp_list_add (&list2, &z.h, SP_ASCENDING);

	entry = sp_list_get (&list1, 1, SP_ASCENDING);
	sp_list_splice (entry, &list2, SP_ASCENDING);

	mu_assert (!sp_list_is_empty (&list1));
	mu_assert (sp_list_is_empty (&list2));

	Thing *expect[] = { &a, &x, &y, &z, &b, &c, NULL };
	size_t i = 0;

	sp_list_each (&list1, entry, SP_ASCENDING) {
		Thing *t = sp_container_of (entry, Thing, h);
		mu_assert_int_eq (t->value, expect[i]->value);
		i++;
	}
	mu_assert_uint_eq (i, sp_len (expect) - 1);
}

static void
test_splice_inner_desc (void)
{
	SpList list1, list2, *entry;
	Thing a, b, c, x, y, z;

	a.value = 1;
	b.value = 2;
	c.value = 3;

	sp_list_init (&list1);
	sp_list_add (&list1, &a.h, SP_ASCENDING);
	sp_list_add (&list1, &b.h, SP_ASCENDING);
	sp_list_add (&list1, &c.h, SP_ASCENDING);

	x.value = 7;
	y.value = 8;
	z.value = 9;

	sp_list_init (&list2);
	sp_list_add (&list2, &x.h, SP_ASCENDING);
	sp_list_add (&list2, &y.h, SP_ASCENDING);
	sp_list_add (&list2, &z.h, SP_ASCENDING);

	entry = sp_list_get (&list1, 1, SP_ASCENDING);
	sp_list_splice (entry, &list2, SP_DESCENDING);

	mu_assert (!sp_list_is_empty (&list1));
	mu_assert (sp_list_is_empty (&list2));

	Thing *expect[] = { &a, &b, &x, &y, &z, &c, NULL };
	size_t i = 0;

	sp_list_each (&list1, entry, SP_ASCENDING) {
		Thing *t = sp_container_of (entry, Thing, h);
		mu_assert_int_eq (t->value, expect[i]->value);
		i++;
	}
	mu_assert_uint_eq (i, sp_len (expect) - 1);
}

int
main (void)
{
	mu_init ("list");

	test_get ();
	test_ascending ();
	test_descending ();
	test_mixed ();
	test_shuffle ();
	test_insert ();
	test_pop ();
	test_replace ();
	test_swap ();
	test_splice_asc ();
	test_splice_desc ();
	test_splice_inner_asc ();
	test_splice_inner_desc ();

	mu_assert (sp_alloc_summary ());
}

