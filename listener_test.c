#include "listener.h"


int main() {
	
	int i,j;
	
	int nsamples = 125;
	
	CryoListener * listener = new_CryoListener("10.0.2.15","12122","http://192.168.17.139/tuber","/home/jesus/cryoboard/libcryo_test");
	
	
	for(i = 0; i < nsamples; i++) {
		
		get_temperature(listener);
		
		for(j = 0; j < NUM_SENSOR_CHANNELS; j++) {
			
			printf("%lf ", listener->kelvin[j]);
			if( j == NUM_SENSOR_CHANNELS-1 ) {
				printf("\n");
			}
			
		}
	}

	delete_CryoListener(listener);
	
	return 0;
}
