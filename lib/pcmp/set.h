/*
 * set.h
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
#ifndef PCMP_SET_H
#define PCMP_SET_H

#include "common.h"

# ifdef __SSE4_2__

#include "set16.h"

/**
 * Finds the the first character matching any character in the set string.
 *
 * @param  cmp   string to search
 * @param  clen  length of search string
 * @param  set   set string
 * @param  slen  length of set string
 * @return  pointer to match or NULL
 */
static inline const uint8_t *
pcmp_set (const uint8_t *restrict cmp, int clen,
          const uint8_t *restrict set, int slen)
{
	if (pcmp_likely (slen <= 16)) {
		return pcmp_set16 (cmp, clen, set, slen);
	}

	do {
		// load string
		const __m128i s2 = _mm_lddqu_si128 ((const __m128i *)cmp);

		// mark set values
		const uint8_t *restrict setp = set;
		int splen = slen;

		do {
			// load set
			const __m128i s1 = _mm_lddqu_si128 ((const __m128i *)setp);

			// compare strings and find the first matched index
			int c = _mm_cmpestri (s1, splen, s2, clen, _SIDD_CMP_EQUAL_ANY);

			// return the offset if an index was found
			if (pcmp_unlikely (c != 16)) {
				return cmp + c;
			}

			setp += 16;
			splen -= 16;
		} while (splen > 0);

		// update positions
		cmp += 16;
		clen -= 16;
	} while (clen > 0);

	return NULL;
}

# else

#include <stddef.h>

static inline const uint8_t *
pcmp_set (const uint8_t *restrict cmp, int clen,
          const uint8_t *restrict set, int slen)
{
	for (int i = 0; i < clen; i++, cmp++) {
		for (int s = 0; s < slen; s++) {
			if (*cmp == set[s]) {
				return cmp;
			}
		}
	}
	return NULL;
}

# endif

#endif

