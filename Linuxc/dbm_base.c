/******************************************************************************
 * Function: 	Dbm used to store data struct.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01							
 * Compile:	gcc  -Wall dbm_base.c -lgdbm_compat -lgdbm -o dbm_base 
******************************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ndbm.h>
//#include <gdbm-ndbm.h>

#define  DB_FILE 		"/tmp/dbm2_test"
#define  ITEMS_USED 	3

DBM *dbm_ptr;

struct test_data {
	int any_integer;
	char misc_chars[15];
	char more_chars[21];
};

int dbm_initial(DBM ** dbm_ptr)
{
	*dbm_ptr = dbm_open(DB_FILE, O_RDWR | O_CREAT, 0666);
	if (*dbm_ptr == 0) {
		fprintf(stderr, "Failed to open database\n");
		return -1;
	}

	return 0;
}

int dbm_stored(DBM * dbm_ptr)
{
	int i, result;
	char key_to_use[20];
	datum key_datum, data_datum;
	struct test_data items_to_store[ITEMS_USED];

	memset(items_to_store, '\0', sizeof(items_to_store));
	strcpy(items_to_store[0].misc_chars, "First!");
	items_to_store[0].any_integer = 47;
	strcpy(items_to_store[0].more_chars, "foo");
	strcpy(items_to_store[1].misc_chars, "bar");
	items_to_store[1].any_integer = 13;
	strcpy(items_to_store[1].more_chars, "unlucky?");
	strcpy(items_to_store[2].misc_chars, "Third");
	items_to_store[2].any_integer = 3;
	strcpy(items_to_store[2].more_chars, "baz");

	for (i = 0; i < ITEMS_USED; i++) {
		sprintf(key_to_use, "%c%c%d", items_to_store[i].misc_chars[0],
			items_to_store[i].more_chars[0],
			items_to_store[i].any_integer);
		key_datum.dptr = key_to_use;
		key_datum.dsize = strlen(key_to_use);
		data_datum.dptr = (void *)&items_to_store[i];
		data_datum.dsize = sizeof(struct test_data);

		result = dbm_store(dbm_ptr, key_datum, data_datum, DBM_REPLACE);
		if (result != 0) {
			fprintf(stderr, "Store fail on key %s\n", key_to_use);
			return -1;
		}
	}

	return 0;
}

int dbm_deleted(DBM * dbm_ptr)
{
	datum key_datum;
	char key_to_use[20];

	sprintf(key_to_use, "bu%d", 13);
	key_datum.dptr = key_to_use;
	key_datum.dsize = strlen(key_to_use);
	if (dbm_delete(dbm_ptr, key_datum) == 0)
		printf("Data with key %s deleted\n", key_to_use);
	else
		printf("Nothing deleted for key %s\n", key_to_use);

	return 0;
}

int dbm_fetched(DBM * dbm_ptr)
{
	char key_to_use[20];
	datum key_datum, data_datum;
	struct test_data item_retrieved;

	for (key_datum = dbm_firstkey(dbm_ptr); key_datum.dptr;
	     key_datum = dbm_nextkey(dbm_ptr)) {
		data_datum = dbm_fetch(dbm_ptr, key_datum);
		if (data_datum.dptr) {
			printf("Data retrieved\n");
			memcpy(&item_retrieved, data_datum.dptr,
			       data_datum.dsize);
			printf("Retrieve item-%s %d %s\n",
			       item_retrieved.misc_chars,
			       item_retrieved.any_integer,
			       item_retrieved.more_chars);
		} else {
			printf("No found for key %s\n", key_to_use);
		}
	}

	return 0;
}

int dbm_closed(DBM * dbm_ptr)
{
	dbm_close(dbm_ptr);

	return 0;
}

int main(int argc, char *argv[])
{
	DBM *dbm_ptr;

	dbm_initial(&dbm_ptr);
	dbm_stored(dbm_ptr);
	dbm_deleted(dbm_ptr);
	dbm_fetched(dbm_ptr);
	dbm_closed(dbm_ptr);

	return 0;
}
