/*
 * mu.h
 * https://github.com/kalamay/mu
 *
 * Copyright (c) 2015, Jeremy Larkin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MU_INCLUDED
#define MU_INCLUDED

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

static size_t mu_assert_count = 0, mu_failure_count = 0;
static const char *mu_name = "test";
static int mu_register = 0;

#define MU_CAT2(n, v) n##v
#define MU_CAT(n, v) MU_CAT2(n, v)
#define MU_TMP(n) MU_CAT(mu_##n, __LINE__)

#define mu_cfail(c, ...) do {                         \
	__sync_fetch_and_add (&mu_failure_count, 1);      \
	fprintf (stderr, "%s:%d: ", __FILE__, __LINE__ ); \
	fprintf (stderr, __VA_ARGS__);                    \
	fputc ('\n', stderr);                             \
	if ((c)) { mu_exit (); }                          \
} while (0)
#define mu_fail(...) mu_cfail(false, __VA_ARGS__)
#define mu_ffail(...) mu_cfail(true, __VA_ARGS__)

#define mu_cassert_msg(c, exp, ...) do {        \
	__sync_fetch_and_add (&mu_assert_count, 1); \
	if (!(exp)) {                               \
		mu_cfail (c, __VA_ARGS__);              \
	}                                           \
} while (0);
#define mu_assert_msg(...) mu_cassert_msg(false, __VA_ARGS__)
#define mu_fassert_msg(...) mu_cassert_msg(true, __VA_ARGS__)

#define mu_cassert_call(c, exp) do {                              \
	__sync_fetch_and_add (&mu_assert_count, 1);                   \
	if ((exp) != 0) {                                             \
		mu_cfail (c, "'%s' failed (%s)", #exp, strerror (errno)); \
	}                                                             \
} while (0);
#define mu_assert_call(exp) mu_cassert_call(false, exp)
#define mu_fassert_call(exp) mu_cassert_call(true, exp)

#define mu_assert(exp) mu_assert_msg(exp, "'%s' failed", #exp)
#define mu_fassert(exp) mu_fassert_msg(exp, "'%s' failed", #exp)

#define mu_cassert_int(c, a, OP, b) do {                                       \
	intmax_t MU_TMP(A) = (a);                                                  \
	intmax_t MU_TMP(B) = (b);                                                  \
	mu_cassert_msg(c, MU_TMP(A) OP MU_TMP(B),                                  \
	    "'%s' failed: %s=%jd, %s=%jd", #a#OP#b, #a, MU_TMP(A), #b, MU_TMP(B)); \
} while (0)
#define mu_assert_int_eq(a, b) mu_cassert_int(false, a, ==, b)
#define mu_fassert_int_eq(a, b) mu_cassert_int(true, a, ==, b)
#define mu_assert_int_ne(a, b) mu_cassert_int(false, a, !=, b)
#define mu_fassert_int_ne(a, b) mu_cassert_int(true, a, !=, b)
#define mu_assert_int_lt(a, b) mu_cassert_int(false, a, <,  b)
#define mu_fassert_int_lt(a, b) mu_cassert_int(true, a, <,  b)
#define mu_assert_int_le(a, b) mu_cassert_int(false, a, <=, b)
#define mu_fassert_int_le(a, b) mu_cassert_int(true, a, <=, b)
#define mu_assert_int_gt(a, b) mu_cassert_int(false, a, >,  b)
#define mu_fassert_int_gt(a, b) mu_cassert_int(true, a, >,  b)
#define mu_assert_int_ge(a, b) mu_cassert_int(false, a, >=, b)
#define mu_fassert_int_ge(a, b) mu_cassert_int(true, a, >=, b)

#define mu_cassert_uint(c, a, OP, b) do {                                      \
	uintmax_t MU_TMP(A) = (a);                                                 \
	uintmax_t MU_TMP(B) = (b);                                                 \
	mu_cassert_msg(c, MU_TMP(A) OP MU_TMP(B),                                  \
	    "'%s' failed: %s=%ju, %s=%ju", #a#OP#b, #a, MU_TMP(A), #b, MU_TMP(B)); \
} while (0)
#define mu_assert_uint_eq(a, b) mu_cassert_uint(false, a, ==, b)
#define mu_fassert_uint_eq(a, b) mu_cassert_uint(true, a, ==, b)
#define mu_assert_uint_ne(a, b) mu_cassert_uint(false, a, !=, b)
#define mu_fassert_uint_ne(a, b) mu_cassert_uint(true, a, !=, b)
#define mu_assert_uint_lt(a, b) mu_cassert_uint(false, a, <,  b)
#define mu_fassert_uint_lt(a, b) mu_cassert_uint(true, a, <,  b)
#define mu_assert_uint_le(a, b) mu_cassert_uint(false, a, <=, b)
#define mu_fassert_uint_le(a, b) mu_cassert_uint(true, a, <=, b)
#define mu_assert_uint_gt(a, b) mu_cassert_uint(false, a, >,  b)
#define mu_fassert_uint_gt(a, b) mu_cassert_uint(true, a, >,  b)
#define mu_assert_uint_ge(a, b) mu_cassert_uint(false, a, >=, b)
#define mu_fassert_uint_ge(a, b) mu_cassert_uint(true, a, >=, b)

#define mu_cassert_str(c, a, OP, b) do {                                             \
	const char *MU_TMP(A) = (const char *)(a);                                       \
	const char *MU_TMP(B) = (const char *)(b);                                       \
	mu_assert_msg (MU_TMP(A) == MU_TMP(B) ||                                         \
	    (MU_TMP(A) && MU_TMP(B) &&  0 OP strcmp (MU_TMP(A), MU_TMP(B))),             \
	    "'%s' failed: %s=\"%s\", %s=\"%s\"", #a#OP#b, #a, MU_TMP(A), #b, MU_TMP(B)); \
} while (0)
#define mu_assert_str_eq(a, b) mu_cassert_str(false, a, ==, b)
#define mu_fassert_str_eq(a, b) mu_cassert_str(true, a, ==, b)
#define mu_assert_str_ne(a, b) mu_cassert_str(false, a, !=, b)
#define mu_fassert_str_ne(a, b) mu_cassert_str(true, a, !=, b)
#define mu_assert_str_lt(a, b) mu_cassert_str(false, a, <,  b)
#define mu_fassert_str_lt(a, b) mu_cassert_str(true, a, <,  b)
#define mu_assert_str_le(a, b) mu_cassert_str(false, a, <=, b)
#define mu_fassert_str_le(a, b) mu_cassert_str(true, a, <=, b)
#define mu_assert_str_gt(a, b) mu_cassert_str(false, a, >,  b)
#define mu_fassert_str_gt(a, b) mu_cassert_str(true, a, >,  b)
#define mu_assert_str_ge(a, b) mu_cassert_str(false, a, >=, b)
#define mu_fassert_str_ge(a, b) mu_cassert_str(true, a, >=, b)

#define mu_cassert_ptr(c, a, OP, b) do {                                     \
	const void *MU_TMP(A) = (a);                                             \
	const void *MU_TMP(B) = (b);                                             \
	mu_assert_msg(MU_TMP(A) OP MU_TMP(B),                                    \
	    "'%s' failed: %s=%p, %s=%p", #a#OP#b, #a, MU_TMP(A), #b, MU_TMP(B)); \
} while (0)
#define mu_assert_ptr_eq(a, b) mu_cassert_ptr(false, a, ==, b)
#define mu_fassert_ptr_eq(a, b) mu_cassert_ptr(true, a, ==, b)
#define mu_assert_ptr_ne(a, b) mu_cassert_ptr(false, a, !=, b)
#define mu_fassert_ptr_ne(a, b) mu_cassert_ptr(true, a, !=, b)
	
#define mu_set(T, var, new) do {                              \
	T old;                                                    \
	do {                                                      \
		old = var;                                            \
	} while (!__sync_bool_compare_and_swap (&var, old, new)); \
} while (0)

static int
mu_final (void)
{
	__sync_synchronize ();
	size_t fails = mu_failure_count, asserts = mu_assert_count;
	const char *name = mu_name;
	mu_set (size_t, mu_failure_count, 0);
	mu_set (size_t, mu_assert_count, 0);
	int rc;
	if (fails == 0) {
		fprintf (stderr, "%8s: passed %zu assertion%s\n",
				name,
				asserts,
				asserts == 1 ? "" : "s");
		rc = EXIT_SUCCESS;
	}
	else {
		fprintf (stderr, "%8s: failed %zu of %zu assertion%s\n",
				name,
				fails,
				asserts,
				asserts == 1 ? "" : "s");
		rc = EXIT_FAILURE;
	}
	fflush (stderr);
	fflush (stdout);
	return rc;
}

static void __attribute__((noreturn))
mu_exit (void)
{
	_exit (mu_final ());
}

static void
mu_init (const char *name)
{
	__sync_synchronize ();
	mu_set (const char *, mu_name, name);
	mu_set (size_t, mu_failure_count, 0);
	mu_set (size_t, mu_assert_count, 0);
	if (__sync_fetch_and_add (&mu_register, 1) == 0) {
		atexit (mu_exit);
	}
}

#endif

