/******************************************************************************
 * Function: 	Mysql use.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01							
 * Compile:	gcc -Wall mysql_base.c -I/usr/include/mysql -lmysqlclient -o mysql_base
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <mysql.h>

MYSQL my_connection;
MYSQL_ROW sqlrow;
MYSQL_RES *res_ptr;
MYSQL_FIELD *field_ptr;

int first_row = 1;

int connect_mysql()
{
	mysql_init(&my_connection);

	if (mysql_real_connect
	    (&my_connection, "localhost", "root", "test", "test", 0, NULL, 0)) {
		printf("Connection success\n");
		return 0;
	} else {
		fprintf(stderr, "Connect error: %s\n",
			mysql_error(&my_connection));
		return -1;
	}
}

int exec_mysql(char *query)
{
	if (mysql_query(&my_connection, query) == 0) {
		printf("Affect %lu rows\n",
		       (unsigned long)mysql_affected_rows(&my_connection));
		return 0;
	} else {
		printf("Exec error %d: %s\n",
		       mysql_errno(&my_connection),
		       mysql_error(&my_connection));
		return -1;
	}
}

int affect_records()
{
	if (mysql_query(&my_connection, "SELECT LAST_INSERT_ID()") == 0) {
		res_ptr = mysql_use_result(&my_connection);
		if (res_ptr) {
			while ((sqlrow = mysql_fetch_row(res_ptr)))
				printf("Inserted childno %s\n", sqlrow[0]);
			mysql_free_result(res_ptr);
		}
		return 0;
	} else {
		printf("SELECT LAST_INSERT_ID() error: %s\n",
		       mysql_error(&my_connection));
		return -1;
	}
}

void display_header()
{
	printf("Column details:\n");

	while ((field_ptr = mysql_fetch_field(res_ptr)) != NULL) {
		printf("\t Name: %s\t Type: \n", field_ptr->name);
		if (IS_NUM(field_ptr->type)) {
			printf("Numeric field\n");
		} else {
			switch (field_ptr->type) {
			case FIELD_TYPE_VAR_STRING:
				printf("VARCHAR\n");
				break;

			case FIELD_TYPE_LONG:
				printf("LONG\n");
				break;

			default:
				printf("Type is %d, check mysql_com.h\n",
				       field_ptr->type);
			}
		}

		printf("\t Max width %ld\n", field_ptr->length);
		if (field_ptr->flags & AUTO_INCREMENT_FLAG)
			printf("\t Auto increments\n");

		printf("\n");
	}
}

void display_row()
{
	unsigned int field_count = 0;

	while (field_count < mysql_field_count(&my_connection)) {
		if (sqlrow[field_count])
			printf("%s ", sqlrow[field_count]);
		else
			printf("NULL");

		field_count++;
	}
	printf("\n");
}

void result_mysql()
{
	if ((res_ptr = mysql_use_result(&my_connection))) {
		while ((sqlrow = mysql_fetch_row(res_ptr))) {
			if (first_row) {
				display_header();
				first_row = 0;
			}

			display_row();
		}

		if (mysql_errno(&my_connection))
			printf("Retrive error: %s\n",
			       mysql_error(&my_connection));

		mysql_free_result(res_ptr);
	}
}

void close_mysql()
{
	mysql_close(&my_connection);
}

int main(int argc, char *argv[])
{
	char sql_str1[128], sql_str2[128], sql_str3[128];

	sprintf(sql_str1, "%s",
		"INSERT INTO children(fname, age) VALUES('Robert', 7)");
	sprintf(sql_str2, "%s",
		"UPDATE children SET AGE = 4 WHERE fname = 'Ann'");
	sprintf(sql_str3, "%s",
		"SELECT childno, fname, age FROM children WHERE age > 5");

	if (connect_mysql())
		return -1;

	if (exec_mysql(sql_str1))
		return -1;
	affect_records();

	if (exec_mysql(sql_str2))
		return -1;
	affect_records();

	if (exec_mysql(sql_str3))
		return -1;
	result_mysql();

	close_mysql();

	return 0;
}
