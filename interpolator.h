#include <stdio.h>
#include <stdlib.h>

#ifndef __CRYO_INTERPOLATOR__
#define __CRYO_INTERPOLATOR__

typedef struct interpolator {
	
	double * y;
	double * x;
	int start;
	int end;
	int len;
	
} interpolator;

interpolator * interpolator_load(FILE * src);
void interpolator_delete(interpolator * self);
double interpolator_get(interpolator * self, double y);



#endif
