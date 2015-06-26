#include <stdio.h>
#include "siphon/endian.h"

#include "siphon/siphon.h"
#include "mu/mu.h"

static const char * test_key_63 = "012345678901234567890123456789012345678901234567890123456789012";

static void
test_metrohash (void)
{
	mu_assert_uint_eq (sp_metrohash64 (test_key_63, 63, 0), 0x400E735C4F048F65);
	mu_assert_uint_eq (sp_metrohash64 (test_key_63, 63, 1), 0x7B5356A8B0EB49AE);
}

int
main (void)
{
	test_metrohash ();

	mu_exit ("hash");
}

