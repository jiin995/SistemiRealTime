/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "rateMonotonic.h"

#define mainCREATE_SIMPLE_BLINKY_DEMO_ONLY	1

static int *onda;
static int *ampiezza;
static int *semiperiodi;

/*Assumendo che i generatori di onde scrivono in aree di memorie differente, vogliamo garantire
che i nostri generatori possono scrivere contemporaneamente nell'array, ma cio' puo' avvenire solo
quando il task scope non legge i dati dell'array, introduciamo la starvation per il processo scope!*/

static SemaphoreHandle_t mutexScope;
static SemaphoreHandle_t mutexGeneratore;
int numeroGeneratori=0;

static TimerHandle_t xTimer = NULL;

#define NTASKS 3
#define mainREGION_1_SIZE	7001
#define mainREGION_2_SIZE	18105
#define mainREGION_3_SIZE	2807

void vFullDemoTickHookFunction( void );
void vFullDemoIdleFunction( void );
static void  prvInitialiseHeap( void );
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );


static void prvSaveTraceFile( void );

/*-----------------------------------------------------------*/
StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];
traceLabel xTickTraceUserEvent;
static portBASE_TYPE xTraceRunning = pdTRUE;

/*-----------------------------------------------------------*/


static void scope(int period) {
	int i;

	TickType_t xNextWakeTime;

	const TickType_t xBlockTime = pdMS_TO_TICKS(period);
	xNextWakeTime = xTaskGetTickCount();

	while (1) {
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);		

		xSemaphoreTake(mutexScope, portMAX_DELAY);

			for (i = 0; i<NTASKS; i++){
				int j = 0;
				for (j = 0; j < onda[i]; j++) printf(" ");
				printf("|\t\t");
			}
			printf("\n");

		xSemaphoreGive(mutexScope, portMAX_DELAY);
	}
}

static void wave_gen(int parameters) {

	int id = parameters;
	TickType_t xNextWakeTime;

	const TickType_t xBlockTime = pdMS_TO_TICKS(semiperiodi[id]);
	xNextWakeTime = xTaskGetTickCount();

	printf("Generatore %d avviato con periodo %d\n", id, semiperiodi[id]*2);

	for (;;) {

		vTaskDelayUntil(&xNextWakeTime, xBlockTime);

		xSemaphoreTake(mutexGeneratore, portMAX_DELAY);
			numeroGeneratori++;
				if (numeroGeneratori == 1)
					xSemaphoreTake(mutexScope, portMAX_DELAY); // Blocco l'accesso al task scope all'array
		xSemaphoreGive(mutexGeneratore, portMAX_DELAY);


		if (onda[id] > 0)
				onda[id] = 0;
		else
				onda[id] = ampiezza[id];	

		xSemaphoreTake(mutexGeneratore, portMAX_DELAY);
			numeroGeneratori--;
				if (numeroGeneratori == 0)
					xSemaphoreGive(mutexScope, portMAX_DELAY); // Sblocco l'accesso al task scope all'array
		xSemaphoreGive(mutexGeneratore, portMAX_DELAY);
	}

}



int main( void ){

	/*Inizializzazine Heap e Trace*/
	prvInitialiseHeap();

	vTraceInitTraceData();
	xTickTraceUserEvent = xTraceOpenLabel("tick");

	/*Inizializzazione aree di memoria condivise tra i task*/

	ampiezza = malloc(sizeof(int) * 3);
	onda= malloc(sizeof(int) * 3);
	semiperiodi = malloc(sizeof(int) * 3);

	/*Inizializzazione mutex per l'area di memoria dove sono conservati i valori delle onde*/
	
	mutexScope = xSemaphoreCreateMutex();
	mutexGeneratore= xSemaphoreCreateMutex();


	/*Assegnazione periodi e ampiezze staticamente */
		ampiezza[0] = 4;
		ampiezza[1] = 3;
		ampiezza[2] = 5;

		semiperiodi[0] = 40;
		semiperiodi[1] = 20;
		semiperiodi[2] = 60;

	int minPeriod; //minimo periodo in modo tale da assegnare il periodo al task scope 

	//mi ritorna le priorita' assegnate secondo RATE MONOTOLIC
	int* priority=rateMonotonicPriority(semiperiodi, NTASKS,&minPeriod); 

	for(int i=0;i<NTASKS;i++)
		xTaskCreate(wave_gen, "Generatore"+i, configMINIMAL_STACK_SIZE,i, priority[i], NULL);

	xTaskCreate(scope, "Scope", configMINIMAL_STACK_SIZE, minPeriod, MINPRIO+2, NULL);

	printf("Per avviare lo scope premere invio \n");
	getchar();

	vTaskStartScheduler();
	
	
	return 0;
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created.  It is also called by various parts of the
	demo application.  If heap_1.c, heap_2.c or heap_4.c is being used, then the
	size of the	heap available to pvPortMalloc() is defined by
	configTOTAL_HEAP_SIZE in FreeRTOSConfig.h, and the xPortGetFreeHeapSize()
	API function can be used to query the size of free heap space that remains
	(although it does not provide information on how the remaining heap might be
	fragmented).  See http://www.freertos.org/a00111.html for more
	information. */
	vAssertCalled( __LINE__, __FILE__ );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
	to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
	task.  It is essential that code added to this hook function never attempts
	to block in any way (for example, call xQueueReceive() with a block time
	specified, or call vTaskDelay()).  If application tasks make use of the
	vTaskDelete() API function to delete themselves then it is also important
	that vApplicationIdleHook() is permitted to return to its calling function,
	because it is the responsibility of the idle task to clean up memory
	allocated by the kernel to any task that has since deleted itself. */

	/* Uncomment the following code to allow the trace to be stopped with any
	key press.  The code is commented out by default as the kbhit() function
	interferes with the run time behaviour. */
	/*
		if( _kbhit() != pdFALSE )
		{
			if( xTraceRunning == pdTRUE )
			{
				vTraceStop();
				prvSaveTraceFile();
				xTraceRunning = pdFALSE;
			}
		}
	*/

	#if ( mainCREATE_SIMPLE_BLINKY_DEMO_ONLY != 1 )
	{
		/* Call the idle task processing used by the full demo.  The simple
		blinky demo does not use the idle task hook. */
		vFullDemoIdleFunction();
	}
	#endif
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected.  This function is
	provided as an example only as stack overflow checking does not function
	when running the FreeRTOS Windows port. */
	vAssertCalled( __LINE__, __FILE__ );
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
	/* This function will be called by each tick interrupt if
	configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
	added here, but the tick hook is called from an interrupt context, so
	code must not attempt to block, and only the interrupt safe FreeRTOS API
	functions can be used (those that end in FromISR()). */

	#if ( mainCREATE_SIMPLE_BLINKY_DEMO_ONLY != 1 )
	{
		vFullDemoTickHookFunction();
	}
	#endif /* mainCREATE_SIMPLE_BLINKY_DEMO_ONLY */

	/* Write a user event to the trace log.  Note:  This project runs under
	Windows, and Windows will not be executing the RTOS threads continuously.
	Therefore tick events will not appear with a regular interval within the the
	trace recording. */
	vTraceUserEvent( xTickTraceUserEvent );
}
/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook( void )
{
	/* This function will be called once only, when the daemon task starts to
	execute	(sometimes called the timer task).  This is useful if the
	application includes initialisation code that would benefit from executing
	after the scheduler has been started. */
}
/*-----------------------------------------------------------*/

void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
static portBASE_TYPE xPrinted = pdFALSE;
volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

	/* Called if an assertion passed to configASSERT() fails.  See
	http://www.freertos.org/a00110.html#configASSERT for more information. */

	/* Parameters are not used. */
	( void ) ulLine;
	( void ) pcFileName;

	printf( "ASSERT! Line %d, file %s, GetLastError() %d\r\n", ulLine, pcFileName, GetLastError() );

 	taskENTER_CRITICAL();
	{
		/* Stop the trace recording. */
		if( xPrinted == pdFALSE )
		{
			xPrinted = pdTRUE;
			if( xTraceRunning == pdTRUE )
			{
				vTraceStop();
				prvSaveTraceFile();
			}
		}

		/* You can step out of this function to debug the assertion by using
		the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
		value. */
		while( ulSetToNonZeroInDebuggerToContinue == 0 )
		{
			__asm{ NOP };
			__asm{ NOP };
		}
	}
	taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

static void prvSaveTraceFile( void )
{
FILE* pxOutputFile;

	fopen_s( &pxOutputFile, "Trace.dump", "wb");

	if( pxOutputFile != NULL )
	{
		fwrite( RecorderDataPtr, sizeof( RecorderDataType ), 1, pxOutputFile );
		fclose( pxOutputFile );
		printf( "\r\nTrace output saved to Trace.dump\r\n" );
	}
	else
	{
		printf( "\r\nFailed to create trace dump file\r\n" );
	}
}
/*-----------------------------------------------------------*/

static void  prvInitialiseHeap( void )
{
/* The Windows demo could create one large heap region, in which case it would
be appropriate to use heap_4.  However, purely for demonstration purposes,
heap_5 is used instead, so start by defining some heap regions.  No
initialisation is required when any other heap implementation is used.  See
http://www.freertos.org/a00111.html for more information.

The xHeapRegions structure requires the regions to be defined in start address
order, so this just creates one big array, then populates the structure with
offsets into the array - with gaps in between and messy alignment just for test
purposes. */
static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
volatile uint32_t ulAdditionalOffset = 19; /* Just to prevent 'condition is always true' warnings in configASSERT(). */
const HeapRegion_t xHeapRegions[] =
{
	/* Start address with dummy offsets						Size */
	{ ucHeap + 1,											mainREGION_1_SIZE },
	{ ucHeap + 15 + mainREGION_1_SIZE,						mainREGION_2_SIZE },
	{ ucHeap + 19 + mainREGION_1_SIZE + mainREGION_2_SIZE,	mainREGION_3_SIZE },
	{ NULL, 0 }
};

	/* Sanity check that the sizes and offsets defined actually fit into the
	array. */
	configASSERT( ( ulAdditionalOffset + mainREGION_1_SIZE + mainREGION_2_SIZE + mainREGION_3_SIZE ) < configTOTAL_HEAP_SIZE );

	/* Prevent compiler warnings when configASSERT() is not defined. */
	( void ) ulAdditionalOffset;

	vPortDefineHeapRegions( xHeapRegions );
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

	/* Pass out a pointer to the StaticTask_t structure in which the Idle task's
	state will be stored. */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

	/* Pass out the array that will be used as the Idle task's stack. */
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;

	/* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;

	/* Pass out a pointer to the StaticTask_t structure in which the Timer
	task's state will be stored. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

	/* Pass out the array that will be used as the Timer task's stack. */
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;

	/* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}


