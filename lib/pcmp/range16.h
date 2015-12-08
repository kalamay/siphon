/*
 * range16.h
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
#ifndef PCMP_RANGE16_H
#define PCMP_RANGE16_H

#include "common.h"

# ifdef __SSE4_2__

/**
 * Finds the index of the first character matching the range string.
 *
 * The range string is a sequence of character pairs where the range
 * is a value inclusively between the pair of characters.
 *
 * @param  cmp   string to search
 * @param  clen  length of search string
 * @param  rng   range string limited to 8 range pairs
 * @param  rlen  length of range string
 * @return  pointer to match or NULL
 */
static inline const uint8_t *
pcmp_range16 (const uint8_t *restrict cmp, int clen,
              const uint8_t *restrict rng, int rlen)
{
	// load range
	const __m128i s1 = _mm_lddqu_si128 ((const __m128i *)rng);

	do {
		// load string
		const __m128i s2 = _mm_lddqu_si128 ((const __m128i *)cmp);

		// compare strings and find the first matched index
		int c = _mm_cmpestri (s1, rlen, s2, clen, _SIDD_CMP_RANGES);

		// return the offset if an index was found
		if (pcmp_unlikely (c != 16)) {
			return cmp + c;
		}

		// update positions
		cmp += 16;
		clen -= 16;
	} while (clen > 0);

	return NULL;
}

# else

#include "range.h"

#define pcmp_range16 pcmp_range

# endif

#endif

