#ifndef SIPHON_CLOCK_H
#define SIPHON_CLOCK_H

#include "common.h"

#include <stdio.h>
#include <time.h>

typedef struct timespec SpClock;

#define SP_NSEC_PER_USEC 1000LL
#define SP_NSEC_PER_MSEC 1000000LL
#define SP_NSEC_PER_SEC  1000000000LL
#define SP_USEC_PER_MSEC 1000LL
#define SP_USEC_PER_SEC  1000000LL
#define SP_MSEC_PER_SEC  1000LL

#define SP_NSEC_TO_USEC(nsec) ((nsec) / SP_NSEC_PER_USEC)
#define SP_NSEC_TO_MSEC(nsec) ((nsec) / SP_NSEC_PER_MSEC)
#define SP_NSEC_TO_SEC(nsec)  ((nsec) / SP_NSEC_PER_SEC)
#define SP_USEC_TO_NSEC(usec) ((usec) * SP_NSEC_PER_USEC)
#define SP_USEC_TO_MSEC(usec) ((usec) / SP_USEC_PER_MSEC)
#define SP_USEC_TO_SEC(usec)  ((usec) / SP_USEC_PER_SEC)
#define SP_MSEC_TO_NSEC(usec) ((usec) * SP_NSEC_PER_MSEC)
#define SP_MSEC_TO_USEC(usec) ((usec) * SP_USEC_PER_MSEC)
#define SP_MSEC_TO_SEC(usec)  ((usec) / SP_MSEC_PER_SEC)
#define SP_SEC_TO_NSEC(sec)   ((sec)  * SP_NSEC_PER_SEC)
#define SP_SEC_TO_USEC(sec)   ((sec)  * SP_USEC_PER_SEC)
#define SP_SEC_TO_MSEC(sec)   ((sec)  * SP_MSEC_PER_SEC)

#define SP_NSEC_TO_NSEC(nsec)  (nsec)
#define SP_SEC_TO_SEC(sec)     (sec)

#define SP_NSEC_REM(nsec) (((nsec) % SP_NSEC_PER_SEC))
#define SP_USEC_REM(usec) (((usec) % SP_USEC_PER_SEC) * SP_NSEC_PER_USEC)
#define SP_MSEC_REM(msec) (((msec) % SP_MSEC_PER_SEC) * SP_NSEC_PER_MSEC)
#define SP_SEC_REM(sec)   0


#define SP_CLOCK_MAKE(val, n) \
    ((SpClock){ SP_##n##_TO_SEC (val), SP_##n##_REM (val) })

#define SP_CLOCK_MAKE_NSEC(nsec) SP_CLOCK_MAKE(nsec, NSEC)
#define SP_CLOCK_MAKE_USEC(usec) SP_CLOCK_MAKE(usec, USEC)
#define SP_CLOCK_MAKE_MSEC(msec) SP_CLOCK_MAKE(msec, MSEC)
#define SP_CLOCK_MAKE_SEC(sec)   SP_CLOCK_MAKE(sec,  SEC)


#define SP_CLOCK_GET(c, n) \
    ((int64_t)(SP_NSEC_TO_##n ((c)->tv_nsec) + SP_SEC_TO_##n ((c)->tv_sec)))

#define SP_CLOCK_NSEC(c) SP_CLOCK_GET(c, NSEC)
#define SP_CLOCK_USEC(c) SP_CLOCK_GET(c, USEC)
#define SP_CLOCK_MSEC(c) SP_CLOCK_GET(c, MSEC)
#define SP_CLOCK_SEC(c)  SP_CLOCK_GET(c, SEC)


#define SP_CLOCK_SET(c, val, n) {        \
    (c)->tv_sec = SP_##n##_TO_SEC (val); \
    (c)->tv_nsec = SP_##n##_REM (val);   \
} while (0)

#define SP_CLOCK_SET_NSEC(c, nsec) SP_CLOCK_SET(c, nsec, NSEC)
#define SP_CLOCK_SET_USEC(c, usec) SP_CLOCK_SET(c, usec, USEC)
#define SP_CLOCK_SET_MSEC(c, msec) SP_CLOCK_SET(c, msec, MSEC)
#define SP_CLOCK_SET_SEC(c, sec)   SP_CLOCK_SET(c, sec,  SEC)


#define SP_CLOCK_TIME(c) \
    ((double)(c)->tv_sec + 1e-9 * (c)->tv_nsec)
    
#define SP_CLOCK_SET_TIME(c, time) do {                           \
    double sec;                                                   \
    (c)->tv_nsec = round (modf ((time), &sec) * SP_NSEC_PER_SEC); \
    (c)->tv_sec = sec;                                            \
} while (0)


#define SP_CLOCK_ABS(c, rel, n) \
    ((int64_t)(rel)+(SP_SEC_TO_##n((c)->tv_sec)+SP_NSEC_TO_##n((c)->tv_nsec)))

#define SP_CLOCK_ABS_NSEC(c, rel) SP_CLOCK_ABS(c, rel, NSEC)
#define SP_CLOCK_ABS_USEC(c, rel) SP_CLOCK_ABS(c, rel, USEC)
#define SP_CLOCK_ABS_MSEC(c, rel) SP_CLOCK_ABS(c, rel, MSEC)
#define SP_CLOCK_ABS_SEC(c, rel)  SP_CLOCK_ABS(c, rel, SEC)


#define SP_CLOCK_REL(c, abs, n) \
    ((int64_t)(abs)-(SP_SEC_TO_##n((c)->tv_sec)+SP_NSEC_TO_##n((c)->tv_nsec)))

#define SP_CLOCK_REL_NSEC(c, rel) SP_CLOCK_REL(c, rel, NSEC)
#define SP_CLOCK_REL_USEC(c, rel) SP_CLOCK_REL(c, rel, USEC)
#define SP_CLOCK_REL_MSEC(c, rel) SP_CLOCK_REL(c, rel, MSEC)
#define SP_CLOCK_REL_SEC(c, rel)  SP_CLOCK_REL(c, rel, SEC)


#define SP_CLOCK_ADD(c, v) do {            \
    (c)->tv_sec += (v)->tv_sec;            \
    (c)->tv_nsec += (v)->tv_nsec;          \
    if ((c)->tv_nsec >= SP_NSEC_PER_SEC) { \
        (c)->tv_sec++;                     \
        (c)->tv_nsec -= SP_NSEC_PER_SEC;   \
    }                                      \
} while (0)

#define SP_CLOCK_SUB(c, v) do {            \
    (c)->tv_sec -= (v)->tv_sec;            \
    (c)->tv_nsec -= (v)->tv_nsec;          \
    if ((c)->tv_nsec < 0LL) {              \
        (c)->tv_sec--;                     \
        (c)->tv_nsec += SP_NSEC_PER_SEC;   \
    }                                      \
} while (0)

#define SP_CLOCK_CMP(c, v, op) (       \
    (c)->tv_sec == (v)->tv_sec         \
        ? (c)->tv_nsec op (v)->tv_nsec \
        : (c)->tv_sec op (v)->tv_sec)


SP_EXPORT int
sp_clock_real (SpClock *clock);

SP_EXPORT int
sp_clock_mono (SpClock *clock);

SP_EXPORT double
sp_clock_diff (SpClock *clock);

SP_EXPORT double
sp_clock_step (SpClock *clock);

SP_EXPORT void
sp_clock_print (SpClock *clock, FILE *out);

#endif

