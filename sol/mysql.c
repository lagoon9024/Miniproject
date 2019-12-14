#define SOCKET int
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mysql/mysql.h>

static char *host = "localhost";
static char *user = "fff";
static char *pass = "1234";
static char *dbname = "db";

void error(MYSQL *conn)
{
	fprintf(stderr, "%s\n", mysql_error(conn));
	mysql_close(conn);
	exit(1);
}
int main(int argc, char **argv){
	MYSQL *conn;
	MYSQL_RES *result;
	conn = mysql_init(NULL);
	int res;
	char buf[1024];

	if(!(mysql_real_connect(conn, host, user, pass, dbname, 0, NULL, 0)))
	{
		fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
		exit(1);
	}
	printf("Connection Successful!\n\n");

	if(!strcmp(argv[1], "1"))//Insert data
	{
		sprintf(buf, "insert into library (name, writer, publisher) values ('%s', '%s', '%s')", argv[2], argv[3], argv[4]);
		res = mysql_query(conn, buf);
		if(!res)
			printf("Insert <%s> <%s> <%s> successful!\n", argv[2], argv[3], argv[4]);
		else
			error(conn);
	}
	else if(!strcmp(argv[1], "2"))//Find data
	{
		mysql_query(conn, "select * from library");
		result = mysql_store_result(conn);
		if(result == NULL)
		{
			error(conn);
		}
		int num_fields = mysql_num_fields(result);

		MYSQL_ROW row;
		while(row = mysql_fetch_row(result))
		{
			if(!strcmp(row[0], argv[2]))
			{
				for(int i=0; i<num_fields; i++)
				{
					printf("%s   ", row[i]);
				}
				printf("\n");
			}
		}

	}
	else
	{
		printf("Usage : <Insert:1/Find:2> <bookname>\n");
		exit(1);
	}
	mysql_free_result(result);
	mysql_close(conn);

	return EXIT_SUCCESS;
}
