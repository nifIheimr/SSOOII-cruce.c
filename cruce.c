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

void sig_action (int signal) {
	if(signal == SIGINT) {
		printf("\nInterrupción\n");
		int retorno;

		//Esperar al hijo
		wait(&retorno);

		//Borrar semáforos

		if(semctl(sem, 0, IPC_RMID) == -1) { printf("Error semctl\n"); }

		exit(0);
	}
}

void crearHijo();

//PRUEBA DE PUSH

 int main(int argc, char *argv[]) {
 	pid_t PPADRE = getpid();
 	int i;
 	char *mc =  NULL;
 	int memid;
 	
	//1. COGER Y VERIFICAR INFO DE ARGUMENTOS
	if(argc < 3 || argc > 3) { exit(-1); } 
	//if(argv[1] > MAXPROC) { exit(-2); } 
	
	int nproc = atoi(argv[1]);
	int velocidad = atoi(argv[2]);
	//2. INICIALIZAR VARIABLES, MECANISMOS IPC, HANDLERS DE SENALES...
	

	if((sem = semget(IPC_PRIVATE, 4, IPC_CREAT | 0600)) == -1) { printf("Error semget\n"); };
	
	//1 = numero de procesos que entran a la vez
	if (semctl(sem, 0, SETVAL, 1) == -1) { printf("Error semctl\n"); }
	
	struct sembuf sopsEntrar, sopsSalir;

	sopsEntrar.sem_num = 0;
	sopsEntrar.sem_op = -1; //Wait
	sopsEntrar.sem_flg = 0;

	sopsSalir.sem_num = 0;
	sopsSalir.sem_op = 1; //Signal
	sopsSalir.sem_flg = 0;
	 
	 
	if((memid = shmget(IPC_PRIVATE, TAMMC, IPC_CREAT | 0600)) == -1) { printf("Error memid\n"); exit (-3); };
	
	mc = shmat(memid, NULL, 0);
	//3. LLAMAR A CRUCE_inicio
	int ret = CRUCE_inicio(velocidad, nproc, sem, mc);
	//4. CREAR PROCESO GESTOR DE SEMAFOROS
	
	for(i = 0; i < nproc; i++) {
		if(getpid() == PPADRE) {
			crearHijo();
		}
	}
	//5. ENTRA EN EL BUCLE INFINITO DEL QUE SOLO SE SALDRA CON UNA INTERRUPCION
	while(1) {
		//5.1
		//5.2
		//5.3
	}
		
		
	//6. CUANDO SE RECIBA UNA SIGINT SE FINALIZARA TODO DE FORMA ORDENADA
 
 } 
 

 void crearHijo() {
 	pid_t pid = fork();
 	
 	switch(pid) {
 		case -1:
 			system("clear");
 			perror("Error creando hijos\n");
 			exit(-1);
 	}
 } 
