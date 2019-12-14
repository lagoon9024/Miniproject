#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#define NAME_SIZE 1024 

void *send_msg(void *arg);
void *recv_msg(void *arg);
char name[NAME_SIZE];
char message[BUFSIZ];
int str_len = 0;

void error_handling(char *message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int main(int argc, char **argv){
	int sock;
	struct sockaddr_in serv_addr;
	int read_len = 0;
	pthread_t snd_thread, rcv_thread;
	void *thread_return;


	if(argc !=4){
		printf("Usage : %s <IP> <PORT> <Nickname>\n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error!");
	strcpy(name,argv[3]);
	write(sock, name, strlen(name));
	char opt[10];
	str_len=read(sock,opt,10);
	printf("%s\n", opt);
	if(str_len==-1)
		error_handling("read() error");
	if(opt[0]=='2'){
		printf("Welcome! new account : %s\n", name);
		printf("Input your passwd :");
		fgets(message, BUFSIZ,stdin);
		char newuser[NAME_SIZE+BUFSIZ];
		sprintf(newuser,"%s:%s", name,message);
		write(sock,newuser,strlen(newuser));
		printf("Created New account : %s\n", name);
		return 0;
	}
	else if(opt[0]=='1'){
		printf("Input your passwd :");
		fgets(message, BUFSIZ,stdin);
		char newuser[NAME_SIZE+BUFSIZ];
		sprintf(newuser,"%s", message);
		write(sock,newuser,strlen(newuser));
		str_len = read(sock,message,BUFSIZ);
		printf("%s\n", message);
		if(strstr(message, "wrong")) return 0;
		pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
		pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
		pthread_join(snd_thread, &thread_return);
		pthread_join(rcv_thread, &thread_return);
	}

	close(sock);

	return 0;
}

void *send_msg(void *arg){
	int sock=*((int *)arg);
	char msg[NAME_SIZE+BUFSIZ];
	while(1){

		printf("Choose option:(add, del, find, q) : ");
		memset(message, 0x0, sizeof(message));
		memset(msg, 0x0, sizeof(msg));
		scanf("%s", message);
		strcat(message,"\n");

		if(!strcmp(message, "q\n") || !strcmp(message, "Q\n")){
			close(sock);
			exit(0);
		}
		else if(!strcmp(message,"add\n")){
			sprintf(msg,"%s",message);
			printf("input title :");
			scanf("%s", message);
			sprintf(msg,"%s:%s",msg,message);
			printf("input author :");
			scanf("%s", message);
			sprintf(msg,"%s:%s",msg,message);
			printf("input publisher :");
			scanf("%s", message);
			sprintf(msg,"%s:%s",msg,message);
			write(sock, msg, strlen(msg));
		}
		else if(!strcmp(message,"del\n") || !strcmp(message,"find\n")){
			sprintf(msg,"%s:",message);
			printf("input name or writer or publisher: ");
			scanf("%s", message);
			strcat(msg, message);
			printf("input data: ");
			scanf("%s", message);
			
			sprintf(msg,"%s:%s",msg,message);

			strcat(msg,":1234");
			write(sock,msg,strlen(msg));
		}
		else{
			printf("undefined option...\n");
			continue;
		}

	}
	return NULL;
}

void *recv_msg(void *arg){
	int sock=*((int *)arg);
	char name_msg[NAME_SIZE+BUFSIZ];
	while(1){
		str_len=read(sock, name_msg, NAME_SIZE+BUFSIZ-1);
		if(str_len==-1)
			return (void *)-1;
		name_msg[str_len]=0;
		fputs(name_msg,stdout);
	}
	return NULL;
}

