#!/bin/bash

RTAIMODULE="/usr/realtime/modules"

if [ $# -eq 0 ] ; then
	echo ""	
	echo "[Module Loader] ==> Nessun parametro"
	echo "		1) Inserisci moduli RTAI"
	echo "		2) Rimuovi moduli RTAI"
	echo ""

	choice=-1
		read choice 

else
	choice=$1
fi
	if [ $choice -eq 1 ] ; then
			sync
     		sudo insmod $RTAIMODULE/rtai_hal.ko
			sync
     		sudo insmod $RTAIMODULE/rtai_sched.ko
			sync
     		sudo insmod $RTAIMODULE/rtai_shm.ko 
			sync
     		sudo insmod $RTAIMODULE/rtai_sem.ko
			
			if [ $? -eq 0 ] ;then 

				echo "Inserisci i periodi"
					read in[0] in[1] in[2]
			
				sync
				sudo insmod rt_process.ko semiperiodi=${in[0]},${in[1]},${in[2]}

				echo "Inserisci le ampiezze"
					read in[0] in[1] in[2]

				./set_a ${in[0]} ${in[1]} ${in[2]}
		
				echo;
				echo "[Module Loader] ==> Moduli correttamente inseriti"
			
			else 
					echo;
					echo "[Module Loader] ==> Errore moduli gia' presenti' "

		fi
	else 
			sync
			sudo rmmod rt_process
			sync
    		sudo rmmod $RTAIMODULE/rtai_sem.ko
			sync
     		sudo rmmod $RTAIMODULE/rtai_shm.ko
			sync
     		sudo rmmod $RTAIMODULE/rtai_sched.ko
			sync
     		sudo rmmod $RTAIMODULE/rtai_hal.ko
			sync

			if [ $? -eq 0 ] ;then 

				echo;
				echo "[Module Loader] ==> Moduli correttamente rimossi"
		
			else 

				echo;
				echo "[Module Loader] ==> Moduli gia' rimossi"

			fi

	fi

