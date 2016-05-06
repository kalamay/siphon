// based on: http://locklessinc.com/articles/locks/
#include <unistd.h>

#define SP_PAUSE() __asm__ __volatile__("pause\n": : :"memory")
#define SP_BARRIER() __asm__ __volatile__("": : :"memory")
#define SP_CMPXCHG(P, O, N) __sync_bool_compare_and_swap((P), (O), (N))
#define SP_ATOMIC_FETCH_ADD(P, V) __sync_fetch_and_add((P), (V))
#define SP_ATOMIC_ADD_FETCH(P, V) __sync_add_and_fetch((P), (V))
#define SP_WAIT(cond) do {              \
	for (unsigned n = 0; (cond); n++) { \
		if (n < 1000) { SP_PAUSE (); }  \
		else { usleep (10); }           \
	}                                   \
} while (0)

typedef union {
	uint32_t u;
	struct {
		uint16_t ticket;
		uint16_t users;
	} s;
} SpLock;

#define SP_LOCK_MAKE() { .u = 0 }

#define SP_LOCK(l) do {                                \
	uint16_t me = SP_ATOMIC_FETCH_ADD (&l.s.users, 1); \
	SP_WAIT (l.s.ticket != me);                        \
} while (0)

#define SP_TRY_LOCK(l) __extension__ ({             \
	uint16_t me = l.s.users;                        \
	uint16_t menew = me + 1;                        \
	uint32_t cmp = ((uint32_t)me << 16) + me;       \
	uint32_t cmpnew = ((uint32_t)menew << 16) + me; \
	SP_CMPXCHG (&l.u, cmp, cmpnew);                 \
})

#define SP_UNLOCK(l) do { \
	SP_BARRIER ();        \
	l.s.ticket++;         \
} while (0)

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

#define SP_WLOCK(l) do {                                         \
	uint64_t me = SP_ATOMIC_FETCH_ADD (&l.u, (uint64_t)1 << 32); \
	uint16_t val = (uint16_t)(me >> 32);                         \
	SP_WAIT (val != l.s.writers);                                \
} while (0)

#define SP_TRY_WLOCK(l) __extension__ ({                      \
	uint64_t me = l.s.users;                                  \
	uint16_t menew = me + 1;                                  \
	uint64_t readers = l.s.readers << 16;                     \
	uint64_t cmp = (me << 32) + readers + me;                 \
	uint64_t cmpnew = ((uint64_t)menew << 32) + readers + me; \
	SP_CMPXCHG (&l.u, cmp, cmpnew);                           \
})

#define SP_WUNLOCK(l) do { \
	SpRWLock t = l;        \
	SP_BARRIER ();         \
	t.s.writers++;         \
	t.s.readers++;         \
	l.us = t.us;           \
} while (0)

#define SP_RLOCK(l) do {                                         \
	uint64_t me = SP_ATOMIC_FETCH_ADD (&l.u, (uint64_t)1 << 32); \
	uint16_t val = (uint16_t)(me >> 32);                         \
	SP_WAIT (val != l.s.readers);                                \
	++l.s.readers;                                               \
} while (0)

#define SP_TRY_RLOCK(l) __extension__ ({                                  \
	uint64_t me = l.s.users;                                              \
	uint64_t writers = l.s.writers;                                       \
	uint16_t menew = me + 1;                                              \
	uint64_t cmp = (me << 32) + (me << 16) + writers;                     \
	uint64_t cmpnew = ((uint64_t) menew << 32) + (menew << 16) + writers; \
	SP_CMPXCHG (&l.u, cmp, cmpnew);                                       \
})

#define SP_RUNLOCK(l) do {                 \
	SP_ATOMIC_ADD_FETCH (&l.s.writers, 1); \
} while (0)

