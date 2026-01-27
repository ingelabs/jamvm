/*
 * Copyright (C) 2008 Robert Lougher <rob@jamvm.org.uk>.
 *
 * This file is part of JamVM.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <sys/time.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>

#include "jam.h"

/* Whether the monotonic clock is available. As the clock_id may be
   present but not actually supported, we must probe at runtime. */
static int have_monotonic_clock = 0;

/* Monotonic timed waits require the pthread_condattr_setclock() function,
   and support for the monotonic clock at runtime. */
static int have_monotonic_timedwait = 0;

/* Attributes for condvars used for relative timed waits, if monotonic
   timed waits are available. */
static pthread_condattr_t condattr;

int haveMonotonicClock() {
    return have_monotonic_clock;
}

int haveMonotonicTimedWait() {
    return have_monotonic_timedwait;
}

pthread_condattr_t *getRelativeWaitCondAttr() {
    return have_monotonic_timedwait ? &condattr : NULL;
}

int initialiseTime() {
#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
    struct timespec ts;
    have_monotonic_clock = (clock_gettime(CLOCK_MONOTONIC, &ts) != -1);

#if defined(HAVE_PTHREAD_CONDATTR_SETCLOCK)
    if(have_monotonic_clock) {
        pthread_condattr_init(&condattr);
        if (pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC) == 0)
            have_monotonic_timedwait = 1;
    }
#endif
#endif
    if(!have_monotonic_timedwait) {
        jam_fprintf(stderr, "Monotonic clock not available. Changes to " \
                            "the current date/time may affect scheduling.\n");
    }

    return 1;
}

void getTimeoutAbsolute(struct timespec *ts, long long millis,
                        long long nanos) {

    /* Calculate seconds (long long prevents overflow) */
    long long seconds = millis / 1000 + nanos / 1000000000;

    /* Calculate nanoseconds */
    nanos %= 1000000000;
    nanos += (millis % 1000) * 1000000;

    /* Adjust values so that nanos is less than 1 second */
    if(nanos > 999999999) {
        seconds++;
        nanos -= 1000000000;
    }

    /* If seconds is too big to fit into the timespec use the
       maximum value (year 2038) */
    ts->tv_sec = seconds > LONG_MAX ? LONG_MAX : seconds;
    ts->tv_nsec = nanos;
}

void getTimeoutRelative(struct timespec *ts, long long millis,
                        long long nanos) {
    /* long long prevents overflow */
    long long seconds;

#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
    if(haveMonotonicTimedWait()) {
        struct timespec tp;

        /* Get the current time */
        clock_gettime(CLOCK_MONOTONIC, &tp);

        /* Calculate seconds */
        seconds = tp.tv_sec + millis / 1000 + nanos / 1000000000;

        /* Calculate nanoseconds */
        nanos %= 1000000000;
        nanos += tp.tv_nsec + ((millis % 1000) * 1000000);
    } else
#endif
    {
        struct timeval tv;

        /* Get the current time */
        gettimeofday(&tv, NULL);

        /* Calculate seconds */
        seconds = tv.tv_sec + millis / 1000 + nanos / 1000000000;

        /* Calculate nanoseconds */
        nanos %= 1000000000;
        nanos += (tv.tv_usec + ((millis % 1000) * 1000)) * 1000;
    }

    /* Adjust values so that nanos is less than 1 second.
       This also prevents overflowing the timespec, as the
       value may be larger than tv_nsec (signed int) */
    seconds += nanos / 1000000000;
    nanos %= 1000000000;

    /* If seconds is too big to fit into the timespec use the
       maximum value (year 2038) */
    ts->tv_sec = seconds > LONG_MAX ? LONG_MAX : seconds;
    ts->tv_nsec = nanos;
}
