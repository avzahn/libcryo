#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#ifndef _ISOC99_SOURCE
# define _ISOC99_SOURCE
#endif

/* Set of includes from packet.c */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <curl/curl.h>
#include <jansson.h>


/* Define the Timestamp struct used by the CryoStreamer and packet_v2
 * structs
 */
#include "timestamp.h"

/* Included from cryoboard.h.
 * 
 * Used by CryoStreamer_get_samples to check packet version do a basic
 * parity check
 * */

#define MAGIC 0x4352594f
#define PACKET_VERSION(p) ntohl(((uint32_t *)(p))[1])
#define PACKET_MAGIC(p) ntohl(((uint32_t *)(p))[0])

/* Also from cryoboard.h */
#define NUM_SENSOR_CHANNELS 16

/* From units.h */
#define IRIG		"IRIG"
#define IRIG_TEST	"IRIG test"
#define TIMESYNC	"Timesync"

#define INFO_CALLER_NAME_LEN 128


/* Included from cryoboard.h.
 * 
 * Struct used by functions included from packet.c to manage client side
 * UDP socket for recieving sensor data.
 * */

typedef struct CryoStreamer {
	struct addrinfo *addr;
	int sockfd;
} CryoStreamer;

typedef struct CryoSamples {
	int32_t _vi[NUM_SENSOR_CHANNELS];
	int32_t _vq[NUM_SENSOR_CHANNELS];
	int32_t _ii[NUM_SENSOR_CHANNELS];
	int32_t _iq[NUM_SENSOR_CHANNELS];

	Timestamp _ts;
} CryoSamples;

/* Included from cryoboard.h.
 * 
 * Defines the packet format used by the cryoboard to send out sensor
 * data. Sensor data will be exactly this struct wrapped in a UDP packet.
 * 
 * NOTE: Uses ADC units
 * 
 * */

typedef struct packet_v2 {
	uint32_t magic;
	uint32_t version;

	uint32_t reserved[3]; /* future use */
	int32_t vi[NUM_SENSOR_CHANNELS];
	int32_t vq[NUM_SENSOR_CHANNELS];
	int32_t ii[NUM_SENSOR_CHANNELS];
	int32_t iq[NUM_SENSOR_CHANNELS];

	uint32_t ts_port;
	union {
		ts_irig_test irig_test;
		ts_irig irig;
		ts_timesync timesync;
	} ts;

} __attribute__((packed)) packet_v2;

/* Pure client side setup taken from packet.c for listening for sensor
 * data from a cryboard.
 * 
 * These don't communicate with the cryoboard in any way, and deal
 * entirely in ADC units. All these do is setup a UDP socket, listen in
 * on it, and then tear it down.
 * */



/* get_samples has been modified to return ADC samples through an
 * argument instead of a return value. This eliminates the need to
 * malloc on every call and free the returned CryoSamples struct
 * repeatedly */
int CryoStreamer_get_samples(CryoStreamer *self, CryoSamples * result, int flush);
CryoStreamer *new_CryoStreamer(char *hostname, char *port);
void delete_CryoStreamer(CryoStreamer *self);

/* libcurl + libjansson based cryoboard communication functions.
 * 
 * libcryo is not responsible for board management or control, but does
 * need to tell the cryoboard to broadcast sensor data to the client IP.
 * Also, we need the sense and drive resistance for all the channels in
 * order to convert the ADC units the cryoboards broadcasts into physical
 * units.
 * */

typedef struct resistance {
	
	double sense[NUM_SENSOR_CHANNELS];
	double drive[NUM_SENSOR_CHANNELS];
	
} resistance;

typedef struct response_info {
	
	char caller_name[INFO_CALLER_NAME_LEN];
	resistance * r;
	
} response_info;

int get_drive_sense_resistance(char * board_url, resistance * r);
int cryoboard_request(json_t * req, char * board_url, response_info * info);
int launch_streamer(char * hostname, char * port, char * board_url);
int kill_streamer(char * hostname, char * port, char * board_url);
