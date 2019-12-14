// mysql_user.c
// USER table

#define SOCKET int
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mysql/mysql.h>
extern char *host;
extern char *user;
extern char *pass;
extern char *dbname;
extern int state;
//int find_id(char *user_id);
//int insert(char *user_id,char *pw);
//int login(char *id,char *pw);

int login(char *user_id,char *pw) // search id
{
	MYSQL *conn = mysql_init(NULL);
	MYSQL_RES *result;
	state = -1;

	printf("host:%s, user:%s, pass:%s, dbname:%s\n", host, user, pass, dbname);
        if(!(mysql_real_connect(conn, host, user, pass, dbname, 0, NULL, 0)))
        {
                fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
                exit(1);
        }
        printf("4Connection Successful!\n\n");

	mysql_query(conn, "select * from USER");
	result = mysql_store_result(conn);

	int num_fields = mysql_num_fields(result);

	MYSQL_ROW row;
	while(row = mysql_fetch_row(result))
	{
		if(!strcmp(row[1], user_id)) // user id 존재
		{
			// pw
			if(!strcmp(row[2],pw)) // pw 일치
			{
				printf("login success!!\n");
				state = 1;
				// login
			}
			else
			{	
				printf("login fail...\n");
				state=-1;
			}
			break;
		}
	}

	mysql_free_result(result);
	mysql_close(conn);
	return state;
}

int find_id(char *id)
{
	state = -1;
	MYSQL *conn = mysql_init(NULL);
	MYSQL_RES *result;
	printf("host:%s, user:%s, pass:%s, dbname:%s\n", host, user, pass, dbname);
	
        if(!(mysql_real_connect(conn, host, user, pass, dbname, 0, NULL, 0)))
        {
                fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
                exit(1);
        }
        printf("5Connection Successful!\n\n");

	mysql_query(conn, "select * from USER");
	result = mysql_store_result(conn);

	int num_fields = mysql_num_fields(result);

	MYSQL_ROW row;
	while(row = mysql_fetch_row(result))
	{
		if(!strcmp(row[1], id)) // user id 존재
		{
			printf("ID : %s\n",id);
			state = 1;
			break;
		}

	}

	mysql_close(conn);
	mysql_free_result(result);
	return state;
}


int insert(char *id,char *pw)
{
	state = -1;
	MYSQL *conn = mysql_init(NULL);
        if(!(mysql_real_connect(conn, host, user, pass, dbname, 0, NULL, 0)))
        {
                fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
                exit(1);
        }
        printf("6Connection Successful!\n\n");
	char buf[BUFSIZ];
	int res;
	sprintf(buf,"insert into USER (user_id,pw) values('%s', '%s')",id,pw);
	res=mysql_query(conn,buf);
	if(!res)
	{
		printf("Insert Success!!\n");
		state=0;
	}
	else
	{
		//error(conn);
		state=-1;
	}
	mysql_close(conn);
	return state;
}
