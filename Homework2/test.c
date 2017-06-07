
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
#include "parameters.h"

static RT_TASK* asyncTask;


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

   // shm=rtai_malloc(KTS_SHM,sizeof(RT_TASK *));
    //printf("%d \n",*shm);
    //rt_task_resume(*shm);

  	SEM* stop_controller = rt_typed_named_sem_init(CON_MUTEX, 1, BIN_SEM | PRIO_Q);
    SEM *	stop_controller_k=rt_typed_named_sem_init(STCK_SEM, 1, BIN_SEM | PRIO_Q);
    RT_TASK ** shm_c_k=rtai_malloc(KTS_SHM,sizeof(RT_TASK *)); //usata per effettuare resume sul controller kernel


    int i=1;

    while(i!=0){

        printf("[Tester ]\n1) Blocca Task livello utente\n");
        printf("2) Sblocca Task livello utente\n");
        printf("3) Blocca Task livello kernel\n");
        printf("4) Sblocca Task livello kernel\n");
        printf("5) Risveglia Task Kernel \n");
        printf("0) Termina \n");


        scanf("%d",&i);
        switch(i){
       
            case 1:{
                      rt_sem_wait(stop_controller);
                      printf("[Tester ]--> Bloccato Task livello utente\n");
                      break;
            }

            case 2:{
                      rt_sem_signal(stop_controller);
                      printf("[Tester ]--> Sbloccato Task livello utente\n");
                      break;
            }

            case 3:{
                      rt_sem_wait(stop_controller_k);
                      printf("[Tester ]--> Bloccato Task livello kernel\n");
                      break;
            }

            case 4:{
                      rt_sem_signal(stop_controller_k);
                      printf("[Tester ]--> Sbloccato Task livello kernel\n");
                      break;
            }
            case 5:{
                      rt_task_resume(*shm_c_k);
                      printf("[Tester ]--> Risvegliato Task livello kernel\n");
                      break;
            }
            case 0:{
                    printf("[Tester]--> Terminato\n");
                    break;
            }

            default:{
                      printf("[Tester ]--> Scelta non valida\n");
                      break;
            }

        }
    }

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