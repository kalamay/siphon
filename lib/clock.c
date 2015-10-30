#include "../include/siphon/clock.h"
#include "config.h"

#include <errno.h>
#include <assert.h>

#if SP_MACH_CLOCK

static clock_serv_t clock_serv;
static mach_timebase_info_data_t info;

static void __attribute__((constructor))
init (void)
{
	if (host_get_clock_service (
				mach_host_self (),
				CALENDAR_CLOCK,
				&clock_serv) != KERN_SUCCESS) {
		fprintf (stderr, "failed to get clock service\n");
	}
	(void)mach_timebase_info (&info);
}

static void __attribute__((destructor))
fini (void)
{
	mach_port_deallocate (mach_task_self (), clock_serv);
}

#endif

int
sp_clock_real (SpClock *clock)
{
	assert (clock != NULL);

#if SP_POSIX_CLOCK
	if (clock_gettime (SP_CLOCK_REALIME, &clock) < 0) {
		return -errno;
	}
#elif SP_MACH_CLOCK
	mach_timespec_t ts;
	if (clock_get_time (clock_serv, &ts) != KERN_SUCCESS) {
		return -errno;
	}
	clock->tv_sec = ts.tv_sec;
	clock->tv_nsec = ts.tv_nsec;
#endif
	return 0;
}

int
sp_clock_mono (SpClock *clock)
{
	assert (clock != NULL);

#if SP_POSIX_CLOCK
	if (clock_gettime (SP_CLOCK_MONOTONIC, &clock) < 0) {
		return -errno;
	}
#elif SP_MACH_CLOCK
	SP_CLOCK_SET_NSEC (clock, (mach_absolute_time () * info.numer) / info.denom);
#endif
	return 0;
}

double
sp_clock_diff (SpClock *clock)
{
	assert (clock != NULL);

	SpClock now;
	if (sp_clock_mono (&now) < 0) {
		return NAN;
	}
	SP_CLOCK_SUB (&now, clock);
	return SP_CLOCK_TIME (&now);
}

double
sp_clock_step (SpClock *clock)
{
	assert (clock != NULL);

	double diff = sp_clock_diff (clock);
	if (isnan (diff) || sp_clock_mono (clock) < 0) {
		return NAN;
	}
	return diff;
}

void
sp_clock_print (SpClock *clock, FILE *out)
{
	if (out == NULL) {
		out = stderr;
	}

	if (clock == NULL) {
		fprintf (out, "#<SpClock:(null)>\n");
	}
	else {
		fprintf (out, "#<SpClock:%p time=%f>\n", (void *)clock, SP_CLOCK_TIME (clock));
	}
}

