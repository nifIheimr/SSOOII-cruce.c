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
#define NSEMAFOROS 4
#define TAMMC 256

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
//PRUEBA DE PUSH

 int main(int argc, char *argv[]) {
 	pid_t PPADRE = getpid();
 	pid_t HIJOCICLOS;
 	int i;
 	struct sigaction manejadora;
 	
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
	
	int nproc = atoi(argv[1]);
	int velocidad = atoi(argv[2]);
	//2. INICIALIZAR VARIABLES, MECANISMOS IPC, HANDLERS DE SENALES...
	
	if((sem = semget(IPC_PRIVATE, NSEMAFOROS, IPC_CREAT | 0600)) == -1) { printf("Error semget\n"); }; //Iniciar semaforo
	//1 = numero de procesos que entran a la vez

	if (semctl(sem, 0, SETVAL,nproc) == -1) { printf("Error semctl\n"); } //Operaciones del semaforo: Asigna el valor 1 al semaforo 1 del array de semaforos
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
	
	while(1){
		if(getpid() == PPADRE) {
			
			int tipo = CRUCE_nuevo_proceso();
			
			waitf(0,1);
			signalf(0,1);
			if(semop(sem,&sopsEntrar,1) == -1){
				printf("Error semop\n");
			}
			
			int valor=semctl(sem, 0, GETVAL);
			printf("Valor del Semaforo: %d\n",valor);//no sale el valor correcto cuando se ejecuta la funcion CRUCE_INICIO¿¿??

				sleep(2);
				posicionsig = CRUCE_inicio_peatOn_ext(&posnac);

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

			sleep(3);
			CRUCE_fin_peatOn();
			
			
			

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
		CRUCE_pon_semAforo(0,2);//SEM_C1 A VERDE
		CRUCE_pon_semAforo(3,2);//SEM_P2 A VERDE
		CRUCE_pon_semAforo(1,1);//SEM_C2 A ROJO
		CRUCE_pon_semAforo(2,1);//SEM_P1 A ROJO
		for(int j=0;j<6;j++){
			pausa();
		}
		//SEM_C1 A AMARILLO
		CRUCE_pon_semAforo(0,3);//SEM_C1 A AMARILLO
		pausa();
		pausa();
		
		CRUCE_pon_semAforo(1,2);//SEM_C2 A VERDE
		CRUCE_pon_semAforo(0,1);//SEM_C1 A ROJO
		CRUCE_pon_semAforo(3,1);//SEM_P2 A ROJO			
		CRUCE_pon_semAforo(2,1);//SEM_P1 A ROJO
		for(int j=0;j<9;j++){
			pausa();
		}
		//SEM_C2 A AMARILLO
		CRUCE_pon_semAforo(1,3);//SEM_C2 A AMARILLO
		pausa();
		pausa();
		
		CRUCE_pon_semAforo(2,2);//SEM_P1 A VERDE
		CRUCE_pon_semAforo(1,1);//SEM_C2 A ROJO
		CRUCE_pon_semAforo(0,1);//SEM_C1 A ROJO
		CRUCE_pon_semAforo(3,1);//SEM_P2 A ROJO
		for(int j=0;j<12;j++){
			pausa();
		}	
	}
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
