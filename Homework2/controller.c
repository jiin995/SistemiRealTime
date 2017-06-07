//------------------- CONTROLLER.C ---------------------- 

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
#define CPUMAP 0x1

//emulates the controller

static RT_TASK *main_Task;
static RT_TASK *read_Task;
static RT_TASK *filter_Task;
static RT_TASK *control_Task;
static RT_TASK *write_Task;
static RT_TASK *gather_Task ;
static int keep_on_running = 1;

static pthread_t read_thread;
static pthread_t filter_thread;
static pthread_t control_thread;
static pthread_t write_thread;
static pthread_t gather_thread;


static RTIME sampl_interv;   //Periodo di cui i task sono periodici

static void endme(int dummy) {keep_on_running = 0;}

int* sensor;
int* actuator;
int* reference;

int buffer[BUF_SIZE];
int head = 0;
int tail = 0;

int avg = 0;
int control =0;

status_struct * status;

RT_TASK **shm_c_k;

SEM* space_avail;
SEM* meas_avail;
SEM* allarm;
SEM* mutex_acquire;
SEM* mutex_filter;
SEM* run_controller;

MBX* filter_mbx;
MBX* actuate_mbx;
MBX* request_mbx;
MBX* status_mbx;


static void * acquire_loop(void * par) {
	
	if (!(read_Task = rt_task_init_schmod(nam2num("READER"), 1, 0, 0, SCHED_FIFO, CPUMAP))) {
		printf("CANNOT INIT SENSOR TASK\n");
		exit(1);
	}

	RTIME expected = rt_get_time() + sampl_interv;
	rt_task_make_periodic(read_Task, expected, sampl_interv);
	rt_make_hard_real_time();

	while (keep_on_running)
	{
		// DATA ACQUISITION FROM PLANT
		rt_sem_wait(mutex_acquire);
		rt_sem_wait(space_avail);
		
		buffer[head] = (*sensor);
		head = (head+1) % BUF_SIZE;

		rt_sem_signal(mutex_acquire);
		rt_sem_signal(meas_avail);

		rt_task_wait_period();
	}
	rt_task_delete(read_Task);
	return 0;
}

static void * filter_loop(void * par) {

	if (!(filter_Task = rt_task_init_schmod(nam2num("FILTER"), 2, 0, 0, SCHED_FIFO, CPUMAP))) {
		printf("CANNOT INIT FILTER TASK\n");
		exit(1);
	}

	RTIME expected = rt_get_time() + sampl_interv;
	rt_task_make_periodic(filter_Task, expected, sampl_interv);
	rt_make_hard_real_time();

	int cnt = BUF_SIZE;
	unsigned int sum = 0;
	while (keep_on_running)
	{
		// FILTERING (average)
		rt_sem_wait(mutex_filter);
		rt_sem_wait(meas_avail); 

		sum += buffer[tail];
		tail = (tail+1) % BUF_SIZE;

		rt_sem_signal(mutex_filter);
		rt_sem_signal(space_avail);
		
		cnt--;

		if (cnt == 0) {
			cnt = BUF_SIZE;
			avg = sum/BUF_SIZE;
			sum = 0;
			// sends the average measure to the controller
			rt_send_if(control_Task, avg);	//se si blocca il controll task e uso la send normale anche filter si blocca e il controller liv kernel non continua la sua esecuzione
			// sends the average measure to the controller
			//uso la if in modo da evitare che il task si blocchi qual'ora la mailbox sia piana
			rt_mbx_send_if(filter_mbx,&avg,sizeof(int ));


		}
		rt_task_wait_period();
	}
	rt_task_delete(filter_Task);
	return 0;
}

static void * control_loop(void * par) {

	if (!(control_Task = rt_task_init_schmod(nam2num("CONTROL"), 3, 0, 0, SCHED_FIFO, CPUMAP))) {
		printf("CANNOT INIT CONTROL TASK\n");
		exit(1);
	}

	RTIME expected = rt_get_time() + sampl_interv;
	rt_task_make_periodic(control_Task, expected, BUF_SIZE*sampl_interv);
	rt_make_hard_real_time();

    rt_task_resume(*shm_c_k);

	unsigned int plant_state = 0;
	int error = 0;
	unsigned int control_action = 0;
	while (keep_on_running)
	{
		rt_sem_wait(run_controller);
		// receiving the average plant state from the filter
		rt_receive(0, &plant_state); 
		// computation of the control law
		error = (*reference) - plant_state;

		if (error > 0) control_action = 1;
		else if (error < 0) control_action = 2;
		else control_action = 3;

		// sending the control action to the actuator
		rt_send(write_Task, control_action);
		rt_sem_signal(run_controller);

		rt_task_wait_period();

	}
	rt_task_delete(control_Task);
	return 0;
}

static void * actuator_loop(void * par) {

	if (!(write_Task = rt_task_init_schmod(nam2num("WRITE1"), 4, 0, 0, SCHED_FIFO, CPUMAP))) {
		printf("CANNOT INIT ACTUATOR TASK\n");
		exit(1);
	}

	RTIME expected = rt_get_time() + sampl_interv;
	rt_task_make_periodic(write_Task, expected, BUF_SIZE*sampl_interv);
	rt_make_hard_real_time();

	unsigned int control_action = 0;
	unsigned int control_action_k = 0;


	while (keep_on_running)
	{
		int c_task=1;

		// receiving the control action from the controller
		//if(!rt_receive_if(0, &control_action)){
		if(!rt_receive_until(0,&control_action,sampl_interv)){
			//il task controll a livello utente e' terminato o non ha inviato in tempo il segnale di controllo
			c_task=0;
			status->status_controller_u=FAILED;
			status->control_u=FAILED;
		}
		else{
			status->status_controller_u=ACTIVE;
			status->control_u=control_action;
		}
		//se ricevo dal controller in modalita' kernel
		if(rt_mbx_receive_if(actuate_mbx,&control_action_k,sizeof(int))==0){
		//if(rt_mbx_receive_until(actuate_mbx,&control_action_k,sizeof(int),sampl_interv)!=0){
			status->status_controller_k=ACTIVE; //ho ricevuto un messaggio dal task di livello kernel 
			status->control_k=control_action_k;

			if(c_task){	//ho ricevuto dai due controller
				if(control_action!=control_action_k){
					rt_sem_signal(allarm); //i due controller mi danno valori diversi
					control_action=0;
				}
			}else {
				control_action=control_action_k;
			} //ho ricevuto solo dal task in modalita' kernel
		}else{ //non ho ricevuto dal task in modalita' kernel
			status->status_controller_k=FAILED;
			status->control_k=FAILED;

			 if(!c_task) // non ho ricevuto informazioni da nessuno dei due task
				control_action=4;
		}
		
		switch (control_action) {
			case 1: control = 1; break;
			case 2:	control = -1; break;
			case 3:	control = 0; break;
			case 4: control = -2; break;
			default: control = 0;
		}

		status->actuate=control;
		
		(*actuator) = control;

		rt_task_wait_period();
	}
	rt_task_delete(write_Task);
	return 0;
}

static void * gather_loop(void * par) {

	if (!(gather_Task = rt_task_init_schmod(nam2num("GATHER"), 5, 0, 0, SCHED_FIFO, CPUMAP))) {
		printf("CANNOT INIT GATHER TASK\n");
		exit(1);
	}

	int req;

	while(keep_on_running){

		if(rt_mbx_receive(request_mbx,&req,sizeof(int))==0){

			rt_sem_wait(mutex_acquire); //blocco acquire
			rt_sem_wait(mutex_filter);	//blocco filter

   			//aggiorno il buffer prima di inviarlo 
				int i=0;
				for( i=0;i<BUF_SIZE;i++){
					status->buffer[i]=buffer[i];
				}

			rt_sem_signal(mutex_acquire);
			rt_sem_signal(mutex_filter);

			rt_mbx_send_if(status_mbx,status,sizeof(status_struct));

		}
		
	}
}


int main(void)
{
	printf("The controller is STARTED!\n");
 	signal(SIGINT, endme);

	if (!(main_Task = rt_task_init_schmod(nam2num("MAINTSK"), 0, 0, 0, SCHED_FIFO, 0xF))) {
		printf("CANNOT INIT MAIN TASK\n");
		exit(1);
	}

	//attach to data shared with the controller
	sensor = rtai_malloc(SEN_SHM, sizeof(int));
	actuator = rtai_malloc(ACT_SHM, sizeof(int));
	reference = rtai_malloc(REFSENS, sizeof(int));
	shm_c_k=rtai_malloc(KTS_SHM,sizeof(RT_TASK *)); //usata per effettuare resume sul controller kernel
	status=rtai_malloc(STATUS_SHM,sizeof(status_struct)); //usata per salvare lo stato tra gather e gli altri thread

	(*reference) = 110;

	//Init message box for send data from and to controll kernel module
	filter_mbx=rt_typed_named_mbx_init(FILTER_MBX,sizeof(int),FIFO_Q);
    actuate_mbx=rt_typed_named_mbx_init(ACTUATE_MBX,sizeof(int),FIFO_Q);
	request_mbx=rt_typed_named_mbx_init(REQUEST_MBX,sizeof(int),FIFO_Q);
	status_mbx=rt_typed_named_mbx_init(STATUS_MBX,sizeof(status_struct),FIFO_Q); //mailbox per inviare lo stato dal gather al diag



	space_avail = rt_typed_sem_init(SPACE_SEM, BUF_SIZE, CNT_SEM | PRIO_Q);
	meas_avail = rt_typed_sem_init(MEAS_SEM, 0, CNT_SEM | PRIO_Q);

	allarm=rt_typed_named_sem_init(ALLARM_SEM,0,BIN_SEM|PRIO_Q);
	mutex_filter = rt_typed_sem_init(FIL_MUTEX, 1, BIN_SEM | PRIO_Q); //per bloccare il filter quando gather deve aggiornare lo stato
	mutex_acquire  = rt_typed_sem_init(ACQ_MUTEX, 1, BIN_SEM | PRIO_Q); //per bloccare l'acquire quando gather deve aggiornare lo stato

	run_controller = rt_typed_named_sem_init(CON_MUTEX, 1, BIN_SEM | PRIO_Q);
	
	if (rt_is_hard_timer_running()) {
		printf("Skip hard real_timer setting...\n");
	} else {
		rt_set_oneshot_mode();
		start_rt_timer(0);
	}

	sampl_interv = nano2count(CNTRL_TIME);
	
	// CONTROL THREADS 
	pthread_create(&read_thread, NULL, acquire_loop, NULL);
	pthread_create(&filter_thread, NULL, filter_loop, NULL);
	pthread_create(&control_thread, NULL, control_loop, NULL);
	pthread_create(&write_thread, NULL, actuator_loop, NULL);
	pthread_create(&gather_thread, NULL, gather_loop, NULL);


	while (keep_on_running) {
		if((*actuator)==-2){
			printf("No Controller running\n");
		}
		printf("Control: %d\n",(*actuator));
		rt_sleep(10000000);
	}

    stop_rt_timer();
	rt_shm_free(SEN_SHM);
	rt_shm_free(ACT_SHM);
	rt_shm_free(REFSENS);
	rt_shm_free(KTS_SHM);
	rt_shm_free(STATUS_SHM); 

	
	rt_mbx_delete(actuate_mbx);
  	rt_mbx_delete (filter_mbx);
	rt_mbx_delete(request_mbx);
	rt_mbx_delete(status_mbx);
	
	rt_named_sem_delete(space_avail);
	rt_named_sem_delete(meas_avail);
	rt_named_sem_delete(allarm);
	rt_named_sem_delete(mutex_filter);
	rt_named_sem_delete(mutex_acquire);
	rt_named_sem_delete(run_controller);


	rt_task_delete(main_Task);
 	printf("The controller is STOPPED\n");
	return 0;
}




