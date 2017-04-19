//-------------------Previtera Gabriele ====>Previtera Gabriele
#include <linux/module.h>
#include <asm/io.h>
#include <asm/rtai.h>
#include <rtai_shm.h>
#include <rtai_sched.h>
#include "parameters.h"

static RT_TASK tasks[NTASKS+1];
static RTIME cpu_use[NTASKS];
static RTIME utilization[NTASKS];

static int *onda;
static int *ampiezza;

static int semiperiodi[3] = { -1, -1, -1};
static int arr_argc = 0;


module_param_array(semiperiodi, int, &arr_argc, 0000);
MODULE_PARM_DESC(semiperiodi, "An array of half periods of wave");
 

static void wave_gen(int t)
{
	RTIME x;
	while(1){
		x=rt_get_time_ns();
		if(onda[t]>0)
			onda[t]=0;
		else
			onda[t]=ampiezza[t];
	
		cpu_use[t]=rt_get_time_ns()-x;
	//	utilization[t]=cpu_use[t]/semiperiodi[t];
		rt_task_wait_period();
	}
}

static void monitor(int in)
{
	RTIME util[NTASKS];
	int i,j;

	i=0;
	while(1)
	{
		if(i==100){
				for(j=0;i<3;i++){
				//	utilization[j]=utilization[j]/100;
					printk(KERN_INFO " \n [Wave-Generator] : TASK %d Utilizzazione %d \n",j,utilization[j]);
				}
				i=0;
			}
		else{
			for(j=0;i<3;i++)
				util[j]+=utilization[j];
			i++;
		}
		rt_task_wait_period();
	}
}

int rest_div(int r,int t){
	while(r<=0){
		r=r-t;
	}
	return -r;
}

int init_module(void)
{

	printk(KERN_INFO " \n [Wave-Generator] : Caricato \n");
    RTIME tick_period;	

	int i=0;

	//Inizializzo le SharedMemory
	
    onda = rtai_kmalloc(SHMNAM_1, 1);
		for(i=0;i<NTASKS;i++){
			onda[i]=0;
		}

    ampiezza = rtai_kmalloc(SHMNAM_2,1);
		for(i=0;i<NTASKS;i++){
			ampiezza[i]=0;
		}
			
	for(i=0;i<NTASKS;i++){
			if((semiperiodi[i]<10)||(semiperiodi[i]>100))
				printk(KERN_INFO "[Wave-Generator]:periodo non valido %d",semiperiodi[i]);
	}

	rt_set_periodic_mode();	//Setto la modalita' periodica per il timer


	tick_period = start_rt_timer(nano2count(TICK_PERIOD));
	
	//Creo i Task che generano le onde

	for(i=0;i<NTASKS;i++){
    		rt_task_init(&tasks[i], wave_gen, i, STACK_SIZE, TASK_PRIORITY, 1, 0);
			rt_task_make_periodic(&tasks[i], rt_get_time() + tick_period, tick_period*semiperiodi[i]);
	}
	
	rt_task_init(&tasks[NTASKS], monitor, 0, STACK_SIZE, TASK_PRIORITY, 1, 0);
	rt_task_make_periodic(&tasks[NTASKS], rt_get_time() + tick_period, tick_period);

	rt_spv_RMS(hard_cpu_id());

    return 0;

}

void cleanup_module(void)

{

//Fermo il timer
    stop_rt_timer();

//Termino i processi
	int i=0;
		for(i=0;i<NTASKS;i++){
   			 rt_task_delete(&tasks[i]);
		}

//Rilascio le SharedMemory
    rtai_kfree(SHMNAM_1);
    rtai_kfree(SHMNAM_2);

    return;

}
