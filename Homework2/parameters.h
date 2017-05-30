//---------------- PARAMETERS.H ----------------------- 

#define TICK_TIME 100000000
#define CNTRL_TIME 50000000

#define TASK_PRIORITY 1

#define STACK_SIZE 10000

#define BUF_SIZE 10

#define SEN_SHM 121111
#define ACT_SHM 112112
#define REFSENS 111213
#define KTS_SHM 212211 //usata per passare il riferimento del task livello kernel in modo da effettuare la resume
#define STATUS_SHM 213321 //usata per condividere lo stato dell'applicazione
#define REQ_SHM    133321


#define ACTUATE_MBX "ACT_MBX"
#define FILTER_MBX "FIL_MBX"
#define REQUEST_MBX "REQ_MBX"
#define STATUS_MBX "STA_MBX"

#define SPACE_SEM 1234444
#define MEAS_SEM 1234445
#define ACQ_MUTEX 323232
#define FIL_MUTEX 344344
#define ALLARM_SEM "ALL_SEM" //semaforo per inviare l'allarme al task allarm
#define MUT_P     "MUTEX"
#define CON_MUTEX "CON_MUTEX"


#define ACTIVE 1
#define FAILED -1

typedef struct  {
    int req;
    int buffer_req;
    int status_controller_u;
    int status_controller_k;
    int buffer[BUF_SIZE];
    int control_u;
    int control_k;
    int actuate;

}status_struct;

