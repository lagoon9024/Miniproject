#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define BUF_SIZE 100
#define MAX_CLNT 32
#define ID_SIZE 10
#define ARR_CNT 5 //파싱할때 몇개까지 문자열을 구분할지

#define DEBUG
typedef struct {
        int fd;
        char *from;//보낸사람아이디
        char *to;//받는사람아이디
        char *msg;//전송할 메세지
        int len;//메세지의 길이???????????
}MSG_INFO;

typedef struct {
        int index;//클라이언트 info의 갯수를 배열로 받는데, 클라이언트가 몇번째 위치인지 알려주기 위해 사용
        int fd;
    char ip[20];
        char id[ID_SIZE];
        char pw[ID_SIZE];
}CLIENT_INFO;

void * handle_clnt(void * arg);//스레드 핸들러,, 각각 클라이언트와 1:1로 통신하도록
void send_msg(MSG_INFO * msg_info, CLIENT_INFO * first_client_info);//모든사람이 아니라, 특정 ID에만 보낼수 있게끔,두번째 인자에서 CLIENT_INFO.index값을 받아준다. 그것이 index를 쓴 이유
void error_handling(char * msg);
void log_file(char * msgstr);

int clnt_cnt=0;//클라이언트 갯수를 세기 위한 전역변수 선언
pthread_mutex_t mutx;//mutex 변수 선언

int main(int argc, char *argv[])
{
        int serv_sock, clnt_sock; //fd를 저장할 변수
        struct sockaddr_in serv_adr, clnt_adr;
        int clnt_adr_sz;//sockaddr의 사이즈를 저장
        int sock_option  = 1;//sock_option은 컨트롤+c를 눌렀을 때의 오류 방지하기 위해 사용->밑에있음
        pthread_t t_id[MAX_CLNT] = {0};//클라이언트의 갯수만큼 아이디를 저장하기 위한 배열
        int str_len = 0;
        int i;
        char idpasswd[(ID_SIZE*2)+3];//id하고 pw가 클라이언트에서 넘어오면, 그것을 저장하기 위한 변수
        char *pToken;//파싱할 때 문자열의 시작주소를 저장하기 위해.
        char *pArray[ARR_CNT]={0};//파싱을 5개 했다면, 5개의 시작 주소를 저장하기위해 포인터 배배열 선언
        char msg[BUF_SIZE];//메세지의 버퍼사이즈를 저장하기 위한 배열

        CLIENT_INFO client_info[MAX_CLNT] = {{0,0,"","1","PASSWD"}, \
                         {0,0,"","2","PASSWD"},  {0,0,"","3","PASSWD"}, \
                         {0,0,"","4","PASSWD"},  {0,0,"","5","PASSWD"}, \
                         {0,0,"","6","PASSWD"},  {0,0,"","7","PASSWD"}, \
                         {0,0,"","8","PASSWD"},  {0,0,"","9","PASSWD"}, \
                         {0,0,"","10","PASSWD"},  {0,0,"","11","PASSWD"}, \
                         {0,0,"","12","PASSWD"},  {0,0,"","13","PASSWD"}, \
                         {0,0,"","14","PASSWD"},  {0,0,"","15","PASSWD"}, \
                         {0,0,"","16","PASSWD"},  {0,0,"","17","PASSWD"}, \
                         {0,0,"","18","PASSWD"},  {0,0,"","19","PASSWD"}, \
                         {0,0,"","20","PASSWD"},  {0,0,"","21","PASSWD"}, \
                         {0,0,"","22","PASSWD"},  {0,0,"","23","PASSWD"}, \
                         {0,0,"","24","PASSWD"},  {0,0,"","25","PASSWD"}, \
                         {0,0,"","26","PASSWD"},  {0,0,"","27","PASSWD"}, \
                         {0,0,"","28","PASSWD"},  {0,0,"","29","PASSWD"}, \
                         {0,0,"","30","PASSWD"},  {0,0,"","31","PASSWD"}, \
                         {0,0,"","LDJARD","PASSWD"}};
//원래는 DB로 저장을 해주거나, 환경파일에 동적할당으로 클라이언트의 정보를 access해야 한다. 그리고 CLIENT_INFO의 참조변수대로 index, fd, ip, id, pw가 저장되어 있음.
        if(argc != 2) {
                printf("Usage : %s <port>\n",argv[0]);
                exit(1);
        }
        fputs("IoT Server Start!!\n",stdout);//문자열을 표준출력장치(stdout)로 뿌린다.

        if(pthread_mutex_init(&mutx, NULL))//만일 오류가 났을때를 대비
                error_handling("mutex init error");

        serv_sock = socket(PF_INET, SOCK_STREAM, 0);//3번째 인자가 0이라는 것은 tcp를 가리킨다.

        memset(&serv_adr, 0, sizeof(serv_adr));
        serv_adr.sin_family=AF_INET;//IPv4를 사용
        serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
        serv_adr.sin_port=htons(atoi(argv[1]));//포트에 들어오는 문자열을 int형으로 바꾸고, 다시 network주소로 변경한다.

         setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&sock_option, sizeof(sock_option));
        if(bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr))==-1)
                error_handling("bind() error");

        if(listen(serv_sock, 5) == -1)//한순간에 5개정도의 정보를 listen하게끔.
                error_handling("listen() error");

        while(1) {
                clnt_adr_sz = sizeof(clnt_adr); //clnt_adr_sz은 clnt_adr의 주소값을 저장하기 위한 변수. clnt_adr의 값이 변할수 있기 때문에 항상 사이즈를 먼저 측정해줘야한다.
                clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);//클라이언트에서 connect신호가 들어올때까지 블로킹된다.
                if(clnt_cnt == MAX_CLNT) //클라이언트의 최대갯수를 넘겼을 때의 오류 처리.여기선 32개까지임
        {
                printf("socket full\n");
                        shutdown(clnt_sock,SHUT_WR);//최대갯수를 넘어서 접속시도한 클라이언트를 셧다운하겠다. (SHUT_WR : write하겠다.)
                continue;//어떤 클라이언트의 접속이 끊어지면, 다시 다른 클라이언트를 받아오도록 반복문의 처음으로 돌아간다.
                }
                else if(clnt_sock < 0)
                {
                printf("fail to call accept()\n");
                continue;
                }

                str_len = read(clnt_sock, idpasswd, sizeof(idpasswd));//클라이언트의 메세지와 id:pw를 받을때까지 블로킹/ 클라이언트의 48번째줄
                idpasswd[str_len] = '\0'; //긴문자열이 오다가 짧은 문자열이 오면, 쓰레기값이 발생. 이를 방지하기 위해 null값을 대입.

                if(str_len > 0)
                {
                        i=0;
                        pToken = strtok(idpasswd,"[:]");//id:pw를 파싱하기 위해. 이때, [ : ] 3개의 문자를 구분자로 설정.

                        while(pToken != NULL) //문자열이 아예 없는 NULL이 아닐때!
                        {
                                pArray[i] =  pToken;//cf)[1:PASSWD] 일때, 시작주소인 "[" 가 0x00 라면 0x00, 0x02, 0x09를 제외한 id와pw의 주소값을 배열에 저장한다.
                        if(++i >= ARR_CNT) //클라이언트의 구분자를 5개까지 설정해줬는데, 그 값을 넘어갔을 경우를 대비
                                        break;
                                pToken = strtok(NULL,"[:]");
                                //결국 pArray[0]에는 id의 시작주소, pArray[1]에는 pw의 시작주소가 들어간다.(시작주소가 들어감을 주의!)
                        }
                        for(i=0;i<MAX_CLNT;i++) //i는 32개보다 적을때를 반복문 실행. 즉 입력한 pArray[0]값과 저장한 32개의 id아이디중에 같은 아이디가 있는지 1~32의 아이디중에 조사.
                        {
                                if(!strcmp(client_info[i].id,pArray[0]))//방금 저장한 pArray[0]값과 구조체에서 저장한 32개의 아이디중에 같은 아이디가 있는지 조사한다.
                                {
                                        if(client_info[i].fd != 0) //클라이언트가 종료할때 fd=0으로 반환한다. fd!=이라면 클라이언트가 접속중인 상태.
                                        {
                                                sprintf(msg,"[%s] Already logged!\n",pArray[0]);//이미 클라이언트가 접속중이므로 오류 처리
                                                write(clnt_sock, msg,strlen(msg));
                                                log_file(msg);
                                                shutdown(clnt_sock,SHUT_WR);//그래서 이미접속되어있는 클라이언트정보로 또 접속시도를 하면 셧다운해버린다.
                                                client_info[i].fd = 0;

                                                break;

                                        }
                                        if(!strcmp(client_info[i].pw,pArray[1])) //이번에는 pw를 체크한다.
                                        {

                                                pthread_mutex_lock(&mutx);
                                                client_info[i].index = i;
                                                client_info[i].fd = clnt_sock;
                                                strcpy(client_info[i].ip,inet_ntoa(clnt_adr.sin_addr));
                                                pthread_mutex_unlock(&mutx);

                                                clnt_cnt++;//클라이언트가 접속한만큼 값을 증가시킨다.
                                                sprintf(msg,"[%s] New connected! (ip:%s,fd:%d,sockcnt:%d)\n",pArray[0],inet_ntoa(clnt_adr.sin_addr),clnt_sock,clnt_cnt);
                                                log_file(msg);
                                                write(clnt_sock, msg,strlen(msg));//154라인에서, 클라이언트가 접속됬다고 출력하는 msg를 클라이언트에도 그대로 보내준다.

                                                pthread_create(t_id+i, NULL, handle_clnt, (void *)(client_info + i));//4번째 매개변수에서, 클라이언트의 정보(client_info)를 스레드 생성하면서 값을 전달해준다. (cf. t_id+i : 포인트 표현 방법) 그리고 4번째 인자에서는 클라이언트의 정보가 담긴 배열의 주소값을 void형 포인터로 받아등린다!
                                                pthread_detach(t_id[i]);//detach를 통해, 스레드를 사용하며 할당했던 자원(ex.malloc)들을 회수한다. (cf. t_id[i] : 배열 표현방법)
                                                break;
                                        }
                                }
                        }
                        if(i == MAX_CLNT)
                        {
                                sprintf(msg,"[%s] Authentication Error!\n",pArray[0]);
                                write(clnt_sock, msg,strlen(msg));
                                log_file(msg);
                                shutdown(clnt_sock,SHUT_WR);
                        }
                }
                else
                        shutdown(clnt_sock,SHUT_WR);

        }
        return 0;
}

void * handle_clnt(void *arg)
{
        CLIENT_INFO * client_info = (CLIENT_INFO *)arg;
        int str_len = 0;
        int index = client_info->index;
        char msg[BUF_SIZE];
        char to_msg[MAX_CLNT*ID_SIZE+1];
        int i=0;
        char *pToken;
        char *pArray[ARR_CNT]={0};
        char strBuff[BUF_SIZE]={0};

        MSG_INFO msg_info;
        CLIENT_INFO  * first_client_info;

        first_client_info = (CLIENT_INFO *)((void *)client_info - (void *)( sizeof(CLIENT_INFO) * index ));//이연산을 통해, index가 몇이던 간에 첫번째 클라이언트의 정보값을 찾아와서 저장할 수 있다.
        while(1)
        {
                memset(msg,0x0,sizeof(msg));
                str_len = read(client_info->fd, msg, sizeof(msg)-1);
                if(str_len <= 0)
                        break;

                msg[str_len] = '\0';
                pToken = strtok(msg,"[:]");
                i = 0;
                while(pToken != NULL)
                {
                        pArray[i] =  pToken;
                        if(++i >= ARR_CNT)
                                break;
                        pToken = strtok(NULL,"[:]");
                }

                msg_info.fd = client_info->fd;
                msg_info.from = client_info->id;//송진자의 id를 저장!
                msg_info.to = pArray[0];
                sprintf(to_msg,"[%s]%s",msg_info.from,pArray[1]);//[송신자의id]수신할 메세지
                msg_info.msg = to_msg;
                msg_info.len = strlen(to_msg);

                sprintf(strBuff,"msg : [%s->%s] %s",msg_info.from,msg_info.to,pArray[1]);//귓속말경우. [누가->누구에게]
                log_file(strBuff);
                send_msg(&msg_info, first_client_info);
        }//여기가지 완료하고 while문이므로 또다른 메세지가 클라이언트로부터 들어오기를 계속 기다린다.

        close(client_info->fd);

        sprintf(strBuff,"Disconnect ID:%s (ip:%s,fd:%d,sockcnt:%d)\n",client_info->id,client_info->ip,client_info->fd,clnt_cnt-1);
        log_file(strBuff);

        pthread_mutex_lock(&mutx);
        clnt_cnt--;
        client_info->fd = 0;
        pthread_mutex_unlock(&mutx);

        return 0;
}

void send_msg(MSG_INFO * msg_info, CLIENT_INFO * first_client_info)
{
        int i=0;

        //[ID]MSG

        pthread_mutex_lock(&mutx);

        if(!strcmp(msg_info->to,"ALLMSG")) //수신자가 ALLMSG이면, 32명의 클라이언트에게 fd=0인 클라이언트 제외하고 동작!
        {
                for(i=0;i<MAX_CLNT;i++)
                        if((first_client_info+i)->fd != 0)      //fd=0이다? 종료된 클라이언트!
                        write((first_client_info+i)->fd, msg_info->msg, msg_info->len);
        }
        else if(!strcmp(msg_info->to,"IDLIST")) //지금 접속되어있는 ID를 보내줘야한다.
        {
                msg_info->msg[strlen(msg_info->msg) - 1] = '\0';

                for(i=0;i<MAX_CLNT;i++)
                {
                        if((first_client_info+i)->fd != 0)
                        {
                                strcat(msg_info->msg,(first_client_info+i)->id);//이때 msg에는 IDLIST 라는 글자만 있지, 다른 글자는 없는상태. 따라서, strcat을 이용해 <IDLIST + 클라이언트의 ID>하겠단 소리.
                                strcat(msg_info->msg," ");
                        }
                }
                strcat(msg_info->msg,"\n");//\n은 약속이다. 보통 이런 뉴라인을 많이 쓴다. buffer의 데이터가 들어오다가 뉴라인을 만나면 하나의 명령이 끝났다는 약속.
        write(msg_info->fd, msg_info->msg, strlen(msg_info->msg));
        }//여기까지 IDLIST 처리하는 부분
        else//귓속말인 경우
                for(i=0;i<MAX_CLNT;i++)
                        if((first_client_info+i)->fd != 0)
                                if(!strcmp(msg_info->to,(first_client_info+i)->id))//id랑 수신자랑 같다면,
                                write((first_client_info+i)->fd, msg_info->msg, msg_info->len);//그 id를 가진 클라이언트에게 write한다.
        pthread_mutex_unlock(&mutx);
}

void error_handling(char *msg)
{
        fputs(msg, stderr);
        fputc('\n', stderr);
        exit(1);
}

void log_file(char * msgstr)
{
        char date[11];
        char fname[80];
        char str[200];
        int logfd;
        struct tm *t;
        time_t tt;
        DIR * dirp;
        tt=time(NULL);
        t=localtime(&tt); //tt를 아시아의 서울 시간으로 환산
#ifdef DEBUG
        fputs(msgstr,stdout);
#endif
        sprintf(date,"%d-%02d-%02d",t->tm_year+1900,t->tm_mon+1,t->tm_mday);//년, 월, 일을 data라는 변수에 저장
        getcwd(str,sizeof(str));//현재 이프로그램이 돌고있는 절대 경로를 str에 가져온다.
        strcat(str,"/log.d");//strcat으로 문자 뒤에 /log.c를 붙힌다.
        dirp = opendir(str);//디렉토리 오픈
        if(dirp == NULL)//만일 dirp이라는 디렉토리가 없다면,
                mkdir(str,0755);//0755라는 디렉토리 생성
        sprintf(fname,"%s/%s.txt",str,date);

                pthread_mutex_lock(&mutx);
        logfd = open(fname, O_RDWR | O_CREAT,0644); //파일을 열자, 파일이 없다면 0644권한으로 파일을 만들자.
        if(logfd > 0 )
        {
                lseek(logfd,(off_t)0,SEEK_END);//두번째 인자: 제일 끝으로 움직이라.
                sprintf(str,"%02d:%02d:%02d %s",t->tm_hour,t->tm_min,t->tm_sec,msgstr);//시 분 초
                write(logfd,str,strlen(str));
                close(logfd);
        }
        else
                perror("open:");
                pthread_mutex_unlock(&mutx);
}