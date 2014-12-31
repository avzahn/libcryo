#include "listener.h"
#include "string.h"

CryoListener * new_CryoListener(
						char * hostname,
						char * port,
						char * board_url,
						char * calibration_dir) {	
	
	CryoListener * self = (CryoListener *) malloc ( sizeof(CryoListener) );
	
	/* setup the interpolators */
	
	int i;
	FILE * f;
	char suffix[] = "/cal0";
	size_t len_suffix = strlen(suffix);
	size_t len_dir = strlen(calibration_dir);
	char * fname = (char *) malloc(sizeof(char) * (len_suffix + len_dir + 4) );
	
	strcpy(fname, calibration_dir);
	strcat(fname, suffix);
	
	printf("\n\n\n");
	
	for(i = 0; i < 9; i++) {
		
		fname[len_dir + len_suffix - 1] += 1;
		
		f = fopen(fname,"r");
		self->interpolators[i] = interpolator_load(f);
		fclose(f);
		
		printf("loaded %s\n",fname);
		
	}	
	
	fname[len_dir + len_suffix - 1] = '1';
	fname[len_dir + len_suffix] = '0';
	fname[len_dir + len_suffix + 1] = '\0';
	
	for(i = 10; i < 17; i++) {
		
		f = fopen(fname,"r");
		self->interpolators[i] = interpolator_load(f);
		fclose(f);
		
		printf("loaded %s\n",fname);
		
		fname[len_dir + len_suffix] += 1;

	}
	
	printf("\n\n\n");
	
	
	/* copy over the port, hostname, and board url to the CryoListener */
	
	self->hostname = (char *) malloc( sizeof(char) * (strlen(hostname) + 1) );
	memcpy(self->hostname, hostname, sizeof(char) * (strlen(hostname) + 1) );
	
	self->board_url = (char *) malloc( sizeof(char) * (strlen(board_url) + 1) );
	memcpy(self->board_url, board_url, sizeof(char) * (strlen(board_url) + 1) );
	
	self->port = (char *) malloc( sizeof(char) * (strlen(port) + 1) );
	memcpy(self->port, port, sizeof(char) * (strlen(port) + 1) );
	
	self->calibration_dir = (char *) malloc( sizeof(char) * (strlen(calibration_dir) + 1) );
	memcpy(self->calibration_dir, calibration_dir, sizeof(char) * (strlen(calibration_dir) + 1) );
	
	
	printf("CryoListener Parameters:\n\n   \
local IP: %s\n   \
local port: %s\n   \
board location: %s\n   \
calibration file directory: %s\n\n", 
	self->hostname, self->port, self->board_url, self->calibration_dir);
	
	
	/* setup the CryoStreamer */
	
	self->cs = new_CryoStreamer(hostname, port);
	
	fprintf(stderr, "new_CryoStreamer()finished\n");
	
	launch_streamer(hostname, port, board_url);
	
	fprintf(stderr, "launch_streamer() finished\n");
	
	/* retrieve the sensor sense and drive resistances */
		
	get_drive_sense_resistance(board_url, &(self->r));
	
	
	
	return self;
	
}

void delete_CryoListener(CryoListener * self) {
	
	int i;
	
	kill_streamer(self->hostname, self->port, self->board_url);
	delete_CryoStreamer(self->cs);
	
	for(i = 0; i < NUM_SENSOR_CHANNELS; i++) {
		free(self->interpolators[i]);
	}
	
	free(self->hostname);
	free(self->port);
	free(self->board_url);
	
	free(self);
	
	
}

int get_temperature(CryoListener * self) {
	
	int j;
	double v, i, g_v, g_i, rms_i, rms_v, vi, vq, iq, ii, r;
	
	j = CryoStreamer_get_samples(self->cs, &(self->cryosamples), 0);
	
	if(!j) { return 0; }
	
	for(j = 0; j < NUM_SENSOR_CHANNELS; j++) {
		
		vi = self->cryosamples._vi[j];
		vq = self->cryosamples._vq[j];
		ii = self->cryosamples._ii[j];
		iq = self->cryosamples._iq[j];
		
		rms_v = sqrt((vi*vi) + (vq*vq));
		rms_i = sqrt((ii*ii) + (iq*iq));
		
		g_v = 1 + (49.4e3 / self->r.sense[j]);
		g_i = 1 + (49.4e3 / self->r.drive[j]);
		
		v = rms_v * 10 / (g_v * (1<<24));
		i = rms_i * 1e4 * g_i * (1<<24);
		
		r = v / i;
		
		self->kelvin[j] = interpolator_get(self->interpolators[j], r);	
		
	}
	
	return 1;
}
