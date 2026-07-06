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

#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC) && \
    defined(HAVE_PTHREAD_CONDATTR_SETCLOCK)
#define MONOTONIC_CONDWAIT
#endif

/* We use the monotonic clock if it is available.  As the clock_id may be
   present but not actually supported, we check it on startup */
static int have_monotonic_clock = FALSE;

/* For relative timeouts, additionally check that condvars can be bound
   to the monotonic clock */
static int have_monotonic_condwait = FALSE;

#ifdef MONOTONIC_CONDWAIT
static pthread_condattr_t monotonic_condattr;
#endif

int initialiseTime() {
#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
    struct timespec ts;
    have_monotonic_clock = (clock_gettime(CLOCK_MONOTONIC, &ts) != -1);

#ifdef MONOTONIC_CONDWAIT
    if(have_monotonic_clock &&
              pthread_condattr_init(&monotonic_condattr) == 0) {
        if(pthread_condattr_setclock(&monotonic_condattr,
                                     CLOCK_MONOTONIC) == 0)
            have_monotonic_condwait = TRUE;
        else
            pthread_condattr_destroy(&monotonic_condattr);
    }
#endif
#endif

    if(!have_monotonic_condwait)
        jam_fprintf(stderr, "Monotonic clock not available. Changes to "
                            "the current date/time may affect scheduling.\n");

    return TRUE;
}

int haveMonotonicClock() {
    return have_monotonic_clock;
}

/* Initialise a condition variable to be used for relative timed waits */
int initReltimeCondVar(pthread_cond_t *cv) {
#ifdef MONOTONIC_CONDWAIT
    if(have_monotonic_condwait)
        return pthread_cond_init(cv, &monotonic_condattr);
#endif
    return pthread_cond_init(cv, NULL);
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
    long long seconds;

#ifdef MONOTONIC_CONDWAIT
    if(have_monotonic_condwait) {
        struct timespec now;

        /* Get the current time */
        clock_gettime(CLOCK_MONOTONIC, &now);

        /* Calculate seconds (long long prevents overflow) */
        seconds = now.tv_sec + millis / 1000 + nanos / 1000000000;

        /* Calculate nanoseconds */
        nanos %= 1000000000;
        nanos += now.tv_nsec + (millis % 1000) * 1000000;
    } else
#endif
    {
        struct timeval tv;

        /* Get the current time */
        gettimeofday(&tv, NULL);

        /* Calculate seconds (long long prevents overflow) */
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
       maximum value (for 32-bit time_t and realtime clock,
       year 2038) */
    ts->tv_sec = seconds > LONG_MAX ? LONG_MAX : seconds;
    ts->tv_nsec = nanos;
}

