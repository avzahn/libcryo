#ifndef __TIMESTAMP_H__
#define __TIMESTAMP_H__

typedef struct ts_irig_test {
	uint32_t s;
	uint32_t ss;
} ts_irig_test;

typedef struct ts_irig {
	uint32_t y,d,h,m,s;
	uint32_t ss;
} ts_irig;

typedef struct ts_timesync {
	uint32_t source;
	uint32_t maj;
	uint32_t ticks;
	uint32_t recent;
} ts_timesync;

/* foudn this one in units.h */
typedef const char *timestamp_port_t;

/*
 * Timestamp structure. This is directly %extended (for Python) and included
 * in streamer packets.
 */
typedef struct Timestamp {

	/* This could be an union, but it's a fairly irrelevant balancing act
	 * between clumsy syntax (anonymous unions aren't portable or
	 * supported) and wasted space (we don't need three different structs
	 * when only one is used at a time.)
	 */

	timestamp_port_t port;

	union {
		ts_irig_test irig_test;
		ts_irig irig;
		ts_timesync timesync;
	} ts;

} Timestamp;

#endif

