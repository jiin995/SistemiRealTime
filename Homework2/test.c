
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

int main(){

    unsigned int control_action;
    unsigned int plant_state;

    scanf("%u",&plant_state);

    if(!(asyncTask = rt_task_init(nam2num("RTEEE"),1,STACK_SIZE,0))){
	    	printf("failed creating rt task\n");
		    exit(-1);
	}


    MBX* filter_mbx=rt_typed_named_mbx_init("FILTER",sizeof(int),FIFO_Q);

    MBX* actuate_mbx=rt_typed_named_mbx_init("ACTUATE",sizeof(int),FIFO_Q);

    printf("ecco %u  \n",plant_state);

    rt_mbx_send(filter_mbx,&plant_state,sizeof(int ));

    printf("ASPETTO %u  \n",plant_state);

    rt_mbx_receive(actuate_mbx,&control_action,sizeof(int));
        
    printf("%u \n",control_action);

    rt_mbx_delete(actuate_mbx);
    rt_mbx_delete (filter_mbx);
    

    return 0;
}