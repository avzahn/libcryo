#include "listener.h"


int main() {
	
	int i,j;
	
	int nsamples = 125;
	
	/* Eventually we will want to build a real configuration interface,
	 * but for the moment this is fairly convenient */
	
	char hostname[] = "192.168.17.5";
	char port[] = "12122";
	char board_url[] = "http://192.168.17.139/tuber";
	char calibration_dir[] = "/home/polarbear/cryoboard_things/libcryo/cal";
	
	CryoListener * listener = new_CryoListener(hostname,
									port,
									board_url,
									calibration_dir);
	
	for(i = 0; i < nsamples; i++) {
		
		get_temperature(listener);
		
		for(j = 0; j < NUM_SENSOR_CHANNELS; j++) {
			
			printf("%lf ", listener->kelvin[j]);
		}
		printf("\n");
			
	}

	delete_CryoListener(listener);
	
	return 0;
}
