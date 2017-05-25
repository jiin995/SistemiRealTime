#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <rtai_lxrt.h>
#include <rtai_shm.h>
#include <rtai_sem.h>
#include <rtai_mbx.h>
#include <rtai_msg.h>
#include <sys/io.h>
#include <signal.h>
#include "parameters.h"

static RT_TASK* asyncTask;

static int keep_on_running = 1;


static void endme(int dummy) {keep_on_running = 0;}

int main(void){

    int i=1;

    printf("The allarm task is STARTED!\n");
 	signal(SIGINT, endme);

    if(!(asyncTask = rt_task_init(nam2num("ALLARM"),1,STACK_SIZE,0))){
	    	printf("failed creating rt task\n");
		    exit(-1);
	}

    SEM* allarm=rt_typed_named_sem_init(ALLARM_SEM,0,BIN_SEM|PRIO_Q);


    while(keep_on_running){

        rt_sem_wait(allarm);
        printf("%d) -->ALLARM \n",i++);

    }

//  rt_named_sem_delete(allarm);
    return 0;
}