#include "../include/siphon/clock.h"
#include "../include/siphon/alloc.h"
#include "mu/mu.h"

static void
test_convert (void)
{
	time_t a, b;

	a = 123;

	b = SP_SEC_TO_NSEC (a);
	mu_assert_int_eq (b, 123000000000);
	mu_assert_int_eq (a, SP_NSEC_TO_SEC (b));

	b = SP_SEC_TO_USEC (a);
	mu_assert_int_eq (b, 123000000);
	mu_assert_int_eq (a, SP_USEC_TO_SEC (b));

	b = SP_SEC_TO_MSEC (a);
	mu_assert_int_eq (b, 123000);
	mu_assert_int_eq (a, SP_MSEC_TO_SEC (b));

	b = SP_SEC_TO_SEC (a);
	mu_assert_int_eq (b, 123);
	mu_assert_int_eq (a, SP_SEC_TO_SEC (b));

	b = SP_MSEC_TO_NSEC (a);
	mu_assert_int_eq (b, 123000000);
	mu_assert_int_eq (a, SP_NSEC_TO_MSEC (b));

	b = SP_MSEC_TO_USEC (a);
	mu_assert_int_eq (b, 123000);
	mu_assert_int_eq (a, SP_USEC_TO_MSEC (b));

	b = SP_USEC_TO_NSEC (a);
	mu_assert_int_eq (b, 123000);
	mu_assert_int_eq (a, SP_NSEC_TO_USEC (b));

	b = SP_NSEC_TO_NSEC (a);
	mu_assert_int_eq (b, 123);
	mu_assert_int_eq (a, SP_NSEC_TO_NSEC (b));
}

static void
test_remainder (void)
{
	time_t a, b;

	a = 100000023456;
	b = SP_NSEC_REM (a);
	mu_assert_int_eq (b, 23456);

	a = 100023456;
	b = SP_USEC_REM (a);
	mu_assert_int_eq (b, 23456000);

	a = 123456;
	b = SP_MSEC_REM (a);
	mu_assert_int_eq (b, 456000000);
}

static void
test_make (void)
{
	SpClock clock;

	clock = SP_CLOCK_MAKE_NSEC (123456789000);
	mu_assert_int_eq (clock.tv_sec, 123);
	mu_assert_int_eq (clock.tv_nsec, 456789000);

	clock = SP_CLOCK_MAKE_USEC (123456789);
	mu_assert_int_eq (clock.tv_sec, 123);
	mu_assert_int_eq (clock.tv_nsec, 456789000);

	clock = SP_CLOCK_MAKE_MSEC (123456);
	mu_assert_int_eq (clock.tv_sec, 123);
	mu_assert_int_eq (clock.tv_nsec, 456000000);
}

static void
test_get (void)
{
	SpClock clock = SP_CLOCK_MAKE_NSEC (123456789000);

	mu_assert_int_eq (SP_CLOCK_NSEC (&clock), 123456789000);
	mu_assert_int_eq (SP_CLOCK_USEC (&clock), 123456789);
	mu_assert_int_eq (SP_CLOCK_MSEC (&clock), 123456);
	mu_assert_int_eq (SP_CLOCK_SEC (&clock), 123);
}

static void
test_set (void)
{
	SpClock clock = SP_CLOCK_MAKE_NSEC (0);

	SP_CLOCK_SET_NSEC (&clock, 123456789000);
	mu_assert_int_eq (SP_CLOCK_NSEC (&clock), 123456789000);

	SP_CLOCK_SET_USEC (&clock, 123456789);
	mu_assert_int_eq (SP_CLOCK_NSEC (&clock), 123456789000);

	SP_CLOCK_SET_MSEC (&clock, 123456);
	mu_assert_int_eq (SP_CLOCK_NSEC (&clock), 123456000000);

	SP_CLOCK_SET_SEC (&clock, 123);
	mu_assert_int_eq (SP_CLOCK_NSEC (&clock), 123000000000);
}

static void
test_abs (void)
{
	SpClock clock = SP_CLOCK_MAKE_NSEC (123456789000);

	time_t abs;

	abs = SP_CLOCK_ABS_NSEC (&clock, 54321);
	mu_assert_int_eq (abs, 123456843321);

	abs = SP_CLOCK_ABS_USEC (&clock, 54321);
	mu_assert_int_eq (abs, 123511110);

	abs = SP_CLOCK_ABS_MSEC (&clock, 54321);
	mu_assert_int_eq (abs, 177777);
}

static void
test_rel (void)
{
	SpClock clock = SP_CLOCK_MAKE_NSEC (123456789000);

	time_t rel;

	rel = SP_CLOCK_REL_NSEC (&clock, 123456843321);
	mu_assert_int_eq (rel, 54321);

	rel = SP_CLOCK_REL_USEC (&clock, 123511110);
	mu_assert_int_eq (rel, 54321);

	rel = SP_CLOCK_REL_MSEC (&clock, 177777);
	mu_assert_int_eq (rel, 54321);
}

static void
test_add (void)
{
	SpClock clock, add;

	clock = SP_CLOCK_MAKE_NSEC (123456789000);
	add = SP_CLOCK_MAKE_NSEC (123456789000);
	SP_CLOCK_ADD (&clock, &add);
	mu_assert_int_eq (clock.tv_sec, 246);
	mu_assert_int_eq (clock.tv_nsec, 913578000);
}

static void
test_sub (void)
{
	SpClock clock, sub;

	clock = SP_CLOCK_MAKE_NSEC (123456789000);
	sub = SP_CLOCK_MAKE_NSEC (123456789000);
	SP_CLOCK_SUB (&clock, &sub);
	mu_assert_int_eq (clock.tv_sec, 0);
	mu_assert_int_eq (clock.tv_nsec, 0);
}

int
main (void)
{
	mu_init ("clock");

	test_convert ();
	test_remainder ();
	test_make ();
	test_get ();
	test_set ();
	test_abs ();
	test_rel ();
	test_add ();
	test_sub ();

	mu_assert (sp_alloc_summary ());
}

