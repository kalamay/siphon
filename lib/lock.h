#include <unistd.h>

// based on: http://locklessinc.com/articles/locks/

#define SP_PAUSE() __asm__ __volatile__("pause\n": : :"memory")
#define SP_BARRIER() __asm__ __volatile__("": : :"memory")

typedef volatile int SpLock;
#define SP_LOCK_MAKE() 0
#define SP_LOCK(l) do {                       \
	while (__sync_lock_test_and_set(&l, 1)) { \
		while (l) { SP_PAUSE (); }            \
	}                                         \
} while (0)
#define SP_UNLOCK(l) __sync_lock_release(&l)

typedef union {
	uint64_t u;
	uint32_t us;
	struct {
		uint16_t writers;
		uint16_t readers;
		uint16_t users;
		uint16_t pad;
	} s;
} SpRWLock;

#define SP_RWLOCK_MAKE() { .u = 0 }

#define SP_WLOCK(l) do {                                          \
	uint64_t me = __sync_fetch_and_add (&l.u, (uint64_t)1 << 32); \
	uint16_t val = (uint16_t)(me >> 32);                          \
	while (val != l.s.writers) { SP_PAUSE (); }                   \
} while (0)

#define SP_WUNLOCK(l) do { \
	SpRWLock t = l;        \
	SP_BARRIER ();         \
	t.s.writers++;         \
	t.s.readers++;         \
	l.us = t.us;           \
} while (0)

#define SP_RLOCK(l) do {                                          \
	uint64_t me = __sync_fetch_and_add (&l.u, (uint64_t)1 << 32); \
	uint16_t val = (uint16_t)(me >> 32);                          \
	for (int n = 0; val != l.s.readers; n++) {                    \
		if (n < 1000) { SP_PAUSE (); }                            \
		else { usleep (10); }                                     \
	}                                                             \
	++l.s.readers;                                                \
} while (0)

#define SP_RUNLOCK(l) do {                  \
	__sync_add_and_fetch (&l.s.writers, 1); \
} while (0)

