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

static RT_TASK* psTask;
static RT_TASK* main_Task;
static RTIME sampl_interv;


static pthread_t polling_thread;


SEM* mutex;
MBX* request_mbx;
int *req;
int op;

static int keep_on_running = 1;

static void endme(int dummy) {keep_on_running = 0;}

static void polling_loop(long p){


    if (!(psTask = rt_task_init_schmod(nam2num("POLLIN"), 0, 0, 0, SCHED_FIFO, 0xF))) {
	    	printf("failed creating rt task\n");
		    exit(-1);
	}

    RTIME expected = rt_get_time() + sampl_interv;
    
    rt_task_make_periodic(psTask, expected, sampl_interv); // /BUF_SIZE
    rt_make_hard_real_time();

    while(keep_on_running){

        rt_sem_wait(mutex);
           if((*req)==1){
                if(rt_mbx_send_if(request_mbx,req,sizeof(int))!=0)                //     printf("[Polling Server] --> Error while send the request action to actuate \n");
                    op=-1;
		    	else // printf("[Polling Server] --> Request inoltred to Gather task");
                   op=1; 
                *req=0;           
            }else
                op=0;

        rt_sem_signal(mutex);

        rt_task_wait_period();
    }

    rt_shm_free(req);
    rt_named_sem_delete(mutex);
    rt_mbx_delete (request_mbx);
    rt_task_delete(psTask);

    return 0;
}

int main(void){

    printf("The Polling Server task is STARTED!\n");
 	signal(SIGINT, endme);

	if (!(main_Task = rt_task_init_schmod(nam2num("MAIN2"), 0, 0, 0, RT_SCHED_RR, 0x1))) {
		printf("CANNOT INIT MAIN TASK\n");
		exit(1);
	}

    mutex=rt_typed_named_sem_init(MUT_P,1,BIN_SEM|PRIO_Q);
    req=rtai_malloc(REQ_SHM,sizeof(int));
    request_mbx=rt_typed_named_mbx_init(REQUEST_MBX,sizeof(int),FIFO_Q);

    op=0;
    *req=0;
    
    if (rt_is_hard_timer_running()) {
		printf("Skip hard real_timer setting...\n");
	} else {
		rt_set_oneshot_mode();
		start_rt_timer(0);
	}

    sampl_interv = nano2count(CNTRL_TIME/BUF_SIZE);

    pthread_create(&polling_thread, NULL, polling_loop, NULL);

    while(keep_on_running){
        if(op==-1){
            printf("[Polling Server] --> Error while send the request to Gather task \n");
        }
        else if(op==1){
            printf("[Polling Server] --> Request inoltred to Gather task \n");
        }

       //printf("active\n");
        rt_sleep(10000000); //perche' ???
    }
    
    return 0;
}