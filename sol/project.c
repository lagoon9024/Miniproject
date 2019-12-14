#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>


int main()
{
	char temp[BUFSIZ];
	int shmid;
	void *shared_memory=(void *)0;
	int *cal_num;
	int status;
	pid_t pid;

	shmid=shmget((key_t)1234,sizeof(int),IPC_CREAT|0666);
	if(shmid==-1)
	{
		perror("shmget");
		exit(0);
	}

	shared_memory=shmat(shmid,(void *)0,0);
	if(shared_memory==(void*)-1){
		perror("shmat");
		exit(0);
	}

	cal_num=(int*)shared_memory;
	*cal_num=5;

	while(1)
	{
		while(*cal_num!=5&&*cal_num!=2);
		printf("입력 : ");	
		scanf("%s",temp);
		*cal_num=0;
		
		if(strcmp(temp,"Q")==0)
		{
			printf("Bye\n");
			
			if(shmctl(shmid,IPC_RMID,0)==-1)
			{
				perror("shmctl");
				exit(1);
			}
			else
				printf("제거\n");
			break;
		}

		if((pid=fork())>0)
		{
			//while(*cal_num!=0)
			//{
			
			if(strcmp(temp,"1")==0)
			{
				char buf[3][50];
				//printf("input a b c\n");
				scanf("%s %s %s",buf[0],buf[1],buf[2]);
				printf("%s %s %s %s\n",temp,buf[0],buf[1],buf[2]);
				execl("./mysql","./mysql",temp,buf[0],buf[1],buf[2],NULL);
				
			}
			else if(strcmp(temp,"2")==0)	
			{
				//buf[0]=new char[BUFSIZ];
				char t[20];
				scanf("%s",t);
				execl("./mysql","./mysql",temp,t,NULL);
			}
			sleep(1);
			//}
			
			
			// sql
			// insert -> led : *cal_num=2
			// select -> led : *cal_num=3
			sleep(5);	
			*cal_num=2;
			printf("pid ex1\n");
			wait(&status);
		}

		else if(pid==0)
		{
			while(*cal_num!=2)
			{
				execv("./piproject",NULL);
				sleep(1);
				// 버퍼링 led
			}
			//led제어*cal_num=5 -> 제어 끝

			*cal_num=5;
			printf("pid ex\n");
			exit(0);
			//exit(&status);
		}
	}
	return 0;
}
