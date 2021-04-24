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
struct sigaction manejadora;

void sig_action(int signal);
void waitf(int,int);
void signalf(int,int);

void crearHijo();
void cicloSem();

void nPausas(int n);

void iniciarPeatones();
void iniciarCoches();
void cruce();

//PRUEBA DE PUSH

 int main(int argc, char *argv[]) {
 	pid_t PPADRE = getpid();
 	pid_t HIJOCICLOS;
 	int i;
 	
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
	
	//CICLO SEMAFORICO

	/*if(getpid() == PPADRE){
		crearHijo();

		if(getpid() != PPADRE){
			cicloSem();
		}
	
	}*/

	if(semctl(sem, 0, SETVAL, nproc) == -1) { printf("Error semctl\n"); } //Operaciones del semaforo: Asigna nprocs de valor al semaforo 0
	
	while(1) {
		if(getpid() == PPADRE) {
			int tipo;
			
			tipo = CRUCE_nuevo_proceso();
			
			waitf(0,1);
			signalf(0,1);

			if(semop(sem, &sopsEntrar, 1) == -1){
				printf("Error semop\n");
			}
			switch(tipo) {
				case 0: iniciarCoches();
				break;

				case 1:iniciarPeatones();
				break;

				default: perror("ERROR SWITCH"); exit(-2);
			}

			if(semop(sem, &sopsSalir, 1) == -1) {
				printf("Error semop\n");
			}	
		}
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

		nPausas(2);
		
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
		CRUCE_fin();
		//system("clear");
		exit(0);
	}
}

void waitf(int numsem,int numwait) {
	sopsEntrar.sem_num = numsem;
	sopsEntrar.sem_op = -numwait; //Wait
	sopsEntrar.sem_flg = 0;
}
void signalf(int numsem,int numsignal) {
	sopsSalir.sem_num = numsem;
	sopsSalir.sem_op = numsignal; //signal
	sopsSalir.sem_flg = 0;
}

void iniciarCoches() {
	struct posiciOn posSig, posSigSig, posPrueba;

	posSigSig.x=0;
	posSigSig.y=0;
	
	posSig = CRUCE_inicio_coche();

	posSigSig = CRUCE_avanzar_coche(posSig);

	while(posSigSig.y >= 0) {
		
		fprintf(stderr, "%d %d\t", posSigSig.x, posSigSig.y);
		posSigSig = CRUCE_avanzar_coche(posSigSig);

		pausa_coche();
	}
	CRUCE_fin_coche();
	

}

void iniciarPeatones() {
	struct posiciOn posSig, posSigSig, posNac;
	
	posNac.x=0;
	posNac.y=0;
	
	posSigSig.x=0;
	posSigSig.y=0;
	
	posSig = CRUCE_inicio_peatOn_ext(&posNac);

	posSigSig = CRUCE_avanzar_peatOn(posSig);

	while(posSigSig.y >= 0) {
		
		fprintf(stderr, "%d %d\t", posSigSig.x, posSigSig.y);
		posSigSig = CRUCE_avanzar_peatOn(posSigSig);

		pausa();
	}
	CRUCE_fin_peatOn();
	

}
