#ifndef SIPHON_PATH_H
#define SIPHON_PATH_H

#include "common.h"

typedef enum {
	SP_PATH_TRAIL_SLASH = 1 << 0,  // keep trailing slash
	SP_PATH_ALLOW_EMPTY = 1 << 1,  // allow empty paths instead of '.'

	SP_PATH_URI = SP_PATH_TRAIL_SLASH | SP_PATH_ALLOW_EMPTY
} SpPathMode;

#ifndef SP_PATH_MAX
# define SP_PATH_MAX 4096
#endif

SP_EXPORT void
sp_path_pop (const char *path, SpRange16 *rng, int n);

SP_EXPORT void
sp_path_split (SpRange16 *a, SpRange16 *b, const char *path, uint16_t plen, int n);

SP_EXPORT void
sp_path_splitext (SpRange16 *a, SpRange16 *b, const char *path, uint16_t plen);

SP_EXPORT uint16_t
sp_path_join (char *out, size_t len,
		const char *a, uint16_t alen,
		const char *b, uint16_t blen,
		SpPathMode mode);

SP_EXPORT uint16_t
sp_path_clean (char *path, uint16_t len, SpPathMode mode);

SP_EXPORT bool
sp_path_match (const char *path, const char *match);

SP_EXPORT ssize_t
sp_path_proc (char *buf, size_t buflen);

SP_EXPORT ssize_t
sp_path_env (const char *name, char *buf, size_t buflen);

#endif

