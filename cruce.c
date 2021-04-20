#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/sem.h>
#include <string.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "cruce.h"


#define TRUE    1
#define FALSE   0
#define MAXPROC 10
#define NSEMAFOROS 4
#define TAMMC 256

int sem;
char *mc =  NULL;
int memid;
struct sembuf sops[4];

void sig_action (int signal) {
	if(signal == SIGINT) {
		printf("\nInterrupción\n");
		
		shmdt(&mc);//Desasociacion
		if(shmctl(memid,IPC_RMID,NULL)==-1){
			fprintf(stderr,"Error Liberar Memoria Compartida");
		}
		if(semctl(sem, 0, IPC_RMID) == -1){
	 		fprintf(stsderr,"Error semctl\n"); 
		}
		exit(0);
	}
}

void waitf(int numsem,int numwait){
	sops[numsem].sem_num = 0;
	sops[numsem].sem_op = -numwait; //Wait
	sops[numsem].sem_flg = 0;
}
void signalf(int numsem,int numsignal){
	sops[numsem].sem_num = 0;
	sops[numsem].sem_op = numsignal; //signal
	sops[numsem].sem_flg = 0;
}

 void crearHijo();

//PRUEBA DE PUSH

 int main(int argc, char *argv[]) {
 	pid_t PPADRE = getpid();
 	pid_t HIJOCICLOS;
 	int i;
 	
 	
	//1. COGER Y VERIFICAR INFO DE ARGUMENTOS
	if(argc < 3 || argc > 3) { exit(-1); } 
	//if(argv[1] > MAXPROC) { exit(-2); } 
	
	int nproc = atoi(argv[1]);
	int velocidad = atoi(argv[2]);
	//2. INICIALIZAR VARIABLES, MECANISMOS IPC, HANDLERS DE SENALES...
	
	if((sem = semget(IPC_PRIVATE, 4, IPC_CREAT | 0600)) == -1) { printf("Error semget\n"); }; //Iniciar semaforo
	//1 = numero de procesos que entran a la vez

	if (semctl(sem, 0, SETVAL, 1) == -1) { printf("Error semctl\n"); } //Operaciones del semaforo: Asigna el valor 1 al semaforo 1 del array de semaforos
	//if (semctl(sem, 1, SETVAL, 0) == -1) { printf("Error semctl\n"); }
	 
	
	if((memid = shmget(IPC_PRIVATE, TAMMC, IPC_CREAT | 0600)) == -1) { printf("Error memid\n"); exit (-3); }; //Creacion de memoria compartida (Min: 256 bytes)
	
	mc = shmat(memid, NULL, 0); //Asociamos el puntero que devuelve shmat a una variable 

	//3. LLAMAR A CRUCE_inicio
	CRUCE_inicio(velocidad, nproc, sem, mc);

	//4. CREAR PROCESO GESTOR DE SEMAFOROS
	
	struct posiciOn posicionsig,posnac,posicionsigsig;

	posnac.x=0;
	posnac.y=0;

	posicionsigsig.x=0;
	posicionsigsig.y=0;
	
	//CICLO SEMAFORICO

	/*if(getpid()==PPADRE){
		crearHijo();
		while(1){
			CRUCE_pon_semAforo(0,2);//SEM_C1 A VERDE
			CRUCE_pon_semAforo(3,2);//SEM_P2 A VERDE
			CRUCE_pon_semAforo(1,1);//SEM_C2 A ROJO
			CRUCE_pon_semAforo(2,1);//SEM_P1 A ROJO
			for(int j=0;j<6;j++){
				pausa();
			}
			
		}
	
	}
	*/

	//while(1){
		if(getpid() == PPADRE) {
			int tipo = CRUCE_nuevo_proceso();
			
			waitf(0,1);
			if(semop(sem,sops,1) == -1){
				printf("Error semop\n");
			}

		
			signalf(0,1);

			if(semop(sem,sops,1) == -1){
				printf("Error semop\n");
			}
			
			while(posicionsigsig.y>=0){
				sleep(2);
				posicionsig = CRUCE_inicio_peatOn_ext(&posnac);

				printf("%d %d\t", posnac.x, posnac.y);
				sleep(3);
				printf("%d %d\t", posicionsig.x, posicionsig.y);

				//semaforo para entrar a la siguiente posicion
				posicionsigsig = CRUCE_avanzar_peatOn(posicionsig);

				printf("%d %d\t", posicionsigsig.x, posicionsigsig.y);
			}

			sleep(3);
			CRUCE_fin_peatOn();
			
			
			
			/*while(posicionsigsig.y>=0){
				posicionsigsig=CRUCE_avanzar_peatOn(posicionsig);
				printf("%d %d\t",posicionsigsig.x,posicionsigsig.y);
				pausa();
			}*/
			/*if(tipo==0){
				posicionsig=CRUCE_inicio_coche();
				printf("%d %d\n",posicionsig.x,posicionsig.y);
				CRUCE_avanzar_coche(posicionsig);
				
			}else{
				printf("%d %d\t",posnac.x,posnac.y);
				posicionsig=CRUCE_inicio_peatOn_ext(&posnac);
				printf("%d %d\t",posnac.x,posnac.y);
				printf("%d %d\t",posicionsig.x,posicionsig.y);
				posnac=CRUCE_avanzar_peatOn(posicionsig);
				printf("%d %d\n",posnac.x,posnac.y);
			}
			*/
			
		}
	//}
	
	
	signal(SIGINT,sig_action);


	//5. ENTRA EN EL BUCLE INFINITO DEL QUE SOLO SE SALDRA CON UNA INTERRUPCION
	while(1) {

		//5.1
		//5.2
		//5.3
	}
		
	if(getpid() == PPADRE) {
		CRUCE_fin();
	} 
		
	//6. CUANDO SE RECIBA UNA SIGINT SE FINALIZARA TODO DE FORMA ORDENADA
 	//Liberamos IPCS

 } 
 

 void crearHijo(){
 	pid_t pid = fork();
 	
 	switch(pid) {
 		case -1:
 			system("clear");
 			perror("Error creando hijos\n");
 			exit(-1);
 	}
 } 
