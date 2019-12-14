#include <stdio.h>
#include <wiringPi.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define LED1 0 //GPIO17 p.no11
#define LED2 2 //GPIO27 p.no13
#define LED3 3 //GPIO22 p.no15
#define LED4 27 //GPIO16 p.no36
#define LED5 28 //GPIO20 p.no38
#define LED6 29 //GPIO21 p.no40


int main(){
	int shmid;
	int pid;

	void *shared_memory = (void *)0;
	
	shmid = shmget((key_t)1234,sizeof(int),0);

	shared_memory = shmat(shmid, (void *)0, SHM_RDONLY);

	if(wiringPiSetup()==-1) return 1;

	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	pinMode(LED3, OUTPUT);
	pinMode(LED4, OUTPUT);
	pinMode(LED5, OUTPUT);
	pinMode(LED6, OUTPUT);

	for(;;){
		digitalWrite(LED2, 1);
		delay(1000);
		digitalWrite(LED3, 1);
		delay(1000);
		digitalWrite(LED4, 1);
		delay(1000);
		digitalWrite(LED5, 1);
		delay(1000);
		digitalWrite(LED6, 1);
		delay(1000);

		digitalWrite(LED2, 0);
		digitalWrite(LED3, 0);
		digitalWrite(LED4, 0);
		digitalWrite(LED5, 0);
		digitalWrite(LED6, 0);
		delay(1000);
	}
	return 0;
}


