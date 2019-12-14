#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

#define BUF_SIZE 100
#define NAME_SIZE 20
#define ARR_CNT 5

void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);

char name[NAME_SIZE]="[Default]";
char msg[BUF_SIZE];

int main(int argc, char *argv[])
{
        int sock;
        struct sockaddr_in serv_addr;
        pthread_t snd_thread, rcv_thread;//thread의 id를 저장하기 위한 변수
        void * thread_return;

        if(argc != 4) {
                printf("Usage : %s <IP> <port> <name>\n",argv[0]);
                exit(1);
        }

        sprintf(name, "%s",argv[3]);//name에 3번째 argument vector(클라이언트id)를 저장

        sock = socket(PF_INET, SOCK_STREAM, 0);
        if(sock < 0)
                error_handling("socket() error");

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family=AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(argv[1]);//서버 주소를 argv1값에서 가져옴
        serv_addr.sin_port = htons(atoi(argv[2]));//port를 argv2에서 가져옴

        if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
                        error_handling("connect() error");//서버로 접속 시도

        sprintf(msg,"[%s:PASSWD]",name);//서버에 메세지와 id:pw 를 전송(서버의 114번째줄)
        write(sock, msg, strlen(msg));
        pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock); //recv_msg 스레드가 먼저 실행된다. 이때 밑의 recv_msg 함수로 이동
        pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);

        pthread_join(snd_thread, &thread_return);
        pthread_join(rcv_thread, &thread_return);
        pthread_detach(snd_thread);
        pthread_detach(rcv_thread);//detach를 통해 자원을 회수하고, 클라이언트를 종료한다. 그런데 rcv_thread와 snd_thread중에 어떤것이 먼저 종료될지는 아무도 모른다.

        close(sock);
        return 0;
}

void * send_msg(void * arg) // arg는 fd값.
{
        int *sock = (int *)arg;
        int str_len;
        int ret;
        fd_set initset, newset;//select라는 함수를 사용하기 위한 변수 선언.
        struct timeval tv;
        char name_msg[NAME_SIZE + BUF_SIZE+2];

        FD_ZERO(&initset);//변수를 초기화 ->>>>>교재 276 참조
        FD_SET(STDIN_FILENO, &initset);//STDIN_FILENO은, fd의 값을 조사해서 initset에 저장.

        fputs("Input a message! [ID]msg (Default ID:ALLMSG)\n",stdout);
        while(1) {
                memset(msg,0,sizeof(msg));
                name_msg[0] = '\0';//혹시 이전에 발생한 쓰레기값이 있다면, 초기화
                tv.tv_sec = 1;//time_value값을 저장
                tv.tv_usec = 0;//microsecond 단위의 값 저장
                newset = initset;
                ret = select(STDIN_FILENO + 1, &newset, NULL, NULL, &tv);//첫 매개변수는, 넘어온값보다 작은값이 들어올때를 검사해서, 검출되면 +1처리해준다. 두번째 변수는 read형 변수관련.
        if(FD_ISSET(STDIN_FILENO, &newset))//newset에 담겨있는 값이, STDIN_FILENO에 들어오는 값인 1인지 확인.
                {
                        fgets(msg, BUF_SIZE, stdin);//표준입출력장치로 메세지를 msg에 저장
                        if(!strncmp(msg,"quit\n",5)) {//클라이언트에서 종료하고자 할때
                                *sock = -1;
                                return NULL;
                        }
                        else if(msg[0] != '[')//클라이언트에서 메세지 보낼때, [ 를 입력하지 않았다면 [ALLMSG]를 붙혀서 메세지를 전송시키기 위해 사용
                        {
                        strcat(name_msg,"[ALLMSG]");
                                strcat(name_msg,msg);
                        }
                        else
                                strcpy(name_msg,msg);
                        if(write(*sock, name_msg, strlen(name_msg))<=0)//클라이언트가 data를 보냈는데, 이미 서버에서 종료되었을 때를 대비
                        {
                                *sock = -1;//sock은 fd를 저장하기 위한 포인터 변수였음.
                                return NULL;
                        }
                }
                if(ret == 0) //1초동안 아무 메세지가 입력되지 않았다면, ret=0이 될테고, 그때 초기화해주는 과정. 그 체크하는 주기가 1초임. 그리고 클라이언트가 아무 키보드도 입력하지 않는다면, 1초에 한번씩 계속 이 if문안으로 들어와 실행된다.
                {
                        if(*sock == -1)
                                return NULL;
                }
        }
}

void * recv_msg(void * arg)
{
        int * sock = (int *)arg;
        int i;
    char *pToken;
        char *pArray[ARR_CNT]={0};

        char name_msg[NAME_SIZE + BUF_SIZE +1];
        int str_len;
        while(1) {
                memset(name_msg,0x0,sizeof(name_msg));
                str_len = read(*sock, name_msg, NAME_SIZE + BUF_SIZE );
                if(str_len <= 0)
                {
                        *sock = -1;
                        return NULL;
                }
                name_msg[str_len] = 0;
                fputs(name_msg, stdout);

/*              pToken = strtok(name_msg,"[:]");
                i = 0;
                while(pToken != NULL)
                {
                        pArray[i] =  pToken;
                        if(++i >= ARR_CNT)
                                break;
                        pToken = strtok(NULL,"[:]");
                }

//              printf("id:%s, msg:%s,%s,%s,%s\n",pArray[0],pArray[1],pArray[2],pArray[3],pArray[4]);
                printf("id:%s, msg:%s\n",pArray[0],pArray[1]);
*/
        }
}

void error_handling(char * msg)
{
        fputs(msg, stderr);
        fputc('\n', stderr);
        exit(1);
}