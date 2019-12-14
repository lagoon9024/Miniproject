#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include "mysql_user.c"
#include "mysql.c"
#define MAXBUF 1024
#define MAX_CLNT 100

typedef struct{
	int fd;
	char ip[20];
	char id[30];

}CLIENT_INFO;
CLIENT_INFO clnt_info[MAX_CLNT];

void * handle_clnt(void * arg);
void send_msg(char * msg, int len, int _fd);
void error_handling(char * msg, int code);

int clnt_cnt = 0;
pthread_mutex_t mutx;

int main(int argc, char **argv)
{
	int serv_sock, clnt_sock;
	int sock_option = 1;
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;
	pthread_t thread_id;
	char *pToken;
	char *pArray[2] = {0};


	if(argc != 2)
	{
		printf("Usage: %s <port> \n", argv[0]);
		exit(1);
	}

	pthread_mutex_init(&mutx, NULL);
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1)
	{
		error_handling("socket() error", 1);
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&sock_option, sizeof(sock_option));
	if(bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("bind() error", 1);

	if(listen(serv_sock, 5) == -1)
		error_handling("listen() error", 1);


	while(1)
	{
		clnt_addr_size = sizeof(clnt_addr);
		clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
		if(clnt_sock == -1)
			error_handling("accept() error", 1);

		char idmsg[MAXBUF];
		int str_len = read(clnt_sock, idmsg, sizeof(idmsg) - 1);
		idmsg[str_len] = '\0';

		pthread_mutex_lock(&mutx);

		clnt_info[clnt_cnt].fd = clnt_sock;
		strcpy(clnt_info[clnt_cnt].ip, inet_ntoa(clnt_addr.sin_addr));
		strcpy(clnt_info[clnt_cnt].id, idmsg);
		clnt_cnt++;
		pthread_mutex_unlock(&mutx);
		printf("connected client IP:%s, id:%s\n", inet_ntoa(clnt_addr.sin_addr), idmsg);

		pthread_create(&thread_id, NULL, handle_clnt, (void *)&clnt_info[clnt_cnt - 1]);
		pthread_detach(thread_id);
	}
	close(serv_sock);
	return 0;
}
void * handle_clnt(void * arg)
{
	CLIENT_INFO *client_arg = (CLIENT_INFO *)arg;
	char id[30], pw[30];
	int str_len = 0, i, flag=0, tmp;
	char msg[MAXBUF], buff[MAXBUF];
	char *pToken;
	char *pArray[4] = {0};
	int clnt_sock = client_arg->fd;
	strcpy(id, client_arg->id);

	/* 
	 *	flag 0 : id check
	 *	flag 1 : pw check
	 *	flag 2 : add id, pw
	 *	flag 3 : find, add, delete
	 * */

	//while(1)
	//{
	if(flag == 0)//id check
	{
		tmp = find_id(id);	
		if(tmp == 1) // ID exist in mysql table -> pw check
		{
			flag = 1;
			write(clnt_sock, "1", strlen("1")+1);
		}
		else// ID not exist in mysql table -> add id, pw
		{
			flag = 2;
			write(clnt_sock, "2", strlen("2")+1);
		}
	}
	if(flag == 1)//pw check
	{
		str_len = read(clnt_sock, pw, 29);
		pw[str_len] = '\0';
		tmp = login(id, pw);
		if(tmp == 1)
		{	
			write(clnt_sock, "login success", strlen("login success"));
			flag = 3;
		}
		else
		{
			write(clnt_sock, "wrong passwd", strlen("wront passwd"));
		}
	}
	if(flag == 2)//add id, pw
	{
		str_len = read(clnt_sock, msg, MAXBUF - 1);
		i = 0;
		pToken = strtok(msg, ":");
		while(pToken != NULL)
		{
			pArray[i] = pToken;
			if(++i >= 2) break;
			pToken = strtok(NULL, ":");
		}
		tmp = insert(pArray[0], pArray[1]);
		sprintf(msg, "Add User [%s:%s] Success!", pArray[0], pArray[1]);
		write(clnt_sock, msg, strlen(msg));
	}
	if(flag == 3)//find, add, delete
	{
		while(1)
		{
			str_len = read(clnt_sock, msg, MAXBUF - 1);
			i = 0;
			pToken = strtok(msg, ":");
			while(pToken != NULL)
			{
				pArray[i] = pToken;
				if(++i >= 4) break;
				pToken = strtok(NULL, ":");
			}
			if(strstr(pArray[0], "add"))
			{
				tmp = add(pArray[1], pArray[2], pArray[3]);
				if(tmp == 1)
				{
					write(clnt_sock, "Add Data Success", strlen("Add Data Success"));
				}
				else if(tmp == -2)
				{
					write(clnt_sock, "Add Data Fail", strlen("Add Data Fail"));
				}
				else
				{
					write(clnt_sock, "mysql connect error", strlen("mysql connect error"));
				}
			}
			else if(strstr(pArray[0], "find"))
			{
				int j = 0;
				char find_msg[BUFSIZ];
				char *find_arr[128];
				char *pToken2;
				strcpy(find_msg, find(pArray[1], pArray[2]));
				printf("find_msg: %s\n",find_msg);
				pToken2 = strtok(find_msg, ":");
				while(pToken2 != NULL)
				{
					find_arr[j] = pToken2;
					pToken2 = strtok(NULL, ":");
					j++;
				}
				if(!strcmp(find_arr[0], "-1"))
				{
					write(clnt_sock, "mysql connect error", strlen("mysql connect error"));
				}
				else if(!strcmp(find_arr[0], "-2"))
				{
					write(clnt_sock, "mysql store_result error", strlen("mysql store_result error"));
				}
				else
				{
					memset(find_msg, 0x0, sizeof(find_msg));
					j = atoi(pArray[0]);
					for(int a = 0; a < j; a++)
					{
						sprintf(find_msg, "%s: %s / %s / %s", a+1, pArray[3*a+1], pArray[3*a+2], pArray[3*a+3]);
						write(clnt_sock, find_msg, strlen(find_msg));
					}	

				}
			}
			else
			{
				tmp = delete(pArray[1], pArray[2]);
				if(tmp == -1)
					write(clnt_sock, "mysql connect error", strlen("mysql connect error"));
				else if(tmp == -2)
					write(clnt_sock, "mysql store_result error", strlen("mysql store_result error"));
				else if(tmp == -3)
					write(clnt_sock, "Delete Fail", strlen("Delete Fail"));
				else
					write(clnt_sock, "Delete Success", strlen("Delete Success"));
			}

		}

	}
	//}
	pthread_mutex_lock(&mutx);
	for(i = 0; i < clnt_cnt; i++)
	{
		if(clnt_sock == clnt_info[i].fd)
		{
			while(i++ < clnt_cnt - 1)
			{
				clnt_info[i].fd = clnt_info[i+1].fd;
				strcpy(clnt_info[i].ip, clnt_info[i+1].ip);
				strcpy(clnt_info[i].id, clnt_info[i+1].ip);
			}
			break;
		}
	}
	clnt_cnt--;
	pthread_mutex_unlock(&mutx);
	close(clnt_sock);
	return NULL;
}
void send_msg(char * msg, int len, int fd_)
{
	int i;
	pthread_mutex_lock(&mutx);
	if(fd_ == -1)
	{
		for(i = 0; i < clnt_cnt; i++)
			write(clnt_info[i].fd, msg, len);
	}
	else
	{
		write(fd_, msg, len);
	}
	pthread_mutex_unlock(&mutx);
}
void error_handling(char *msg, int code)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(code);
}

