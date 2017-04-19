#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "/usr/realtime/include/rtai_shm.h"
#include "parameters.h"

static int end;
static void endme(int dummy) { end=1; }

int main (void)
{
    int*  onda;
    
    //gestione della terminazione del processo 
    signal(SIGINT, endme);
    // allocazione memoria
    onda = rtai_kmalloc(SHMNAM_1, 1);
    
    int i = 0;	
    
    while (!end) {
     
      for(i=0; i<NTASKS; i++)
      {		
		int j = 0;
		for (j = 0; j < onda[i]; j++) printf(" ");
      		printf("|\t\t");
      } 
      printf("\n");
      usleep(900); //attesa 
    }
    
    
    rtai_free (SHMNAM_1, &onda);
    
    return 0;
}

