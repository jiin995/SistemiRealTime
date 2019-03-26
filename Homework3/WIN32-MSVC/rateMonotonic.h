#ifndef __RM
#define __RM

#include "FreeRTOSConfig.h"

#define MAXPRIO configMAX_PRIORITIES
#define MINPRIO 1

typedef struct {
	int id;
	int periodo;
}periodiID;

	int * rateMonotonicPriority(int *periodi, int nPeriodi,int *minPeriod);

#endif
