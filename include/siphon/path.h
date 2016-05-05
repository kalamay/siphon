#ifndef SIPHON_PATH_H
#define SIPHON_PATH_H

#include "common.h"
#include "range.h"
#include "clock.h"

#include <time.h>

#ifndef SP_PATH_MAX
# define SP_PATH_MAX 4096
#endif

#ifndef SP_PATH_ITER_DEPTH
# define SP_PATH_ITER_DEPTH 32
#endif

typedef enum {
	SP_PATH_TRAIL_SLASH = 1 << 0,  // keep trailing slash
	SP_PATH_ALLOW_EMPTY = 1 << 1,  // allow empty paths instead of '.'

	SP_PATH_URI = SP_PATH_TRAIL_SLASH | SP_PATH_ALLOW_EMPTY
} SpPathMode;

typedef enum {
	SP_PATH_UNKNOWN = 0,
	SP_PATH_FIFO    = 1,
	SP_PATH_CHR     = 2,
	SP_PATH_DIR     = 4,
	SP_PATH_BLK     = 6,
	SP_PATH_REG     = 8,
	SP_PATH_LNK     = 10,
	SP_PATH_SOCK    = 12,
	SP_PATH_WHT     = 14,
} SpPathType;

typedef struct {
	unsigned long device;
	unsigned int  mode;
	unsigned int  nlink;
	unsigned int  uid;
	unsigned int  gid;
	unsigned long rdev;
	long          size;
	SpClock       atime;
	SpClock       mtime;
	SpClock       ctime;
} SpStat;

typedef struct {
	void **stack;
	uint16_t flags, dirlen, pathlen;
	uint8_t cur, max;
	SpStat stat;
	bool empty;
	char path[SP_PATH_MAX];
} SpDir;

SP_EXPORT void
sp_path_pop (const char *path, SpRange16 *rng, int n);

SP_EXPORT void
sp_path_split (SpRange16 *a, SpRange16 *b, const char *path, uint16_t plen, int n);

SP_EXPORT void
sp_path_splitext (SpRange16 *a, SpRange16 *b, const char *path, uint16_t plen);

SP_EXPORT int
sp_path_join (char *out, size_t len,
		const char *a, uint16_t alen,
		const char *b, uint16_t blen,
		SpPathMode mode);

SP_EXPORT uint16_t
sp_path_clean (char *path, uint16_t len, SpPathMode mode);

SP_EXPORT bool
sp_path_match (const char *path, const char *match);

SP_EXPORT int
sp_path_proc (char *buf, size_t buflen);

SP_EXPORT int
sp_path_env (const char *name, char *buf, size_t buflen);



SP_EXPORT int
sp_dir_open (SpDir *self, const char *path, uint8_t depth);

SP_EXPORT void
sp_dir_close (SpDir *self);

SP_EXPORT int
sp_dir_next (SpDir *self);

SP_EXPORT void
sp_dir_skip (SpDir *self);

SP_EXPORT int
sp_dir_follow (SpDir *self);

SP_EXPORT SpPathType
sp_dir_type (SpDir *self);

SP_EXPORT const SpStat *
sp_dir_stat (SpDir *self);

SP_EXPORT void
sp_dir_path (const SpDir *self, const char **start, size_t *len);

SP_EXPORT void
sp_dir_dirname (const SpDir *self, const char **start, size_t *len);

SP_EXPORT void
sp_dir_basename (const SpDir *self, const char **start, size_t *len);



SP_EXPORT int
sp_stat (const char *path, SpStat *sbuf, bool follow);

#endif

