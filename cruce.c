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
#define NSEMAFOROS 15
#define TAMMC 256
#define MAXPROCESOS 127
#define LONGITUD_MAXIMA_MSJ 80

static volatile sig_atomic_t ejecuta = 1;

void limpiarIPCS();
void waitf(int,int);
void signalf(int,int);

int crearHijo();
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





 int main(int argc, char *argv[]) {
 	pid_t PPADRE = getpid();
 	int i,tipo;
	pid_t pid;
 	
	key_t clave;
	int buzon, envio, recibo;
	char etiqueta[100];
	struct passwd *informe;

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

	
	if(semctl(sem, 1, SETVAL, nproc) == -1) { perror("Error semctl"); exit(errno); } //Control de Max Proc

	if(semctl(sem, 2, SETVAL, 1) == -1) { perror("Error semctl");}//Comprobar Coches
	
	if(semctl(sem, 3, SETVAL, 1) == -1) { perror("Error semctl");}//Iniciar Mov Coche
	
	if(semctl(sem, 4, SETVAL, 1) == -1) { perror("Error semctl");}//Mover Pos Sig Coche
	
	if(semctl(sem, 5, SETVAL, 1) == -1) { perror("Error semctl");}//Bucle de coches
	
	if(semctl(sem, 6, SETVAL, 1) == -1) { perror("Error semctl");}//Bucle de coches


	if(semctl(sem, 7, SETVAL, 0) == -1) { perror("Error semctl"); exit(errno); } //Semaforo C1

	if(semctl(sem, 8, SETVAL, 0) == -1) { perror("Error semctl"); exit(errno); } //Semaforo C2


	if(semctl(sem, 9, SETVAL, 0) == -1) { perror("Error semctl"); exit(errno); } //Semaforo P1
	
	if(semctl(sem, 10, SETVAL, 0) == -1) { perror("Error semctl"); exit(errno); } //Semaforo P2
	
	if(semctl(sem, 11, SETVAL, 1) == -1) { perror("Error semctl"); exit(errno); } //Inicio Peaton
	
	if(semctl(sem, 12, SETVAL, 1) == -1) { perror("Error semctl"); exit(errno); } //MOver Pos Sig Peaton
	
	if(semctl(sem, 13, SETVAL, 1) == -1) { perror("Error semctl"); exit(errno); } //BUcle Peaton
	
	if((buzon = msgget(clave, IPC_CREAT | 0666) == -1)) { perror("No se puede crear/encontrar el buzon"); }
	
	//3. LLAMAR A CRUCE_inicio
	CRUCE_inicio(velocidad, nproc, sem, mc);

	//4. CREAR PROCESO GESTOR DE SEMAFOROS
	if(getpid() == PPADRE){
		crearHijo();
		if(getpid() != PPADRE){
			cicloSem();
		}
	}

	

	/*int num=semctl(sem,5,GETVAL);
	printf("VALOR DEL SEMAFORO %d\n",num);
	*/
	//5.- BUBLE INFINITO SALE CON CTRL+C
	
	while(ejecuta) {
		if(getpid()==PPADRE){
		
			waitf(1,1);
			
			 tipo=CRUCE_nuevo_proceso();
			
			pid_t pid = fork();

	 		switch(pid) {
				case -1:
					system("clear");
					perror("Error creando hijos");
					exit(1);
				case 0:
					signal(SIGINT, SIG_IGN);
					
				default:
					continue;
					
				
	 		}
	 	} else if(getpid()!=PPADRE){
	 		
	 		switch(tipo) {
				case 0: iniciarCoches();
					break;

				case 1:iniciarPeatones();
					break;

				default: perror("ERROR SWITCH"); exit(errno);
			
			}
 			//exit(0); //HACER QUE EL PROCESO TERMINE O NO??!!
	 	}
	 }
		
	
	
	
	return 0;
	
 	

 } 


 void cicloSem(){
 	while(1){
		 //Para comprobar si hay un proceso en la zona critica del cruce hay que comprobar el valor del semaforo

		//FASE 1
		
		CRUCE_pon_semAforo(0,2);//SEM_C1 A VERDE
		CRUCE_pon_semAforo(1,1);//SEM_C2 A ROJO
		CRUCE_pon_semAforo(2,1);//SEM_P1 A ROJO
		CRUCE_pon_semAforo(3,2);//SEM_P2 A VERDE
		signalf(7,1);
		signalf(10,1);
		nPausas(6);
		waitf(7,1);
		waitf(10,1);

		//SEM_C1 A AMARILLO
		cruce(0,3);
		nPausas(2);
		
		
		//FASE 2
		CRUCE_pon_semAforo(0,1);//SEM_C1 A ROJO
		CRUCE_pon_semAforo(1,2);//SEM_C2 A VERDE
		CRUCE_pon_semAforo(2,1);//SEM_P1 A ROJO
		CRUCE_pon_semAforo(3,1);//SEM_P2 A ROJO
		signalf(8,1);		
		nPausas(9);
		waitf(8,1);
		
		//SEM_C2 A AMARILLO
		cruce(1,3);
		nPausas(2);
		
		
		//FASE 3
		CRUCE_pon_semAforo(0,1);//SEM_C1 A ROJO
		CRUCE_pon_semAforo(1,1);//SEM_C2 A ROJO
		CRUCE_pon_semAforo(2,2);//SEM_P1 A VERDE
		CRUCE_pon_semAforo(3,1);//SEM_P2 A ROJO
		signalf(9,1);
		nPausas(12);
		waitf(9,1);
			
	}
}

void cruce(int sem,int color) {
	CRUCE_pon_semAforo(sem,color);//SEM A AMARILLO
	nPausas(2);
}

//Mientras haya cambio de semaforos bloqueamos la entrada al cruce de todos los procesos

 int crearHijo(){
 	pid_t pid = fork();
 	
 	switch(pid) {
 		case -1:
 			system("clear");
 			perror("Error creando hijos");
 			exit(errno);
		case 0:
			return pid;
		default:
			return pid;
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

	shmdt(&mc); //Desasociacion CAMBIAR EN ENCINA &mc -> mc

	if(shmctl(memid,IPC_RMID,NULL)==-1){
		perror("Error Liberar Memoria Compartida");
		exit(errno);
	}
	if(sem != -1){
		if(semctl(sem, 0, IPC_RMID) == -1){
		 	perror("Error semctl"); 
			exit(errno);
		}
	}
	CRUCE_fin();
	exit(0);
	
		
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
	int compro;
	struct posiciOn posSig, posSigSig,posPrueba;
	posSigSig.x=0;
	posSigSig.y=0;
	
	waitf(2,1);//wait de comprobacion
	compro=0;
	posSig = CRUCE_inicio_coche();
	waitf(3,1);//wait para iniciar el mov	
	posSigSig = CRUCE_avanzar_coche(posSig);
	
	waitf(5,1);//wait solo un proceso en el bucle (MEJORAR)
	while(posSigSig.y >= 0) {
		if(posSigSig.x==33 && posSigSig.y==4){
			waitf(7,1);
			posSigSig = CRUCE_avanzar_coche(posSigSig);
			signalf(7,1);
		}else if(posSigSig.x == 11 && posSigSig.y == 10){
			waitf(8,1);
			posSigSig = CRUCE_avanzar_coche(posSigSig);
			signalf(8,1);
		}else{
			posSigSig = CRUCE_avanzar_coche(posSigSig);
		}
		if(compro == 1) { pausa_coche();} 	
		if(compro == 0){//Solo entra una vez cada PROCESO
			compro = 1;
			signalf(2,1);//signal de comprobacion
			signalf(3,1);//signal para indicar que puede pasar a la siguiente posicion, SOLO ENTRA UNA VEZ
			pausa_coche();
		}
		if(posSigSig.y >= 0){
			if(posSig.x==33 && posSig.y==4){
				waitf(7,1);
				posSigSig = CRUCE_avanzar_coche(posSigSig);
				signalf(7,1);
			}else if(posSigSig.x == 11 && posSigSig.y == 10){
				waitf(8,1);
				posSigSig = CRUCE_avanzar_coche(posSigSig);
				signalf(8,1);
			}else{
				posSigSig = CRUCE_avanzar_coche(posSigSig);
			}
		}
	
	}
	signalf(5,1);
	
	//signalf(1,1);//Signal del final del Proceso
	CRUCE_fin_coche();
		
}


void iniciarPeatones() {
	struct posiciOn posSig, posSigSig, posNac;
	int compro;
	posNac.x=0;
	posNac.y=0;
	
	posSigSig.x=0;
	posSigSig.y=0;
	
	waitf(11,1);
	compro=0;
	posSig = CRUCE_inicio_peatOn_ext(&posNac);
	waitf(12,1);
	posSigSig = CRUCE_avanzar_peatOn(posSig);
	
	waitf(13,1);
	while(posSigSig.y >= 0) {
		if(posSigSig.y==12 && posSigSig.x>20){
				waitf(10,1);
				posSigSig = CRUCE_avanzar_peatOn(posSigSig);;
				signalf(10,1);
		}else if(posSigSig.x==29){
				waitf(9,1);
				posSigSig = CRUCE_avanzar_peatOn(posSigSig);;
				signalf(9,1);
		}else{
				posSigSig = CRUCE_avanzar_peatOn(posSigSig);
		}
		if(compro == 1) { pausa();} 
		if(compro == 0){//Solo entra una vez cada PROCESO
			compro = 1;
			signalf(11,1);//signal de comprobacion
			signalf(12,1);//signal para indicar que puede pasar a la siguiente posicion, SOLO ENTRA UNA VEZ
			pausa();
		}
		if(posSigSig.y >= 0){
			if(posSigSig.y==12 && posSigSig.x>20){
				waitf(10,1);
				posSigSig = CRUCE_avanzar_peatOn(posSigSig);;
				signalf(10,1);
			}else if(posSigSig.x==29){
				waitf(9,1);
				posSigSig =CRUCE_avanzar_peatOn(posSigSig);;
				signalf(9,1);
			}else{
				posSigSig = CRUCE_avanzar_peatOn(posSigSig);;
			}
		}
	}
	signalf(13,1);
	CRUCE_fin_peatOn();
	

}

