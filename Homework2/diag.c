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

static RT_TASK* diagTask;

static int keep_on_running = 1;

SEM* mutex;
SEM* status_mbx;

int *req;

status_struct *status;


//static void endme(int dummy) {keep_on_running = 0;}

int main(void){

    int i=1;

    printf("The diag task is STARTED!\n");
 //	signal(SIGINT, endme);

    if(!(diagTask = rt_task_init(nam2num("DIAG"),1,STACK_SIZE,0))){
	    	printf("failed creating rt task\n");
		    exit(-1);
	}

    mutex=rt_typed_named_sem_init(MUT_P,1,BIN_SEM|PRIO_Q);
    status_mbx=rt_typed_named_mbx_init(STATUS_MBX,sizeof(status_struct),FIFO_Q);

    req=rtai_malloc(REQ_SHM,sizeof(int));

    while(keep_on_running){

        scanf("%d",&i);
        rt_sem_wait(mutex);

        *req=1;

        rt_sem_signal(mutex);

        printf("Richiesta inviata \n");

        rt_mbx_receive(status_mbx,status,sizeof(status_struct));
        printf("Richiesta ricevuto \n");


    }

    return 0;
}