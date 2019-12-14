// mysql.c
// library table

#define SOCKET int
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mysql/mysql.h>
char *host = "localhost";
char *user = "fff";
char *pass = "1234";
char *dbname = "db";
int state = -1;

char * error(MYSQL *conn, int val)
{
	char * tmp;
	sprintf(tmp, "%d", val);
	return tmp;
}

char * find(char *index,char *text)
{

	MYSQL *conn = mysql_init(NULL);
	MYSQL_RES *result;
	conn = mysql_init(NULL);
	int res;
	char buf[BUFSIZ]="";
	char s[BUFSIZ]="";
	int row_num;
	int row_cnt=0;

	if(!(mysql_real_connect(conn, host, user, pass, dbname, 0, NULL, 0)))
	{
		return "-1";
	}
	printf("1Connection Successful!\n\n");



	mysql_query(conn, "select * from library");
	result = mysql_store_result(conn);
	if(result == NULL)
	{
		error(conn, -2);
	}
	int num_fields = mysql_num_fields(result);

	
	if(!strcmp("name\n",index))
		row_num=0;
	else if(!strcmp("writer\n",index))
		row_num=1;
	else if(!strcmp("publisher\n",index))
		row_num=2;

	MYSQL_ROW row;
	while(row = mysql_fetch_row(result))
	{
		strcpy(buf,"");
		if(strstr(row[row_num], text))
		{
			row_cnt++;
			for(int i=0; i<num_fields; i++)
			{
				sprintf(buf,":%s",row[i]);
				strcat(s,buf);
			}
		}
	}

	
	sprintf(buf,"%d",row_cnt);
	mysql_close(conn);
	mysql_free_result(result);
	strcat(buf,s);
	printf("buf : %s\n",buf);

	return buf;

}

int add(char *name,char *writer,char*publisher)
{
	MYSQL *conn = mysql_init(NULL);
	MYSQL_RES *result;
	conn = mysql_init(NULL);
	int res;
	char buf[1024];

	if(!(mysql_real_connect(conn, host, user, pass, dbname, 0, NULL, 0)))
	{
		return -1;
	}
	printf("2connection Successful!\n\n");

	printf("name:%s, writer:%s, pub:%s\n", name, writer, publisher);
	sprintf(buf, "insert into library (name, writer, publisher) values ('%s', '%s', '%s')", name, writer, publisher);
	res = mysql_query(conn, buf);

	if(!res)
	{
		return 1;
	}
	else
	{
		error(conn, -2);
	}
	mysql_close(conn);
	mysql_free_result(result);
}


int delete(char *index,char *text)
{
	MYSQL *conn = mysql_init(NULL);
	MYSQL_RES *result;
	conn = mysql_init(NULL);
	int res;
	char buf[1024];

	if(!(mysql_real_connect(conn, host, user, pass, dbname, 0, NULL, 0)))
	{
		return -1;
	}
	printf("3Connection Successful!\n\n");

	mysql_query(conn, "select * from library");
	result = mysql_store_result(conn);
	if(result == NULL)
	{
		return -2;
	}
	int num_fields = mysql_num_fields(result);

	
	sprintf(buf,"delete from library where %s='%s'",index,text);

	res=mysql_query(conn,buf);

	if(!res)
	{
		printf("Delete successful!\n");
		return 1;
	}
	else
	{
		return -3;
	}
	mysql_close(conn);
	mysql_free_result(result);
}
