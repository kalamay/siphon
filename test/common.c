#include "../include/siphon/common.h"
#include "mu.h"

static void
test_power_of_2_uint8 (void)
{
	uint8_t a, b;
	
	a = 0;
	b = sp_power_of_2 (a);
	mu_assert_uint_eq (b, 0);

	a = 1;
	b = sp_power_of_2 (a);
	mu_assert_uint_eq (b, 1);

	a = 2;
	b = sp_power_of_2 (a);
	mu_assert_uint_eq (b, 2);

	a = 3;
	b = sp_power_of_2 (a);
	mu_assert_uint_eq (b, 4);

	a = 12;
	b = sp_power_of_2 (a);
	mu_assert_uint_eq (b, 16);

	a = 120;
	b = sp_power_of_2 (a);
	mu_assert_uint_eq (b, 128);

	a = 200;
	b = sp_power_of_2 (a);
	mu_assert_uint_eq (b, 0);
}

static void
test_power_of_2_uint16 (void)
{
	uint16_t a, b;
	
	a = 0;
	b = sp_power_of_2 (a);
	mu_assert_uint_eq (b, 0);

	a = 1;
	b = sp_power_of_2 (a);
	mu_assert_uint_eq (b, 1);

	a = 2;
	b = sp_power_of_2 (a);
	mu_assert_uint_eq (b, 2);

	a = 3;
	b = sp_power_of_2 (a);
	mu_assert_uint_eq (b, 4);

	a = 12;
	b = sp_power_of_2 (a);
	mu_assert_uint_eq (b, 16);

	a = 120;
	b = sp_power_of_2 (a);
	mu_assert_uint_eq (b, 128);

	a = 200;
	b = sp_power_of_2 (a);
	mu_assert_uint_eq (b, 256);

	a = 1025;
	b = sp_power_of_2 (a);
	mu_assert_uint_eq (b, 2048);
}

static void
test_power_of_2_prime (void)
{
	mu_assert_uint_eq (sp_power_of_2_prime (0), 0);
	mu_assert_uint_eq (sp_power_of_2_prime (1), 1);
	mu_assert_uint_eq (sp_power_of_2_prime (2), 2);
	mu_assert_uint_eq (sp_power_of_2_prime (3), 2);
	mu_assert_uint_eq (sp_power_of_2_prime (4), 3);
	mu_assert_uint_eq (sp_power_of_2_prime (8), 7);
	mu_assert_uint_eq (sp_power_of_2_prime (31), 13);
	mu_assert_uint_eq (sp_power_of_2_prime (32), 31);
	mu_assert_uint_eq (sp_power_of_2_prime (33), 31);
}

int
main (void)
{
	mu_init ("common");

	test_power_of_2_uint8 ();
	test_power_of_2_uint16 ();
	test_power_of_2_prime ();
}

