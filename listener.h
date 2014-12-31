#include "cryo.h"
#include "interpolator.h"
#include <math.h>

typedef struct CryoListener {
	
	char * hostname;
	char * port;
	char * board_url;
	char * calibration_dir;
	double kelvin[NUM_SENSOR_CHANNELS];
	resistance r;
	CryoSamples cryosamples;
	CryoStreamer * cs;
	interpolator  * interpolators[NUM_SENSOR_CHANNELS];
	
} CryoListener;

CryoListener * new_CryoListener(
						char * hostname,
						char * port,
						char * board_url,
						char * calibration_dir);
void delete_CryoListener(CryoListener * self);
int get_temperature(CryoListener * self);
