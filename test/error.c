#include "../include/siphon/error.h"
#include "../include/siphon/alloc.h"
#include "mu/mu.h"

#include <pthread.h>

static void
test_add (void)
{
	const SpError *e;

	e = sp_error (2001);
	mu_assert_ptr_eq (e, NULL);

	e = sp_error_add (2001, "test", "BAD", "some bad thing");
	mu_assert_ptr_ne (e, NULL);
	mu_assert_int_eq (e->code, -2001);
	mu_assert_str_eq (e->domain, "test");
	mu_assert_str_eq (e->name, "BAD");
	mu_assert_str_eq (e->msg, "some bad thing");

	e = sp_error_add (2001, "test", "WORSE", "some worse thing");
	mu_assert_ptr_eq (e, NULL);

	e = sp_error (2001);
	mu_assert_ptr_ne (e, NULL);
	mu_assert_int_eq (e->code, -2001);
	mu_assert_str_eq (e->domain, "test");
	mu_assert_str_eq (e->name, "BAD");
	mu_assert_str_eq (e->msg, "some bad thing");
}

static void
test_get (void)
{
	const SpError *e;

	e = sp_error (EINVAL);
	mu_assert_ptr_ne (e, NULL);
	mu_assert_int_eq (e->code, SP_ESYSTEM (EINVAL));
	mu_assert_str_eq (e->domain, "system");
	mu_assert_str_eq (e->name, "EINVAL");

	e = sp_error (SP_EAI_CODE (EAI_FAMILY));
	mu_assert_ptr_ne (e, NULL);
	mu_assert_int_eq (e->code, SP_EAI_CODE (EAI_FAMILY));
	mu_assert_str_eq (e->domain, "addr");
	mu_assert_str_eq (e->name, "EFAMILY");

	e = sp_error (SP_HTTP_ESYNTAX);
	mu_assert_ptr_ne (e, NULL);
	mu_assert_int_eq (e->code, SP_HTTP_ESYNTAX);
	mu_assert_str_eq (e->domain, "http");
	mu_assert_str_eq (e->name, "ESYNTAX");
}

static void
test_missing (void)
{
	const SpError *e;

	e = sp_error (0);
	mu_assert_ptr_eq (e, NULL);

	e = sp_error (150);
	mu_assert_ptr_eq (e, NULL);

	e = sp_error (9999);
	mu_assert_ptr_eq (e, NULL);
}

static void
test_iter (void)
{
	int last = 0;
	const SpError *e = NULL;
	while ((e = sp_error_next (e)) != NULL) {
		mu_assert_int_lt (e->code, last);
		last = e->code;
	}
}

static void *
add1 (void *val)
{
	(void)val;
	const SpError *e = sp_error_add (3001, "test", "ADD1", "thing 1");
	mu_assert_ptr_ne (e, NULL);
	return NULL;
}

static void *
add2 (void *val)
{
	(void)val;
	const SpError *e = sp_error_add (3002, "test", "ADD2", "thing 2");
	mu_assert_ptr_ne (e, NULL);
	return NULL;
}

static void *
add3 (void *val)
{
	(void)val;
	const SpError *e = sp_error_add (3003, "test", "ADD3", "thing 3");
	mu_assert_ptr_ne (e, NULL);
	return NULL;
}

static void
test_threads (void)
{
	pthread_t t[3];
	pthread_create (&t[0], NULL, add1, NULL);
	pthread_create (&t[1], NULL, add2, NULL);
	pthread_create (&t[2], NULL, add3, NULL);

	pthread_join (t[0], NULL);
	pthread_join (t[1], NULL);
	pthread_join (t[2], NULL);

	const SpError *e;

	e = sp_error (3001);
	mu_assert_str_eq (e->msg, "thing 1");

	e = sp_error (3002);
	mu_assert_str_eq (e->msg, "thing 2");

	e = sp_error (3003);
	mu_assert_str_eq (e->msg, "thing 3");
}

int
main (void)
{
	mu_init ("error");

	test_add ();
	test_get ();
	test_missing ();
	test_iter ();
	test_threads ();

	mu_assert (sp_alloc_summary ());
}

