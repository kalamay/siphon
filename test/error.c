#include <errno.h>

#include "siphon/error.h"
#include "mu/mu.h"

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
	mu_assert_str_eq (e->domain, "getaddrinfo");
	mu_assert_str_eq (e->name, "EAI_FAMILY");

	e = sp_error (SP_HTTP_ESYNTAX);
	mu_assert_ptr_ne (e, NULL);
	mu_assert_int_eq (e->code, SP_HTTP_ESYNTAX);
	mu_assert_str_eq (e->domain, "http");
	mu_assert_str_eq (e->name, "SP_HTTP_ESYNTAX");
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

int
main (void)
{
	test_add ();
	test_get ();
	test_iter ();

	mu_exit ("error");
}

