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
#define NSEMAFOROS 7
#define TAMMC 256
#define MAXPROCESOS 127


static volatile sig_atomic_t ejecuta=1;

void limpiarIPCS();
void waitf(int,int);
void signalf(int,int);

void crearHijo();
void cicloSem();

void nPausas(int n);

void iniciarPeatones();
void iniciarCoches();
void cruce();

struct sembuf sopsEntrar,sopsSalir;
struct sigaction sa;


int sem;
char *mc =  NULL;
int memid;


/*union semun {
	int val;
    	struct semid_ds *buf;
    	ushort_t *array;
};*/


 int main(int argc, char *argv[]) {
 	pid_t PPADRE = getpid();
 	int i;
 	
 	//MANEJADORAS CTRL+C
 	sa.sa_handler = &limpiarIPCS;
    	if (sigemptyset(&sa.sa_mask) != 0) return 0;
    	if (sigaddset(&sa.sa_mask, SIGINT) != 0) return 0;
    	sa.sa_flags = 0;
    	if (sigaction(SIGINT, &sa, NULL) != 0) return 0;
       
       
	//1. COGER Y VERIFICAR INFO DE ARGUMENTOS
	if(argc < 3 || argc > 3) { exit(-1); } 
	
	if(atoi(argv[1]) > MAXPROCESOS) { perror("ERROR: Demasiados procesos, desbordamiento"); exit(errno); } 
	
	int nproc = atoi(argv[1]);
	int velocidad = atoi(argv[2]);
	
	//2. INICIALIZAR VARIABLES, MECANISMOS IPC, HANDLERS DE SENALES...
	
	if((sem = semget(IPC_PRIVATE, NSEMAFOROS, IPC_CREAT | 0600)) == -1) { perror("Error semget"); exit(errno); }; //Iniciar semaforo
	

	if((memid = shmget(IPC_PRIVATE, TAMMC, IPC_CREAT | 0600)) == -1) { perror("Error memid"); exit(errno); }; //Creacion de memoria compartida (Min: 256 bytes)
	
	mc = shmat(memid, NULL, 0); //Asociamos el puntero que devuelve shmat a una variable 

	//3. LLAMAR A CRUCE_inicio
	CRUCE_inicio(velocidad, nproc, sem, mc);

	//4. CREAR PROCESO GESTOR DE SEMAFOROS
	if(getpid() == PPADRE){
		//crearHijo();

		if(getpid() != PPADRE){
			cicloSem();
		}
		
	
	}

	if(semctl(sem, 5, SETVAL, nproc) == -1) { perror("Error semctl"); exit(errno); } //Operaciones del semaforo: Asigna nprocs de valor al semaforo 0
	if(semctl(sem, 6, SETVAL, 1) == -1) { perror("Error semctl"); exit(errno); } 
	
	/*int num=semctl(sem,5,GETVAL);
	printf("VALOR DEL SEMAFORO %d\n",num);*/
	
	//5.- BUBLE INFINITO SALE CON CTRL+C
	while(ejecuta) {
		if(getpid() == PPADRE) {
			CRUCE_nuevo_proceso();
			crearHijo();
		
		}else{
			signal(SIGINT, SIG_IGN);
			waitf(5,1);
			iniciarCoches();
			break;
					
		}
			
		
	}			
	
	/*switch(tipo) {
		case 0: iniciarCoches();
			break;

		case 1:iniciarPeatones();
			break;

		default: perror("ERROR SWITCH"); exit(errno);
	}*/
	
	return 0;
	
 	

 } 
 void cicloSem(){
 	while(1){
		 //Para comprobar si hay un proceso en la zona critica del cruce hay que comprobar el valor del semaforo
 		if (semctl(sem, 4, SETVAL, 1) == -1) { perror("Error semctl"); exit(errno); }
		 
 		waitf(4,1);
		

		CRUCE_pon_semAforo(0,2);//SEM_C1 A VERDE
		//signalf(0,1);
		CRUCE_pon_semAforo(1,1);//SEM_C2 A ROJO
		//waitf(1,1);
		CRUCE_pon_semAforo(2,1);//SEM_P1 A ROJO
		//waitf(2,1);
		CRUCE_pon_semAforo(3,2);//SEM_P2 A VERDE
		//signalf(3,1);
		
		signalf(4,1);
		nPausas(6);

		//SEM_C1 A AMARILLO
		if (semctl(sem, 4, SETVAL, 1) == -1) { perror("Error semctl"); exit(errno); }

		cruce(0,3);
		
		waitf(4,1);

		CRUCE_pon_semAforo(0,1);//SEM_C1 A ROJO
		//waitf(0,1);
		CRUCE_pon_semAforo(1,2);//SEM_C2 A VERDE
		//signalf(1,1);
		CRUCE_pon_semAforo(2,1);//SEM_P1 A ROJO
		//waitf(2,1);
		CRUCE_pon_semAforo(3,1);//SEM_P2 A ROJO		
		//waitf(3,1);	
		

		nPausas(9);

		signalf(4,1);
		
		//SEM_C2 A AMARILLO
		cruce(1,3);


		nPausas(2);

		CRUCE_pon_semAforo(0,1);//SEM_C1 A ROJO
		//waitf(0,1);
		CRUCE_pon_semAforo(1,1);//SEM_C2 A ROJO
		//waitf(1,1);
		CRUCE_pon_semAforo(2,2);//SEM_P1 A VERDE
		//signalf(2,1);
		CRUCE_pon_semAforo(3,1);//SEM_P2 A ROJO
		//waitf(3,1);

		nPausas(12);
		
			
	}
}

void cruce(int sem,int color) {
	CRUCE_pon_semAforo(sem,color);//SEM A AMARILLO
	pausa();
	pausa();
}

//Mientras haya cambio de semaforos bloqueamos la entrada al cruce de todos los procesos

 void crearHijo(){
 	pid_t pid = fork();
 	
 	switch(pid) {
 		case -1:
 			system("clear");
 			perror("Error creando hijos");
 			exit(errno);
 	}
 } 

void nPausas(int n) {
	int j;
	for(j = 0; j < n; j++){
		pausa();
	}
}


void limpiarIPCS() {
	int PID;
	while(PID = wait(NULL)){
		if(errno == ECHILD){
			break;
		}
	}
	shmdt(&mc);//Desasociacion
	if(shmctl(memid,IPC_RMID,NULL)==-1){
		perror("Error Liberar Memoria Compartida");
		exit(errno);
	}
	if(sem!=-1){
		if(semctl(sem, 0, IPC_RMID) == -1){
		 	perror("Error semctl"); 
			exit(errno);
		}
	}
	CRUCE_fin();
	ejecuta=0;
	
		
}

void waitf(int numsem,int numwait) {
	sopsEntrar.sem_num = numsem;
	sopsEntrar.sem_op = -numwait; //Wait
	sopsEntrar.sem_flg = 0;
	if(semop(sem, &sopsEntrar, 1) == -1){
		perror("Error semop");
		exit(errno);
	}
}
void signalf(int numsem,int numsignal) {
	sopsSalir.sem_num = numsem;
	sopsSalir.sem_op = numsignal; //signal
	sopsSalir.sem_flg = 0;
	if(semop(sem, &sopsSalir, 1) == -1) {
		perror("Error semop");
		exit(errno);
	}
	
}

void iniciarCoches() {
	struct posiciOn posSig, posSigSig, posPrueba;
	posSigSig.x=0;
	posSigSig.y=0;
	
	posSig = CRUCE_inicio_coche();

	posSigSig = CRUCE_avanzar_coche(posSig);
	
	while(posSigSig.y >= 0) {
		//fprintf(stderr, "%d %d\t", posSigSig.x, posSigSig.y);
		posSigSig = CRUCE_avanzar_coche(posSigSig);

		pausa_coche();
	}
	
	CRUCE_fin_coche();
	signalf(5,1);
	exit(0);
	

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
		
		//fprintf(stderr, "%d %d\t", posSigSig.x, posSigSig.y);
		posSigSig = CRUCE_avanzar_peatOn(posSigSig);

		pausa();
	}
	CRUCE_fin_peatOn();
	

}

