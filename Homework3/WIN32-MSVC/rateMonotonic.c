#include "rateMonotonic.h"

#define DEBUG 1

int * rateMonotonicPriority(int *periodi, int nPeriodi,int *minPeriod) {

	int *prio = malloc(sizeof(int)*nPeriodi);
	int ordinato = 0;
	int n = nPeriodi;

	periodiID *periodiOrdinati = malloc(sizeof(periodiID)*nPeriodi);
		for (int i = 0; i < nPeriodi; i++) {
			periodiOrdinati[i].periodo = periodi[i];
			periodiOrdinati[i].id = i;
		}

	while (n > 1 && ordinato == 0) {
		ordinato = 1;
		for (int i = 0; i < n - 1; i++) {
			if (periodiOrdinati[i].periodo > periodiOrdinati[i + 1].periodo) {
				periodiID p = periodiOrdinati[i];
				periodiOrdinati[i] = periodiOrdinati[i + 1];
				periodiOrdinati[i + 1] = p;
				ordinato = 0;
			}
		}
		n--;
	}

//Assegno le priorita' in base ai periodi 

	for (int i = 0; i < nPeriodi; i++) {
		int j = i - 1;

		//controllo se ci sono periodi dupicati in modo tale da assegnare la stessa priorita'

		while ((periodiOrdinati[j].periodo != periodiOrdinati[i].periodo) && (j >= 0))
				j--;

		if (periodiOrdinati[j].periodo == periodiOrdinati[i].periodo)
				prio[periodiOrdinati[i].id] = prio[periodiOrdinati[j].id];
		else {
				if (MAXPRIO - 1 > MINPRIO)
					prio[periodiOrdinati[i].id] = MAXPRIO - i;
				else
					prio[periodiOrdinati[i].id] = MINPRIO;
		}
	}
	
#if( DEBUG==1)
	printf("[RMSCHEDULER]-->Task ordinati per periodo e priority \n***********\n");
	for (int i = 0; i < nPeriodi; i++) {
		printf("%d) Periodo : %d\tNumeroTask--->%d \n", i, periodiOrdinati[i].periodo, periodiOrdinati[i].id);
		printf("\tPriority --->%d\n***********\n", prio[periodiOrdinati[i].id]);
	}

	printf("\n\n[RMSCHEDULER]-->Task non ordinati \n***********\n");
	for (int i = 0; i < nPeriodi; i++) {
		printf("NumeroTask: %d\tPeriodo: %d\n", i, periodi[i]);
		printf("\tPriority --->%d\n***********\n", prio[i]);

	}
#endif

	*minPeriod = periodiOrdinati[0].periodo;
	return prio;
}
