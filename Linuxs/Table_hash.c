/******************************************************************************
 * Function:	A example of hash operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01				  		  
 * Complie:	gcc -Wall -g Table_hash.c -o Table_hash
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define  MIN_STR_LEN 	16
#define  MAX_STR_LEN 	64
#define  MAX_HASH_LEN 	10000

typedef struct hashnode {
	char *Key;
	int Value;
	struct hashnode *Next;
} HashNode;

int hash_table_size;
HashNode *hashTable[MAX_HASH_LEN];

void init_HashTable()
{
	hash_table_size = 0;
	memset(hashTable, 0, sizeof(HashNode *) * MAX_HASH_LEN);
}

unsigned int Str_Hash(const char *skey)
{
	const char *p = (const char *)skey;
	unsigned int h = *p;

	if (h) {
		for (p += 1; *p != '\0'; ++p)
			h = (h << 5) - h + *p;
	}

	return h;
}

void insert_HashTable(const char *skey, int nvalue)
{
	int pos;
	HashNode *pHead = NULL;
	HashNode *pNewNode = NULL;

	if (hash_table_size >= MAX_HASH_LEN)
		return;

	pos = Str_Hash(skey) % MAX_HASH_LEN;
	pHead = hashTable[pos];
	while (pHead) {
		if (strcmp(pHead->Key, skey) == 0) {
			printf("%s already exists!\n", skey);
			return;
		}
		pHead = pHead->Next;
	}

	pNewNode = (HashNode *) malloc(sizeof(HashNode));
	if (pNewNode == NULL)
		return;
	memset(pNewNode, 0, sizeof(HashNode));

	pNewNode->Key = (char *)malloc(sizeof(char) * (strlen(skey) + 1));
	if (pNewNode->Key == NULL)
		return;
	memset(pNewNode->Key, 0, sizeof(char) * (strlen(skey) + 1));

	strcpy(pNewNode->Key, skey);
	pNewNode->Value = nvalue;
	pNewNode->Next = hashTable[pos];
	hashTable[pos] = pNewNode;
	hash_table_size++;
}

void delete_HashTable(const char *skey)
{
	HashNode *pHead = NULL;
	HashNode *pLast = NULL;
	HashNode *pRemove = NULL;

	unsigned int pos = Str_Hash(skey) % MAX_HASH_LEN;
	if (hashTable[pos]) {
		pHead = hashTable[pos];
		while (pHead) {
			if (strcmp(skey, pHead->Key) == 0) {
				pRemove = pHead;
				break;
			}
			pLast = pHead;
			pHead = pHead->Next;
		}

		if (pRemove) {
			if (pLast)
				pLast->Next = pRemove->Next;
			else
				hashTable[pos] = NULL;

			free(pRemove->Key);
			free(pRemove);
		}
	}
}

HashNode *search_HashTable(const char *skey)
{
	HashNode *pHead = NULL;
	unsigned int pos = Str_Hash(skey) % MAX_HASH_LEN;

	if (hashTable[pos]) {
		pHead = hashTable[pos];
		while (pHead) {
			if (strcmp(skey, pHead->Key) == 0)
				return pHead;
			else
				pHead = pHead->Next;
		}
	}

	return NULL;
}

void show_HashTable()
{
	int i;
	HashNode *pHead = NULL;

	for (i = 0; i < MAX_HASH_LEN; ++i) {
		if (hashTable[i]) {
			pHead = hashTable[i];
			printf("[key-%d]\t", i);
			while (pHead) {
				printf("%s: %d", pHead->Key, pHead->Value);
				pHead = pHead->Next;
			}
			printf("\n");
		}
	}
}

void clean_HashTable()
{
	int i;
	HashNode *pHead = NULL;
	HashNode *pTemp = NULL;

	for (i = 0; i < MAX_HASH_LEN; ++i) {
		if (hashTable[i]) {
			pHead = hashTable[i];
			while (pHead) {
				pTemp = pHead;
				pHead = pHead->Next;
				if (pTemp) {
					free(pTemp->Key);
					free(pTemp);
				}
			}
		}
	}
}

int main(int argc, char **argv)
{
	int size = 0;
	char str[MAX_STR_LEN];
	char skey[MAX_STR_LEN];
	char dkey[MAX_STR_LEN];
	HashNode *pNode = NULL;

	srand(time(NULL));

	printf("Init HashTable start, input HashTable size: ");
	scanf("%d", &size);
	printf("Init HashTable and insert [strlen < 20]:\n");
	init_HashTable();

	while (size--) {
		scanf("%s", str);
		insert_HashTable(str, size);
	}

	printf("\nShow HashTable:\n");
	show_HashTable();

	printf("\nInput the search key: ");
	scanf("%s", skey);
	pNode = search_HashTable(skey);
	if (pNode == NULL)
		printf("No found key[%s]!\n", skey);
	else
		printf("Key[%s] value: %d\n", skey, pNode->Value);

	printf("\nInput the delete key: ");
	scanf("%s", dkey);
	delete_HashTable(dkey);
	printf("After remove %s:\n", dkey);
	show_HashTable();

	clean_HashTable();
	printf("\nDestory Hash finish and quit!\n");

	return 0;
}
