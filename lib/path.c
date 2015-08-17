#include "siphon/path.h"

#include <string.h>
#include <errno.h>
#include <assert.h>

#ifdef __APPLE__

static void *
memrchr (const void *m, int c, size_t n)
{
	const uint8_t *s = m;
	c = (uint8_t)c;
	while (n--) if (s[n]==c) return (void *)(s+n);
	return 0;
}

#endif

void
sp_path_pop (const char *path, SpRange16 *rng, int n)
{
	assert (path != NULL);
	assert (rng != NULL);

	while (n > 0 && rng->len > 0) {
		const char *m = memrchr (path+rng->off, '/', rng->len);
		if (m == NULL) {
			rng->len = 0;
			break;
		}
		rng->len = m - path - rng->off;
		if (rng->len == 0) {
			rng->len = 1;
			break;
		}
		n--;
	}
}

static void
split_left (SpRange16 *a, SpRange16 *b, const char *path, uint16_t plen, int n)
{
	assert (plen > 0);

	const char *p = path;
	size_t rem = plen;
	size_t len = 0;

	if (n == 1 && plen == 1 && *p == '/') {
		goto out;
	}

	for (; n > 0 && rem > 0; n--, rem = plen - (p - path)) {
		const char *m = memchr (p, '/', rem);
		if (m == NULL) {
			// no furthur path separators
			p = path + plen;
			rem = 0;
			len = plen;
			break;
		}
		p = m + 1;
		len = m == path ? 1 : m - path;
	}

out:
	a->off = 0;
	a->len = len;
	b->off = p - path;
	b->len = rem;
}

static void
split_right (SpRange16 *a, SpRange16 *b, const char *path, uint16_t plen, int n)
{
	assert (plen > 0);

	const char *p = path;
	size_t rem = plen;
	size_t len;

	for (; n > 0; n--, rem = p - 1 - path) {
		const char *m = memrchr (path, '/', rem);
		if (m == NULL) {
			// path is not root, and we reached the end
			p = path;
			rem = 0;
			break;
		}

		// update second segment to after the slash
		p = m + 1;

		// path is root, and we reached the end
		if (m == path) {
			// short-circuit the loop given that we'd just find the end again
			if (n > 1) {
				rem = 0;
				p = path;
			}
			// this is the last match, so just set remain and bail
			else {
				rem = 1;
			}
			break;
		}
	}
	len = plen - (p - path);

	a->off = 0;
	a->len = rem;
	b->off = p - path;
	b->len = len;
}

void
sp_path_split (SpRange16 *a, SpRange16 *b, const char *path, uint16_t plen, int n)
{
	assert (a != NULL);
	assert (b != NULL);
	assert (path != NULL);
	assert (n != 0);

	if (plen == 0) {
		a->off = a->len = b->off = b->len = 0;
	}
	else if (n < 0) {
		split_left (a, b, path, plen, -n);
	}
	else {
		split_right (a, b, path, plen, n);
	}
}

void
sp_path_splitext (SpRange16 *a, SpRange16 *b, const char *path, uint16_t plen)
{
	assert (a != NULL);
	assert (b != NULL);
	assert (path != NULL);

	const char *s = path;
	size_t n = plen;

	a->off = 0;
	a->len = n;
	b->off = n;
	b->len = 0;

	while (n--) {
		if (s[n]=='.') {
			// split around the dot
			a->len = n;
			b->off = n + 1;
			b->len = plen - n - 1;
			break;
		}
		if (s[n]=='/') break;
	}
}

static inline bool
has_suffix_dir (const char *buf, uint16_t len)
{
	return (len == 1 && buf[0] == '.')
		|| (len == 2 && buf[0] == '.' && buf[1] == '.')
		|| (len >= 2 && buf[len-2] == '/' && buf[len-1] == '.')
		|| (len >= 3 && buf[len-3] == '/' && buf[len-2] == '.' && buf[len-1] == '.')
		;
}

uint16_t
sp_path_join (char *out, size_t len,
		const char *a, uint16_t alen,
		const char *b, uint16_t blen,
		SpPathMode mode)
{
	uint16_t olen;
	int err = 0;

	// ignore b if it is empty
	if (blen == 0) {
		olen = alen;
		if (olen > len) {
			olen = 0;
			err = ENAMETOOLONG;
		}
		else {
			memmove (out, a, olen);
		}
		goto done;
	}

	// ignore a if it is empty or b is absolute
	if (alen == 0 || *b == '/') {
		olen = blen;
		if (olen > len) {
			olen = 0;
			err = ENAMETOOLONG;
		}
		else {
			memmove (out, b, olen);
		}
		goto done;
	}

	if (a[alen-1] == '/') {
		alen--;
	}
	olen = alen + blen + 1;
	if (olen > len) {
		olen = 0;
		err = ENAMETOOLONG;
		goto done;
	}
	memmove (out, a, alen);
	out[alen] = '/';
	memmove (out+alen+1, b, blen);
	if ((mode & SP_PATH_TRAIL_SLASH) && has_suffix_dir (b, blen)) {
		if (olen+1 > (uint16_t)len) {
			olen = 0;
			err = ENAMETOOLONG;
			goto done;
		}
		out[olen++] = '/';
	}

done:
	if (len > olen) {
		out[olen] = '\0';
	}
	errno = err;
	return olen;
}

/*
 * Changes path to the shortest name equivalent to path by purely lexical processing.
 * It applies the following rules iteratively until no further processing can be done:
 *
 * 1. Replace multiple separator elements with a single one.
 * 2. Eliminate each . path name element (the current directory).
 * 3. Eliminate each inner .. path name element (the parent directory)
 *    along with the non-.. element that precedes it.
 * 4. Eliminate .. elements that begin a rooted path:
 *    that is, replace "/.." by "/" at the beginning of a path
 *
 * (based on: http://golang.org/pkg/path/filepath/#Clean)
 */
uint16_t
sp_path_clean (char *path, uint16_t len, SpPathMode mode)
{
	assert (path != NULL);

	bool rooted = path[0] == '/';
	char *p, *w, *up, *pe;

	p = up = path;
	pe = p + len;
	w = rooted ? p + 1 : p;

	while (p < pe) {
		switch (*p) {
			case '/':
				// empty path element
				p++;
				break;
			case '.':
				if (p+1 == pe || *(p+1) == '/') {
					// . element
					p++;
					break;
				}
				if (*(p+1) == '.' && (p+2 == pe || *(p+2) == '/')) {
					// .. element: remove to last /
					p += 2;
					if (w > up) {
						// can backtrack
						for (w--; w > up && *w != '/'; w--);
					}
					else if (!rooted) {
						// cannot backtrack, but not rooted, so append .. element.
						if (w > path) {
							*(w++) = '/';
						}
						*(w++) = '.';
						*(w++) = '.';
						up = w;
					}
					break;
				}
				// fallthrough
			default:
				// real path element.
				// add slash if needed
				if ((rooted && w != path+1) || (!rooted && w != path)) {
					*(w++) = '/';
				}
				// copy element
				for (; p < pe && *p != '/'; p++) {
					*(w++) = *p;
				}
				break;
		}
	}
	if (!(mode & SP_PATH_ALLOW_EMPTY) && w == path) {
		*(w++) = '.';
	}
	if ((mode & SP_PATH_TRAIL_SLASH) && (rooted || w > path) && *(p-1) == '/') {
		*(w++) = '/';
	}
	if (w < pe) {
		*w = '\0';
	}
	return w - path;
}

static bool
match_bracket (const char **p, const char **m)
{
	bool neg = false;
	if (**m == '!' || **m == '^') {
		(*m)++;
		neg = true;
	}

	while (1) {
		switch (**m) {
			case '\0':
				return false;
			case ']':
				if (!neg) return false;
				(*p)++;
				return true;
			case '\\':
				*m = *m + 1;
				// fallthrough
			default:
				if (**m == **p) {
					if (neg) return false;
					const char *me = strchr (*m, ']');
					if (me == NULL) return false;
					(*p)++;
					*m = me;
					return true;
				}
				break;
		}
		(*m)++;
	}
}

static bool
match_brace (const char **p, const char **m)
{
	const char *mark = *m;
	ssize_t len = -1;
	while (1) {
		switch (**m) {
			case '\0':
				return false;
			case '}':
				if (*m - mark > len && strncmp (*p, mark, *m - mark) == 0) {
					len = *m - mark;
				}
				if (len < 0) return false;
				(*p) += len;
				return true;
			case ',':
				if (*m - mark > len && strncmp (*p, mark, *m - mark) == 0) {
					len = *m - mark;
				}
				mark = *m + 1;
				break;
		}
		(*m)++;
	}
}

bool
sp_path_match (const char *path, const char *match)
{
	assert (path != NULL);
	assert (match != NULL);

	const char *p = path;
	const char *m = match;

	while (*p && *m) {
		switch (*m) {
			case '?':
				if (*p == '/' || *p == '.' || *p == '\0') {
					return false;
				}
				p++;
				break;
			case '*':
				while (*p != '/' && *p != '.' && *p != '\0' && *p != m[1]) {
					p++;
				}
				break;
			case '[':
				m++;
				if (!match_bracket (&p, &m)) {
					return false;
				}
				break;
			case '{':
				m++;
				if (!match_brace (&p, &m)) {
					return false;
				}
				break;
			case '\\':
				m++;
				// fallthrough
			default:
				if (*p != *m) {
					return false;
				}
				p++;
				break;
		}
		m++;
	}

	return *p == '\0' && *m == '\0';
}

