#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "/usr/realtime/include/rtai_shm.h"
#include "parameters.h"

static int end;
static void endme(int dummy) { end=1; }

int main (int argc, char ** argv)
{
    int*  ampiezza;
    
    // allocazione memoria
    ampiezza = rtai_kmalloc(SHMNAM_2, 1); 
    
    int i = 0;	

    for (i = 1; i < argc; i++) {ampiezza[i-1] = atoi(argv[i]);}
    
    rtai_free (SHMNAM_2, &ampiezza);
    
    return 0;
}

