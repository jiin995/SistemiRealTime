
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/stat.h>
#include <asm/io.h>
#include <asm/rtai.h>
#include <rtai_shm.h>
#include <rtai_msg.h>
#include <rtai_mbx.h>
#include <rtai_sched.h>
#include "parameters.h"

static RT_TASK **shm_c_k; // utilizzato per condividere il puntatore al task in modo tale che possa essere ripreso quando il controllore e' pronto cosicche' possiamo evitare allarmi dovuti alla non sync
static RT_TASK controller_k;

static MBX  * actuate_mbx;
static MBX  * filter_mbx;

static int suspend = -1;
module_param(suspend, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(suspend, "if set 1 the task not suspend");

static int* reference;

static void control_loop(int in){

	unsigned int plant_state = 0;
	int error = 0;
	unsigned int control_action = 0;

	if(suspend!=1){
		printk(KERN_INFO "Suspend %d   %d",*shm_c_k,&controller_k);
		rt_task_suspend(&controller_k);
		printk(KERN_INFO "Resume");
	}

	while (1){
		// receiving the average plant state from the filter
		if(!rt_mbx_receive(filter_mbx,&plant_state,sizeof(int))){ //non uso la receive if perche' se non arriva in tempo il msg e' actuator a decidere cosa fare 
			//0 ho ricevuto il messaggio 
     	   printk(KERN_INFO"[Controller_Kernel] --> Plant_State %d \n",plant_state);

			// computation of the control law
			error = (*reference) - plant_state;

			if (error > 0) control_action = 1;
			else if (error < 0) control_action = 2;
			else control_action = 3;
			//control_action=4;
			// sending the control action to the actuator		
		}else{
			printk(KERN_INFO"[Controller_Kernel] --> Error while receiving message from filter  \n");
			control_action=0;
		}
		if(rt_mbx_send_if(actuate_mbx,&control_action,sizeof(int))!=0)
				printk(KERN_INFO"[Controller_Kernel] --> Error while send the control action to actuate \n");
			else
				printk(KERN_INFO"[Controller_Kernel] --> Control_action %d \n",control_action);	
		rt_task_wait_period();
    }

}

int init_module(void){

	printk(KERN_INFO"[Controller_Kernel] --> Module loaded");

	//Attach at shared memory
	reference = rtai_kmalloc(REFSENS, sizeof(int));
	
    if (rt_is_hard_timer_running()) {
		printk(KERN_INFO "Skip hard real_timer setting...");
	} else {
		rt_set_oneshot_mode();
		start_rt_timer(0);
	}
    
    rt_task_init(&controller_k, control_loop, 0, STACK_SIZE, TASK_PRIORITY, 1, 0);

	shm_c_k=rtai_kmalloc(KTS_SHM,sizeof(RT_TASK *));
	*shm_c_k=&controller_k;

    RTIME   sampl_interv = nano2count(CNTRL_TIME);

	filter_mbx=rt_typed_named_mbx_init(FILTER_MBX,sizeof(int),FIFO_Q);
    actuate_mbx=rt_typed_named_mbx_init(ACTUATE_MBX,sizeof(int),FIFO_Q);

   	rt_task_make_periodic(&controller_k, rt_get_time() + sampl_interv, sampl_interv*BUF_SIZE);

    return 0;
}

void cleanup_module(void){

 //   stop_rt_timer();
 	rt_task_delete(&controller_k);

	printk(KERN_INFO"[Controller_Kernel] --> Module removed");

	rtai_kfree(KTS_SHM);
	rt_mbx_delete(actuate_mbx);
	rt_mbx_delete (filter_mbx);


}