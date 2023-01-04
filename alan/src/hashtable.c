/**
 * @file    hashtable.c
 * @brief   A generic hash table.
 * @author  W.H.K. Bester (whkbester@cs.sun.ac.za)
 * @date    2022-08-03
 */
#include "hashtable.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFFER_SIZE 1024
#define INITIAL_DELTA_INDEX 4
#define PRINT_BUFFER_SIZE 1024

/** an entry in the hash table */
typedef struct htentry HTentry;
struct htentry {
	void *key;         /*<< the key                      */
	void *value;       /*<< the value                    */
	HTentry *next_ptr; /*<< the next entry in the bucket */
};
typedef struct {
	char *id;
	int num;
} Name;
/** a hash table container */
struct hashtab {
	/** a pointer to the underlying table                              */
	HTentry **table;
	/** the current size of the underlying table                       */
	unsigned int size;
	/** the current number of entries                                  */
	unsigned int num_entries;
	/** the maximum load factor before the underlying table is resized */
	float max_loadfactor;
	/** the index into the delta array                                 */
	unsigned short idx;
	/** a pointer to the hash function                                 */
	unsigned int (*hash)(void *, unsigned int);
	/** a pointer to the comparison function                           */
	int (*cmp)(void *, void *);
};

unsigned int hash(void *key, unsigned int size);
int scmp(void *v1, void *v2);
void val2str(void *key, void *value, char *buffer);

/* --- function prototypes -------------------------------------------------- */
static int getsize(HashTab *ht);
static HTentry **talloc(int tsize);
static void rehash(HashTab *ht);
static void freekeys(void *k);
static void freevals(void *k);

/** the array of differences between a power-of-two and the largest prime less
 * than that power-of-two.                                                    */
unsigned short delta[] = {0,  0, 1, 1, 3, 1, 3, 1,  5, 3,  3, 9,  3,  1, 3,  19,
						  15, 1, 5, 1, 3, 9, 3, 15, 3, 39, 5, 39, 57, 3, 35, 1};

#define MAX_IDX (sizeof(delta) / sizeof(unsigned short))

/* --- hash table interface ------------------------------------------------- */

HashTab *ht_init(float loadfactor, unsigned int (*hash)(void *, unsigned int),
				 int (*cmp)(void *, void *))
{
	unsigned int i;
	i = 0;
	// allocate space for a HashTab variable,
	HashTab *ht = (HashTab *)malloc(sizeof(HashTab));
	ht->num_entries = 0;
	ht->idx = INITIAL_DELTA_INDEX;
	ht->size = getsize(ht);
	ht->table = (HTentry **)calloc(ht->size, sizeof(HTentry *));
	ht->max_loadfactor = loadfactor;
	ht->hash = hash;
	ht->cmp = cmp;

	if (ht->table == NULL) {
		free(ht->table);
		// Initialization failed.
		return NULL;
	}

	unsigned int r;
	// set all table values null
	for (r = 0; r < ht->size; r++) {
		ht->table[r] = NULL;
	}

	return ht;
}

int ht_insert(HashTab *ht, void *key, void *value)
{
	int k;
	HTentry *p;

	unsigned int N = ht->size;
	unsigned int ne;
	float current_loadfactor;
	p = calloc(1, sizeof(HTentry));
	talloc(1);
	p->key = key;
	p->value = value;
	k = ht->hash(key, ht->size);

	// if key found
	if (ht->table[k]) {
		HTentry *next;
		HTentry *prev = NULL;
		/* theres already something in the index*/
		next = ht->table[k];
		// use for loop to find next free position

		while (next) {
			prev = next;
			next = next->next_ptr;
		}
		ht->num_entries++;
		prev->next_ptr = p;
		p->next_ptr = NULL;
	} else {
		ht->table[k] = p;
		p->next_ptr = NULL;
		ht->num_entries++;
	}
	ne = ht->num_entries;
	current_loadfactor = (float)ne / (float)N;
	if (current_loadfactor > ht->max_loadfactor) {
		rehash(ht);
	}
	return EXIT_SUCCESS;
}

Boolean ht_search(HashTab *ht, void *key, void **value)
{
	int k;
	HTentry *p;

	k = ht->hash(key, ht->size);
	for (p = ht->table[k]; p; p = p->next_ptr) {
		if (ht->cmp(key, p->key) == 0) {
			*value = p->value;
			break;
		}
	}

	return (p ? TRUE : FALSE);
}

int ht_free(HashTab *ht, void (*freekey)(void *k), void (*freeval)(void *v))
{
	unsigned int i;
	HTentry *p;
	/* free the table and container */

	for (i = 0; i < ht->size; i++) {
		if (ht->table[i]) {
			// freekey(ht->table[i]->key);
			// freeval(ht->table[i]->value);
			p = ht->table[i];
			while (p) {
				freekey(p->key);
				freeval(p->value);
				p = p->next_ptr;
			}

			free(ht->table[i]);
		}
	}
	free(ht->table);
	free(ht);
	return EXIT_SUCCESS;
}

void ht_print(HashTab *ht, void (*keyval2str)(void *k, void *v, char *b))
{
	unsigned int i;
	HTentry *p;
	char buffer[PRINT_BUFFER_SIZE];

	for (i = 0; i < ht->size; i++) {
		printf("bucket[%2i]", i);
		for (p = ht->table[i]; p != NULL; p = p->next_ptr) {
			keyval2str(p->key, p->value, buffer);
			printf(" --> %s", buffer);
		}
		printf(" --> NULL\n");
	}
}

/* --- utility functions ---------------------------------------------------- */

static int getsize(HashTab *ht)
{
	int k, l, new_size, result;
	result = 1;
	k = ht->idx + 1;
	l = ht->idx + 1;
	;

	// power function
	while (k != 0) {
		result *= 2;
		--k;
	}
	ht->idx = l;
	new_size = result - delta[l];
	return new_size;
}

static HTentry **talloc(int tsize)
{
	tsize = 0;
	return NULL;
}

static void rehash(HashTab *ht)
{
	/* store reference to the old table */

	HTentry **new_table, **pp, *this, **qq;
	unsigned int i;
	unsigned int newidx, new_size, old_size, oldidx;

	old_size = ht->size;
	new_size = getsize(ht);

	new_table = (HTentry **)calloc(new_size, sizeof(HTentry *));

	/* nullify the new table */
	for (i = 0; i < new_size; i++) {
		new_table[i] = NULL;
	}

	for (oldidx = 0; oldidx < ht->size; oldidx++) {
		for (pp = &ht->table[oldidx]; *pp;) {
			this = *pp;
			*pp = this->next_ptr;
			this->next_ptr = NULL;

			newidx = ht->hash(this->key, new_size);
			for (qq = &new_table[newidx]; *qq; qq = &(*qq)->next_ptr) {
			}

			*qq = this;
		}
	}

	ht_free(ht, freekeys, freevals);
	ht->table = new_table;
	ht->size = new_size;
}
void freekeys(void *k) { free(k); }

void freevals(void *k) { free(k); }

