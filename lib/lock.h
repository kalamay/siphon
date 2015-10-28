#if defined(SP_NO_THREADS)

# define LOCK()
# define UNLOCK()

#elif defined(SP_PTHREAD)

# include <pthread.h>

typedef pthread_mutex_t SpLock;
# define SP_LOCK_MAKE() PTHREAD_MUTEX_INITIALIZER
# define SP_LOCK(l) pthread_mutex_lock (&l)
# define SP_UNLOCK(l) pthread_mutex_unlock (&l)

#elif defined(__GCC_ATOMIC_TEST_AND_SET_TRUEVAL)

typedef volatile int SpLock;
# define SP_LOCK_MAKE() 0
# define SP_LOCK(l) do {} while (__sync_lock_test_and_set (&l, 1))
# define SP_UNLOCK(l) __sync_lock_release(&l)

#elif defined(_MSC_VER)

typedef volatile long SpLock;
# define SP_LOCK_MAKE() 0
# define SP_LOCK(l) do {} while (InterlockedExchange (&l, 1))
# define SP_UNLOCK(l) do { _ReadWriteBarrier (); l = 0; } while (0)

#endif

