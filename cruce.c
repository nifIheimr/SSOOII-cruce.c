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
#define NSEMAFOROS 5
#define TAMMC 256
#define MAXPROCESOS 127

int sem;
char *mc =  NULL;
int memid;
struct sembuf sopsEntrar,sopsSalir;

void sig_action (int signal) {
	if(signal == SIGINT) {
		printf("\nInterrupción\n");
		
		shmdt(&mc);//Desasociacion
		if(shmctl(memid,IPC_RMID,NULL)==-1){
			fprintf(stderr,"Error Liberar Memoria Compartida");
		}
		if(semctl(sem, 0, IPC_RMID) == -1){
	 		fprintf(stderr,"Error semctl\n"); 
		}
		exit(0);
	}
}

void waitf(int numsem,int numwait){
	sopsEntrar.sem_num = numsem;
	sopsEntrar.sem_op = -numwait; //Wait
	sopsEntrar.sem_flg = 0;
}
void signalf(int numsem,int numsignal){
	sopsSalir.sem_num = numsem;
	sopsSalir.sem_op = numsignal; //signal
	sopsSalir.sem_flg = 0;
}

void crearHijo();
void cicloSem();
void cruce();
void nPausas(int n);

//PRUEBA DE PUSH

 int main(int argc, char *argv[]) {
 	pid_t PPADRE = getpid();
 	pid_t HIJOCICLOS;
 	int i;
 	struct sigaction manejadora;
	struct sembuf sops[10];
 	
 	//CUANDO SE RECIBA UNA SIGINT SE FINALIZARA TODO DE FORMA ORDENADA
 	//Liberamos IPCS
 	manejadora.sa_handler = sig_action;
    if (sigemptyset(&manejadora.sa_mask) != 0) return 0;
    if (sigaddset(&manejadora.sa_mask, SIGINT) != 0) return 0;
    manejadora.sa_flags = 0;
    if (sigaction(SIGINT, &manejadora, NULL) != 0) return 0;
       
       
	//1. COGER Y VERIFICAR INFO DE ARGUMENTOS
	if(argc < 3 || argc > 3) { exit(-1); } 
	//if(argv[1] > MAXPROC) { exit(-2); } 

	if(atoi(argv[1]) > MAXPROCESOS) { perror("ERROR: Demasiados procesos, desbordamiento"); exit(-1); } 
	
	int nproc = atoi(argv[1]);
	int velocidad = atoi(argv[2]);
	//2. INICIALIZAR VARIABLES, MECANISMOS IPC, HANDLERS DE SENALES...
	
	if((sem = semget(IPC_PRIVATE, NSEMAFOROS, IPC_CREAT | 0600)) == -1) { printf("Error semget\n"); }; //Iniciar semaforo
	//1 = numero de procesos que entran a la vez

	//if (semctl(sem, 1, SETVAL, 0) == -1) { printf("Error semctl\n"); }
	 

	if((memid = shmget(IPC_PRIVATE, TAMMC, IPC_CREAT | 0600)) == -1) { printf("Error memid\n"); exit (-3); }; //Creacion de memoria compartida (Min: 256 bytes)
	
	mc = shmat(memid, NULL, 0); //Asociamos el puntero que devuelve shmat a una variable 

	//3. LLAMAR A CRUCE_inicio
	CRUCE_inicio(velocidad, nproc, sem, mc);

	//4. CREAR PROCESO GESTOR DE SEMAFOROS
	
	struct posiciOn posicionsig,posnac,posicionsigsig,posprueba;

	posnac.x=0;
	posnac.y=0;

	posicionsigsig.x=0;
	posicionsigsig.y=0;
	
	posprueba.x=0;
	posprueba.y=0;
	
	//CICLO SEMAFORICO

	if(getpid()==PPADRE){
		crearHijo();
		if(getpid()!=PPADRE){
			cicloSem();
		
		}
	
	}

	if (semctl(sem, 0, SETVAL,nproc) == -1) { printf("Error semctl\n"); } //Operaciones del semaforo: Asigna nprocs de valor al semaforo 0
	while(1){
		if(getpid() == PPADRE) {
			int tipo = CRUCE_nuevo_proceso();
			/*int valor=semctl(sem, 0, GETVAL);
			printf("Valor del Semaforo: %d",valor);*/
			
			waitf(0,1);
			signalf(0,1);
			if(semop(sem,&sopsEntrar,1) == -1){
				printf("Error semop\n");
			}		
			sleep(2);
			
			posicionsig=CRUCE_inicio_coche();
			sleep(3);
			//fprintf(stderr, "%d %d\t",posicionsig.x,posicionsig.y);
			posicionsigsig=CRUCE_avanzar_coche(posicionsig);
			fprintf(stderr, "%d %d\t", posicionsigsig.x, posicionsigsig.y);
			posicionsigsig=CRUCE_avanzar_coche(posicionsigsig);

			/*while(posicionsigsig.y>=0){
				//semaforo para entrar a la siguiente posicion
				posicionsigsig = CRUCE_avanzar_coche(posicionsigsig);
				printf("%d %d\t", posicionsigsig.x, posicionsigsig.y);
			}*/
			/*posicionsig = CRUCE_inicio_peatOn_ext(&posnac);
			printf("%d %d\t", posnac.x, posnac.y);
			sleep(3);
			printf("%d %d\t", posicionsig.x, posicionsig.y);
			posicionsigsig = CRUCE_avanzar_peatOn(posicionsig);
			printf("%d %d\t", posicionsigsig.x, posicionsigsig.y);
			while(posicionsigsig.y>=0){
				//semaforo para entrar a la siguiente posicion
				posicionsigsig = CRUCE_avanzar_peatOn(posprueba);
				printf("%d %d\t", posicionsigsig.x, posicionsigsig.y);
			}
			*/
			sleep(3);
			//CRUCE_fin_peatOn();
			if(semop(sem,&sopsSalir,1) == -1){
				printf("Error semop\n");
			}
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
	}

	

	//5. ENTRA EN EL BUCLE INFINITO DEL QUE SOLO SE SALDRA CON UNA INTERRUPCION
	while(1) {

		//5.1
		//5.2
		//5.3
	}
		
	if(getpid() == PPADRE) {
		CRUCE_fin();
	} 
		
	
 	

 } 
 void cicloSem(){
 	while(1){
		 //Para comprobar si hay un proceso en la zona critica del cruce hay que comprobar el valor del semaforo
 		if (semctl(sem, 4, SETVAL, 0) == -1) { printf("Error semctl\n"); }
		 
 		waitf(4,1);

		CRUCE_pon_semAforo(0,2);//SEM_C1 A VERDE
		CRUCE_pon_semAforo(3,2);//SEM_P2 A VERDE
		CRUCE_pon_semAforo(1,1);//SEM_C2 A ROJO
		CRUCE_pon_semAforo(2,1);//SEM_P1 A ROJO

		signalf(4,1);

		nPausas(6);


		//SEM_C1 A AMARILLO
		if (semctl(sem, 4, SETVAL, 0) == -1) { printf("Error semctl\n"); }

		cruce();
		
		waitf(4,1);

		CRUCE_pon_semAforo(1,2);//SEM_C2 A VERDE
		CRUCE_pon_semAforo(0,1);//SEM_C1 A ROJO
		CRUCE_pon_semAforo(3,1);//SEM_P2 A ROJO			
		CRUCE_pon_semAforo(2,1);//SEM_P1 A ROJO

		nPausas(9);

		signalf(4,1);

		//SEM_C2 A AMARILLO
		CRUCE_pon_semAforo(1,3);//SEM_C2 A AMARILLO
		pausa();
		pausa();
		
		CRUCE_pon_semAforo(2,2);//SEM_P1 A VERDE
		CRUCE_pon_semAforo(1,1);//SEM_C2 A ROJO
		CRUCE_pon_semAforo(0,1);//SEM_C1 A ROJO
		CRUCE_pon_semAforo(3,1);//SEM_P2 A ROJO

		nPausas(12);
		
			
	}
}

void cruce() {
	CRUCE_pon_semAforo(0,3);//SEM_C1 A AMARILLO
	pausa();
	pausa();
}

//Mientras haya cambio de semaforos bloqueamos la entrada al cruce de todos los procesos

 void crearHijo(){
 	pid_t pid = fork();
 	
 	switch(pid) {
 		case -1:
 			system("clear");
 			perror("Error creando hijos\n");
 			exit(-1);
 	}
 } 

void nPausas(int n) {
	for(int j = 0; j < n; j++){
			pausa();
		}
}