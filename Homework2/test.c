
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <asm/rtai.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <rtai_lxrt.h>
#include <rtai_shm.h> 
#include <rtai_sem.h>
#include <rtai_msg.h>
#include <rtai_mbx.h>
#include <sys/io.h>
#include <signal.h>
#include "parameters.h"

static RT_TASK* asyncTask;
RT_TASK **shm;


int main(){

    unsigned int control_action;
    unsigned int plant_state;

    //scanf("%u",&plant_state);

    if(!(asyncTask = rt_task_init(nam2num("RTEEE"),1,STACK_SIZE,0))){
	    	printf("failed creating rt task\n");
		    exit(-1);
	}


  //  MBX* filter_mbx=rt_typed_named_mbx_init(FILTER_MBX,sizeof(int),FIFO_Q);

    //MBX* actuate_mbx=rt_typed_named_mbx_init(ACTUATE_MBX,sizeof(int),FIFO_Q);

    shm=rtai_malloc(KTS_SHM,sizeof(RT_TASK *));
    printf("%d \n",*shm);
    rt_task_resume(*shm);

    rt_task_delete(asyncTask);


   /* printf("Invio %u  \n",plant_state);

    rt_mbx_send(filter_mbx,&plant_state,sizeof(int ));

    rt_mbx_receive_if(actuate_mbx,&control_action,sizeof(int));
        
    printf("Ricevuto %u \n",control_action);
    */

      rt_shm_free(KTS_SHM);
    //  rt_mbx_delete(actuate_mbx);
     // rt_mbx_delete (filter_mbx);
    
    return 0;
}