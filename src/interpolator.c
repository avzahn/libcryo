#include "interpolator.h"

/* This whole file assumes the interpolation data is first sorted, and
 * second represents a sampling of an invertible continuous function. */


interpolator * interpolator_load(FILE * src) {

	int nlines = 0;
	int status = EOF + 1;
	int current_size = 128;
	
	interpolator * self;

	self = (interpolator *) malloc(sizeof(interpolator));
	
	if(self == NULL) {
		return NULL;
	}
	
	self->y = (double *) malloc(current_size * sizeof(double));
	self->x = (double *) malloc(current_size * sizeof(double));
	
	if(self->y == NULL || self->x == NULL) {
		return NULL;
	}
	
	while(status != EOF) {
		
		if(nlines == current_size) {
			current_size *= 2;
			self->y = (double *) realloc(self->y, current_size * sizeof(double));
			self->x = (double *) realloc(self->x, current_size * sizeof(double));
		}
		
		status = fscanf(src, "%lf %lf", nlines + self->y, nlines + self->x);
	
		nlines++;		
	}
	
	/* For some reason fscanf scans in two 0.0's for the last line if that line is
	 * just a newline character. Ignoring the last line like this might be a bug 
	 * though */
	
	self->len = nlines - 1;
	self->start = 0;
	self->end = nlines - 2;

	return self;
	
}

void interpolator_delete(interpolator * self) {
	free(self->y);
	free(self->x);
	free(self);
}

double interpolator_get(interpolator * self, double y) {
	
	double y_start, y_end, y_mid, m, x;
	int mid, r;
	
	/* self will carry the search bounds, which we search over
	 * with a nonrecursive bisection approach. */
	 	 
	/* Check that the search bounds contain the requested value.
	 * If they don't, try to expand them as far as the interpolator
	 * will go, and if that fails, return 0.0 */
	
	y_start = self->y[self->start];
	y_end = self->y[self->end];	
	
	if( y < y_start ) {
		if( y < self->y[0] ) {
			return 0.0;
		}
		y_start = self->y[0];
		self->start = 0;
	}
	if( y > y_end ) {
		if ( y > self->y[self->len - 1] ) {
			return 0.0;
		}
		y_end = self->y[self->len -1];
		self->end = self->len - 1;
	}	

	/* Now start bisecting */
	while(1) {
		
		/* Check that we aren't already done */
		if( self->end - self->start  == 1 ) {
			m =  (self->x[self->end] - self->x[self->start]) / (y_end - y_start);
			x =  self->x[self->start] + m * (y-y_start);
			break;
		}
		
		mid = (self->end + self->start) / 2;
		y_mid = self->y[mid];
		
		if( y > y_mid ) {
			y_start = y_mid;
			self->start = mid;
			continue;
		}
		y_end = y_mid;
		self->end = mid;		
	}
	
	/* Leave the search bounds for the next call nearby */
	r = self->len / 8;
	self->start -= r;
	self->end += r;
	if ( self->start  < 0 ) { self->start = 0; }
	if ( self->end  > self->len - 1 ) { self->end = self->len - 1; }
	
	return x;
	
}
