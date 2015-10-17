#if defined(SP_NO_THREADS)

# define LOCK()
# define UNLOCK()

#elif defined(SP_PTHREAD)

# include <pthread.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
# define LOCK() pthread_mutex_lock (&lock)
# define UNLOCK() pthread_mutex_unlock (&lock)

#elif defined(__GCC_ATOMIC_TEST_AND_SET_TRUEVAL)

static volatile int lock = 0;
# define LOCK() do {} while (__sync_lock_test_and_set (&lock, 1))
# define UNLOCK() __sync_lock_release(&lock)

#elif defined(_MSC_VER)

static volatile long lock = 0;
# define LOCK() do {} while (InterlockedExchange (&lock, 1))
# define UNLOCK() do { _ReadWriteBarrier (); lock = 0; } while (0)

#endif

