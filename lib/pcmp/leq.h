/*
 * leq.h
 * https://github.com/kalamay/pcmp
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
#ifndef PCMP_LEQ_H
#define PCMP_LEQ_H

#include "common.h"

# ifdef __SSE4_2__

/**
 * Compares the equality of a mixed case string to a lower case string.
 *
 * @param  s1  mixed case string to compare
 * @param  s2  lower case string
 * @param  n   length of strings
 * @return  1 if the strings are equal, 0 otherwise
 */
static inline int
pcmp_leq (const uint8_t *restrict s1, const uint8_t *restrict s2, int n)
{
	int c, result = 0;

	static const uint8_t brng[2]  = {'A','Z'};
	static const uint8_t badd[16] = {
		32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32
	};

	__m128i rng = _mm_loadu_si128 ((const __m128i *)brng);
	__m128i add = _mm_loadu_si128 ((const __m128i *)badd);
	__m128i zer = _mm_setzero_si128 ();

	do {
		__m128i m1, m2, mask;
		// load mixed case string
		m1 = _mm_lddqu_si128 ((const __m128i *)s1);
		// mask has 255 for each upper case character and 0 for the rest
		mask = _mm_cmpestrm (rng, 2, m1, n - result,
				_SIDD_CMP_RANGES | _SIDD_UNIT_MASK);
		// change each 255 to 32
		mask = _mm_blendv_epi8 (zer, add, mask);
		// add mask to string which changes upper case to lower
		m1 = _mm_add_epi8 (m1, mask);
		// load lower case string
		m2 = _mm_lddqu_si128 ((const __m128i *)s2);
		// compare strings and find the last matched index
		c = _mm_cmpestri (m1, n - result, m2, 16,
				_SIDD_CMP_EQUAL_EACH | _SIDD_MASKED_NEGATIVE_POLARITY);
		// update positions
		s1 += 16;
		s2 += 16;
		result += c;
	} while (c == 16);

	return result == n;
}

# else

#include <ctype.h>

static inline int
pcmp_leq (const uint8_t *restrict s1, const uint8_t *restrict s2, int n)
{
	for (; n && *s2 == tolower (*s1); s2++, s1++, n--);
	return n == 0;
}

# endif

#endif

