#include <stdio.h>
#include <stdlib.h>
#include "cryo.h"

/* Included from packet.c.
 * 
 * Listens on a UDP socket (contained within a CryoStreamer struct and
 * built by new_CryoStreamer) for a packet of sensor data
 * as it comes off the cryoboard.
 * 
 * NOTE: Uses ADC units
 * 
 * */	 

int CryoStreamer_get_samples(CryoStreamer *self, CryoSamples * results, int flush) {
	
	/* Found in timestamp.c. These were global in cryo, but libcryo
	 * should only need them here
	 */

	const uint32_t STREAMER_IRIG = 1;
	const uint32_t STREAMER_IRIG_TEST = 2;
	const uint32_t STREAMER_TIMESYNC = 3;
	
	packet_v2 *pv2 = NULL;
	/*double ts_unix;*/

	char _buf[4096];

	void *buf = _buf;
	int i;

	/* Flush results before grabbing anything, if desired */
	while(flush && recv(self->sockfd, buf, 4096, MSG_DONTWAIT) >= 0)
		;

	/* Retrieve samples */
	i = recv(self->sockfd, buf, 4096, 0);

	if(PACKET_MAGIC(buf) != MAGIC || PACKET_VERSION(buf) != 2) {
		fprintf(stderr,"Packet didn't have correct magic!");
		goto error;
	}

	pv2 = (struct packet_v2 *)buf;

	if(pv2 && sizeof(*pv2) != i) {
		fprintf(stderr,"Unexpected packet size! (%i != %lu)", i, sizeof(*pv2));
		goto error;
	}
		
	for(i=0; i<NUM_SENSOR_CHANNELS; i++) {

		/* The ADC is 24 bits; the samples are packed into
		 * 32-bit integers. Renormalize. I kinda wish this
		 * shift-unshift wasn't present, but it is, so we're
		 * a bit stuck with it. */

		results->_vi[i] = (int32_t)ntohl(pv2->vi[i]) >> 8;
		results->_vq[i] = (int32_t)ntohl(pv2->vq[i]) >> 8;
		results->_ii[i] = (int32_t)ntohl(pv2->ii[i]) >> 8;
		results->_iq[i] = (int32_t)ntohl(pv2->iq[i]) >> 8;

		/* Add timestamps */
		if(pv2) {
			if(ntohl(pv2->ts_port) == STREAMER_IRIG) {
				results->_ts.port = IRIG;

				results->_ts.ts.irig.y = ntohl(pv2->ts.irig.y);
				results->_ts.ts.irig.d = ntohl(pv2->ts.irig.d);
				results->_ts.ts.irig.h = ntohl(pv2->ts.irig.h);
				results->_ts.ts.irig.m = ntohl(pv2->ts.irig.m);
				results->_ts.ts.irig.s = ntohl(pv2->ts.irig.s);
				results->_ts.ts.irig.ss = ntohl(pv2->ts.irig.ss);

			} else if(ntohl(pv2->ts_port) == STREAMER_IRIG_TEST) {
				results->_ts.port = IRIG_TEST;

				results->_ts.ts.irig_test.s = ntohl(pv2->ts.irig_test.s);
				results->_ts.ts.irig_test.ss = ntohl(pv2->ts.irig_test.ss);

			} else if(ntohl(pv2->ts_port) == STREAMER_TIMESYNC) {
				results->_ts.port = TIMESYNC;

				results->_ts.ts.timesync.source = ntohl(pv2->ts.timesync.source);
				results->_ts.ts.timesync.maj = ntohl(pv2->ts.timesync.maj);
				results->_ts.ts.timesync.ticks = ntohl(pv2->ts.timesync.ticks);
				results->_ts.ts.timesync.recent = ntohl(pv2->ts.timesync.recent);

			} else {
				fprintf(stderr,"Unexpected timestamp type! %i",
						ntohl(pv2->ts_port));
				goto error;
			}
		}
	}

	return 1;
	
error:

	return 0;
}

CryoStreamer *new_CryoStreamer(char *hostname, char *port) {
	CryoStreamer *self = NULL;
	struct addrinfo hints;
	struct ip_mreq mreq;
	int fd = -1;
	int is_multicast;
	const int one = 1;

	if(!(self = calloc(1, sizeof(*self)))) {
		fprintf(stderr,"Out of memory!");
		return(NULL);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_PASSIVE;

	if(getaddrinfo(hostname, port, &hints, &self->addr)) {
		fprintf(stderr,"Oops! getaddrinfo() failed");
		goto error;
	}

	/* Create and tweak socket */
	if((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		fprintf(stderr,"Failed to create socket!");
		goto error;
	}	

	is_multicast = IN_MULTICAST(ntohl(((struct sockaddr_in *)self->addr->ai_addr)->sin_addr.s_addr));

	/* Permit multiple bindings to the same multicast addr/port. */
	if(is_multicast)
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	if(bind(fd, self->addr->ai_addr, self->addr->ai_addrlen) == -1) {
		fprintf(stderr,"Failed to bind() socket");
		goto error;
	}
	

	if(is_multicast) {
		memset(&mreq, 0, sizeof(mreq));

		memcpy(&mreq.imr_multiaddr.s_addr,
				&((struct sockaddr_in *)self->addr->ai_addr)->sin_addr.s_addr,
				sizeof(struct in_addr));
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		if(setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1) {
			fprintf(stderr,"Multicast: couldn't add membership.");
			goto error;
		}
	}
	

	self->sockfd = fd;


	return(self);
error:
	if(fd != -1)
		close(fd);

	if(self)
		free(self);

	perror("\n\nnew_CryoStreamer()");

	return(NULL);
}

void delete_CryoStreamer(CryoStreamer *self) {
	assert(self);

	freeaddrinfo(self->addr);
	close(self->sockfd);

	memset(self, 0, sizeof(*self));
	free(self);
}




/* Board control functions */

/* Everything here uses cryoboard_request to send requests to the cryoboard,
 * and checky_reply to read replies. All the functions communicate using the
 * response_info struct argument */



size_t check_reply(char * contents, size_t size, size_t nmenb, void * userdata) {
	
	int i;
	
	json_t * reply, * json;
	json_error_t error;
	
	response_info * info ;
	
	info = (response_info *) userdata;
	
	reply = json_loads(contents, 0, &error);
	
	if(!reply) {
		fprintf(stderr, "%s failed: reply not valid json:\n%s", info->caller_name, error.text );
		return 0;
	}
	
	json = json_array_get(reply,0);
	json = json_object_get(json, "error");
	
	if(!json) {
		fprintf(stderr, "%s failed: corrupt reply", info->caller_name);
		return 0;
	}
	
	if(!json_is_null(json)) {
		
		fprintf(stderr, "%s failed: cryoboard reports failure", info->caller_name);
		return 0;
	}
	
	if( strcmp(info->caller_name, "dump_state" ) == 0 ) {

		json = json_array_get(reply,0);
		json = json_object_get(json, "result");
		json = json_object_get(json, "sensor_channels");
		
		if (!json_is_null(json)) {
		
			for(i = 0; i < NUM_SENSOR_CHANNELS; i++) {
				
				info->r->sense[i] = json_real_value( 
										json_object_get( 
											json_array_get(json, i),
											"sense_resistance"
										)
									);
											
				info->r->drive[i] = json_real_value( 
										json_object_get( 
											json_array_get(json, i),
											"drive_resistance"
										)
									);
				
			} 
			
		} else { return 0; }
	
	}	
	
	json_decref(reply);
	
	return 1;
	
}

/* Use libcurl to dispatch a request to a cryoboard, and register
 * check_reply as the callback that extracts information from
 * the reply and places it in the info struct 
 * */
int cryoboard_request(json_t * req, char * board_url, response_info * info) {
	
	CURL * curl;
	CURLcode res;
	
	curl_global_init(CURL_GLOBAL_ALL);
	
	curl = curl_easy_init();
	
	if(curl) {
		
		curl_easy_setopt(curl, CURLOPT_URL, board_url);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, check_reply);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) info);
		
		res = curl_easy_perform(curl);
		
		if(res != CURLE_OK) {
			fprintf(stderr, "%s failed: %s\n",info->caller_name, curl_easy_strerror(res));
			return 0;
		}
		
		curl_easy_cleanup(curl);
		json_decref(req);
		
	}
	
	else {
		
		fprintf(stderr, "%s failed: unable initialize libcurl\n", info->caller_name);
		return 0;
	}
	
	curl_global_cleanup();
	json_decref(req);
	
	return 1;
	
}

int kill_streamer(char * hostname, char * port, char * board_url) {

	char name[] = "kill_streamer";

	json_t * req;
	
	response_info info;
	
	strncpy(info.caller_name, name,INFO_CALLER_NAME_LEN);
	
	req = json_pack("{s:s,s:s,s:[s,s]}",
		"object",
		"cryoboard",
		"method",
		"kill_streamer",
		"args",
		hostname,
		port);
		
	return cryoboard_request(req, board_url, &info);
	
}

int launch_streamer(char * hostname, char * port, char * board_url) {

	char name[] = "launch_streamer";
	
	response_info info;

	json_t * req;
	
	fprintf(stderr,"asdf\n");
	
	strncpy(info.caller_name, name,INFO_CALLER_NAME_LEN);
	
	fprintf(stderr,"asdf\n");
	
	req = json_pack("{s:s,s:s,s:[s,s]}",
		"object",
		"cryoboard",
		"method",
		"launch_streamer",
		"args",
		hostname,
		port);

	return cryoboard_request(req, board_url, &info);
	
}

int get_drive_sense_resistance(char * board_url, resistance * r) {

	char name[] = "dump_state";
	response_info info;
	int ret;
	
	strncpy(info.caller_name, name,INFO_CALLER_NAME_LEN); 
	info.r = r;

	CURL * curl;
	CURLcode * res;
	
	json_t * req;
	
	req = json_pack("{s:s,s:s,s:[s]}",
		"object",
		"cryoboard",
		"method",
		name,
		"args",
		"null");
		
	return cryoboard_request(req, board_url, &info);
}


