#include "../include/siphon/path.h"
#include "../include/siphon/error.h"
#include "../include/siphon/alloc.h"

#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>

#define F_OPEN 1
#define F_SKIP 2
#define F_STAT 4

#ifdef __APPLE__

#include <mach-o/dyld.h>

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
		n--;
		rng->len = m - path - rng->off;
		if (rng->len == 0) {
			if (n == 0) {
				rng->len = 1;
			}
			break;
		}
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

int
sp_path_join (char *out, size_t len,
		const char *a, uint16_t alen,
		const char *b, uint16_t blen,
		SpPathMode mode)
{
	int olen = SP_PATH_EBUFS;

	// ignore b if it is empty
	if (blen == 0) {
		if (alen <= len) {
			olen = alen;
			memmove (out, a, olen);
		}
		goto done;
	}

	// ignore a if it is empty or b is absolute
	if (alen == 0 || *b == '/') {
		if (blen <= len) {
			olen = blen;
			memmove (out, b, olen);
		}
		goto done;
	}

	if (a[alen-1] == '/') {
		alen--;
	}
	olen = alen + blen + 1;
	if ((size_t)olen > len) {
		olen = SP_PATH_EBUFS;
		goto done;
	}
	memmove (out, a, alen);
	out[alen] = '/';
	memmove (out+alen+1, b, blen);
	if ((mode & SP_PATH_TRAIL_SLASH) && has_suffix_dir (b, blen)) {
		if (olen+1 > (uint16_t)len) {
			olen = SP_PATH_EBUFS;
			goto done;
		}
		out[olen++] = '/';
	}

done:
	if (olen >= 0 && len > (size_t)olen) {
		out[olen] = '\0';
	}
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

bool
sp_path_suffix (const char *path, const char *match)
{
	if (*match != '/') {
		const char *end = match;
		int n = 0;
		while ((end = strchr (end+1, '/'))) { n++; }
		SpRange16 a, b;
		sp_path_split (&a, &b, path, strnlen (path, SP_PATH_MAX), n+1);
		path += b.off;
	}
	return sp_path_match (path, match);
}

int
sp_path_proc (char *buf, size_t buflen)
{
	ssize_t len = -1;

#if defined(__linux)
	len = readlink ("/proc/self/exe", buf, buflen-1);
#elif defined(__APPLE__)
	if (buflen < SP_PATH_MAX) {
		return SP_ESYSTEM (ENOBUFS);
	}
	char tmp[SP_PATH_MAX*2];
	uint32_t tmplen = sizeof (tmp)-1;
	if (_NSGetExecutablePath (tmp, &tmplen) == 0) {
		tmp[tmplen] = '\0';
		if (realpath (tmp, buf)) {
			len = strlen (buf);
		}
	}
#elif defined(__FreeBSD__)
	int mib[4];
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;
	buflen--;
	if (sysctl (mib, 4, buf, &buflen, NULL, 0) == 0) {
		len = buflen;
	}
#elif defined(BSD)
	len = readlink ("/proc/curproc/file", buf, buflen-1);
#endif

	if (len > 0) {
		buf[len] = '\0';
		return (int)len;
	}
	return SP_ESYSTEM (errno);
}

int
sp_path_env (const char *name, char *buf, size_t buflen)
{
	if (realpath (name, buf) && access (buf, X_OK) == 0) {
		return strlen (buf);
	}

	char *path = getenv ("PATH");
	if (path == NULL) {
		return SP_ESYSTEM (errno);
	}

	size_t namelen = strlen (name), pathlen = 0;
	char *pathsep = NULL;
	int len;

	while ((pathsep = strchr (path, ':')) != NULL) {
		pathlen = pathsep - path;
		if (pathlen == 0 || buflen < pathlen) continue;
		len = sp_path_join (buf, buflen, path, pathlen, name, namelen, 0);
		path = pathsep + 1;
		if (len > 0 && access (buf, X_OK) == 0) {
			return len;
		}
	}

	return SP_PATH_ENOTFOUND;
}

int
sp_dir_open (SpDir *self, const char *path, uint8_t depth)
{
	uint16_t len = strnlen (path, sizeof self->path);
	if (len == sizeof self->path) {
		return SP_PATH_EBUFS;
	}

	if (sp_stat (path, &self->stat, true) < 0) {
		return SP_ESYSTEM (errno);
	}

	if (!S_ISDIR (self->stat.mode)) {
		return -ENOTDIR;
	}

	self->stack = NULL;
	if (depth) {
		self->stack = sp_calloc (depth, sizeof *self->stack);
		if (self->stack == NULL) {
			return SP_ESYSTEM (errno);
		}
	}

	self->flags = (depth ? F_OPEN : 0) | F_STAT;
	self->dirlen = len;
	self->pathlen = len;
	self->cur = 0;
	self->max = depth;
	memcpy (self->path, path, len);
	self->path[len] = '\0';
	return 0;
}

void
sp_dir_close (SpDir *self)
{
	assert (self != NULL);

	self->flags &= ~F_OPEN;
	for (int8_t i = 0; i < self->cur; i++) {
		closedir (self->stack[i]);
	}
	if (self->max > 0) {
		sp_free (self->stack, self->max * sizeof *self->stack);
		self->stack = NULL;
	}
	self->cur = 0;
	self->max = 0;
}

static bool
is_rel_dir (struct dirent *ent)
{
	return ent->d_name[0] == '.' && (ent->d_name[1] == '\0'
				|| (ent->d_name[1] == '.' && ent->d_name[2] == '\0'));

}

static int
walk (SpDir *self)
{
	struct dirent *ent;
	DIR *dir;
	int rc = 0, popn = self->empty ? 1 : 2;
	size_t len;

again:

	dir = self->stack[self->cur-1];
	do {
		errno = 0;
		ent = readdir (dir);
		if (ent == NULL) {
			rc = SP_ESYSTEM (errno);
			if (rc < 0) { return rc; }

			if (self->cur == 1) {
				rewinddir (dir);
				self->flags = F_OPEN;
				self->cur = 1;
				return 0;
			}

			closedir (dir);
			self->stack[--self->cur] = NULL;

			SpRange16 pop = { 0, self->pathlen };
			sp_path_pop (self->path, &pop, popn);
			popn = 1;
			self->dirlen = self->pathlen = pop.len;
			goto again;
		}
	} while (is_rel_dir (ent));

#ifdef SP_HAVE_DIRENT_NAMLEN
	size_t namlen = ent->d_namlen;
#else
	size_t namlen = strnlen (ent->d_name, sizeof ent->d_name);
#endif

	len = self->dirlen + namlen + 1;
	if (len >= sizeof self->path) {
		return SP_PATH_EBUFS;
	}

	self->pathlen = (uint16_t)len;
	self->path[self->dirlen] = '/';
	memcpy (self->path+self->dirlen+1, ent->d_name, namlen);
	self->path[len] = '\0';
	self->empty = false;

	if (ent->d_type == DT_UNKNOWN) {
		int rc = sp_stat (self->path, &self->stat, false);
		if (rc < 0) { return rc; }
		self->flags |= F_STAT;
	}
	else {
		self->stat.mode = DTTOIF (ent->d_type);
	}

	return 1;
}

int
sp_dir_next (SpDir *self)
{
	assert (self != NULL);

	if (!(self->flags & F_OPEN)) {
		return SP_PATH_ECLOSED;
	}

	if (self->cur < self->max &&
			!(self->flags & F_SKIP) &&
			sp_dir_type (self) == SP_PATH_DIR) {
		DIR *dir = opendir (self->path);
		if (dir == NULL) { return SP_ESYSTEM (errno); }
		self->dirlen = self->pathlen;
		self->stack[self->cur++] = dir;
		self->empty = true;
	}
	self->flags = F_OPEN;

	return walk (self);
}

void
sp_dir_skip (SpDir *self)
{
	assert (self != NULL);

	self->flags |= F_SKIP;
}

int
sp_dir_follow (SpDir *self)
{
	assert (self != NULL);

	if (sp_dir_type (self) == SP_PATH_LNK) {
		int rc = sp_stat (self->path, &self->stat, true);
		if (rc < 0) {
			return SP_ESYSTEM (errno);
		}
		self->flags |= F_STAT;
	}
	return sp_dir_type (self) == SP_PATH_DIR ? 0 : -ENOTDIR;
}

SpPathType
sp_dir_type (SpDir *self)
{
	assert (self != NULL);

	return IFTODT (self->stat.mode);
}

const SpStat *
sp_dir_stat (SpDir *self)
{
	assert (self != NULL);

	if (!(self->flags & F_STAT)) {
		int rc = sp_stat (self->path, &self->stat, false);
		if (rc < 0) {
			errno = -rc;
			return NULL;
		}
		self->flags |= F_STAT;
	}
	return &self->stat;
}

void
sp_dir_pathname (const SpDir *self, const char **start, size_t *len)
{
	assert (self != NULL);

	*start = self->path;
	*len = self->pathlen;
}

void
sp_dir_dirname (const SpDir *self, const char **start, size_t *len)
{
	assert (self != NULL);

	*start = self->path;
	*len = self->dirlen;
}

void
sp_dir_basename (const SpDir *self, const char **start, size_t *len)
{
	assert (self != NULL);

	*start = self->path + self->dirlen;
	*len = self->pathlen - self->dirlen;
}

static inline void
copy_stat (SpStat *sbuf, const struct stat *s)
{
	sbuf->device = s->st_dev;
	sbuf->mode = s->st_mode;
	sbuf->nlink = s->st_nlink;
	sbuf->uid = s->st_uid;
	sbuf->gid = s->st_gid;
	sbuf->rdev = s->st_rdev;
	sbuf->size = s->st_size;
#if SP_STAT_CLOCK
	sbuf->atime = s->st_atimespec;
	sbuf->mtime = s->st_mtimespec;
	sbuf->ctime = s->st_ctimespec;
#else
	sbuf->atime.tv_sec = s->st_atime;
	sbuf->atime.tv_nsec = 0;
	sbuf->mtime.tv_sec = s->st_mtime;
	sbuf->mtime.tv_nsec = 0;
	sbuf->ctime.tv_sec = s->st_ctime;
	sbuf->ctime.tv_nsec = 0;
#endif
}

int
sp_stat (const char *path, SpStat *sbuf, bool follow)
{
	struct stat s;
	int rc = follow ? lstat (path, &s) : stat (path, &s);
	if (rc < 0) {
		return SP_ESYSTEM (errno);
	}
	copy_stat (sbuf, &s);
	return 0;
}

int
sp_fstat (int fd, SpStat *sbuf)
{
	struct stat s;
	int rc = fstat (fd, &s);
	if (rc < 0) {
		return SP_ESYSTEM (errno);
	}
	copy_stat (sbuf, &s);
	return 0;
}

