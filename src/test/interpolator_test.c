#include "interpolator.h"

int main() {
	
	FILE * f;
	interpolator * i;
	
	f = fopen("../../cal/cal1","r");
	i = interpolator_load(f);
	fclose(f);
	
	
	printf("%lf, %lf\n", i->y[0], i->x[0]);
	printf("%lf, %lf\n", i->y[10], i->x[10]);
	printf("%lf, %lf\n", i->y[99], i->x[99]);
	
	printf("%lf : %lf\n", 1.5, interpolator_get(i, 1.5));
	printf("%lf : %lf\n", 2.0, interpolator_get(i, 2.0));
	printf("%lf : %lf\n", 55.2, interpolator_get(i, 55.2));
	printf("%lf : %lf\n", 70.3, interpolator_get(i, 70.3));
	
	interpolator_delete(i);
}
