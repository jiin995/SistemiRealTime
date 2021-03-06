//-------------------Previtera Gabriele ==> HomeWork1-----------


#include <linux/module.h>
#include <asm/io.h>
#include <asm/rtai.h>
#include <rtai_shm.h>
#include <rtai_sched.h>
#include "parameters.h"

static RT_TASK tasks[NTASKS+1];
static RTIME cpu_use[NTASKS];
static RTIME utilization[NTASKS];
static RTIME slack_time[NTASKS];

static int *onda;
static int *ampiezza;

static int semiperiodi[3] = { -1, -1, -1};
static int arr_argc = 0;


module_param_array(semiperiodi, int, &arr_argc, 0000);
MODULE_PARM_DESC(semiperiodi, "An array of half periods of wave");
 

static void wave_gen(int t){

	RTIME start;

	while(1){
		start=rt_get_cpu_time_ns();

		if(onda[t]>0)
			onda[t]=0;
		else
			onda[t]=ampiezza[t];
	
		cpu_use[t]=rt_get_cpu_time_ns()-start;
		
	// la moltiplico per 1000 per vedere il risultato della moltiplicazione in intero
		utilization[t]=cpu_use[t]*(100/semiperiodi[t]);

	//non conoscendo il tempo di arrivo uso come tempo di arrivo il tempo in cui il task inizia l'esecuzione
	//applicando la formula deadline-tempoDiArrivo-tempoDiElaborazione
		slack_time[t]=count2nano(next_period())-start-cpu_use[t];

		rt_task_wait_period();
	}
}

static void monitor(int in){

	RTIME util[NTASKS];
	RTIME slack[NTASKS];
	int i,j;

	for(i=0;i<NTASKS;i++)
		util[i]=0;

#ifdef DEBUG
    printk(KERN_INFO " \n [Wave-Generator] : Task Monitor Avviato \n");
#endif
	i=0;
	while(1){
		if(i==100){
				for(j=0;j<3;j++){
					util[j]=util[j]*0.01;
					slack[j]=slack[j]*0.01;
					printk(KERN_INFO " \n [Wave-Generator]:{Monitor}==> TASK %d Utilizzazione media %d \n",j,util[j]);
					printk(KERN_INFO " [Wave-Generator]:{Monitor}==> TASK %d Slack_Time medio %d \n",j,slack[j]);
				}
				i=0;
			}
		else{
			for(j=0;j<3;j++){
				//potrei benissimo includere questa parte nel task gen di onde, ma preferisco tener separato il calcolo dello slack e dell'utilizzo
				//dal task che fa la media e li stampa.

				util[j]+=utilization[j];
				slack[j]+=slack_time[j];
			}
			i++;
		}
		rt_task_wait_period();
	}
}

int init_module(void){

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
				printk(KERN_ERR "[Wave-Generator]:Periodo non valido %d: %d",i,semiperiodi[i]);
	}

	rt_set_periodic_mode();	//Setto la modalita' periodica per il timer

	tick_period = start_rt_timer(nano2count(TICK_PERIOD));
	
//Creo i Task che generano le onde
	for(i=0;i<NTASKS;i++){
    		rt_task_init(&tasks[i], wave_gen, i, STACK_SIZE, TASK_PRIORITY, 1, 0);
			rt_task_make_periodic(&tasks[i], rt_get_time() + tick_period, tick_period*(semiperiodi[i]));
	}

//creo il Task monitor	
	rt_task_init(&tasks[NTASKS], monitor, 1, STACK_SIZE, TASK_PRIORITY, 1, 0);
	rt_task_make_periodic(&tasks[NTASKS], rt_get_time() + tick_period, tick_period);

//Schedulo i task con RM
	rt_spv_RMS(hard_cpu_id());

    return 0;

}

void cleanup_module(void){

//Fermo il timer
    stop_rt_timer();

//Termino i task che generano le onde e il task monitor
	int i=0;
		for(i=0;i<NTASKS+1;i++){
   			 rt_task_delete(&tasks[i]);
		}

//Rilascio le SharedMemory
    rtai_kfree(SHMNAM_1);
    rtai_kfree(SHMNAM_2);

	printk(KERN_INFO "[Wave-Generator]:Rimosso");

    return;

}
