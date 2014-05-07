/*
 * File: cmap.c
 * Author: Elizabeth Howe
 * ----------------------
 * The implementation for cmap. 
 */


#include "cmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <string.h>

#define REHASH_LOAD 2  // When load >= constant, time to grow table
#define CAPACITY_HINT 1000

/* Type: struct CMapImplementation
 * -------------------------------
 * This definition completes the CMap type that was declared in
 * cmap.h.
 */
struct CMapImplementation {
	void **base;
	int valuesz;
	int nbucket;
	CleanupValueFn fn;
	int count;
};

static struct CMapImplementation *cm;

/* Function: hash
 * --------------
 * This function adapted from Eric Roberts' _The Art and Science of C_
 * It takes a string and uses it to derive a "hash code," which
 * is an integer in the range [0..nbuckets-1]. The hash code is computed
 * using a method called "linear congruence." A similar function using this
 * method is described on page 144 of Kernighan and Ritchie. The choice of
 * the value for the multiplier can have a significant effort on the
 * performance of the algorithm, but not on its correctness.
 * This hash function is case-sensitive, "ZELENSKI" and "Zelenski" are
 * not guaranteed to hash to same code.
 */
static int hash(const char *s, int nbuckets)
{
   const unsigned long MULTIPLIER = 2630849305L; // magic prime number
   unsigned long hashcode = 0;
   for (int i = 0; s[i] != '\0'; i++)  
      hashcode = hashcode * MULTIPLIER + s[i];  
   return hashcode % nbuckets;                                  
}

/*
 * Cell layout: 
 * value at the base;
 * next pointer;
 * key string
 */

/*
 * Returns cell next pointer.
 */
static void *cellNextGet(const void *cellBase) 
{
	const char *p = cellBase;
	p = p + cm->valuesz;
	return *(void **)p;
}

/*
 * Sets next pointer field to point to void *pnext.
 */
static void cellNextSet(void *cellBase, void *pnext) 
{
	char *p = cellBase;
	p = p + cm->valuesz;
	*(void **)(p) = pnext;
}

/*
 * Returns the pointer to the value field.
 */
static void *cellValp(const void *cellBase) 
{
	// keeping this as a separate function in case we change layout
	return (void *)(cellBase); 
}

/*
 * Returns pointer to key field, which we know to be a char *.
 */
static char *cellKeyp(const void *cellBase)
{	
	return (char *)(cellBase) + sizeof(void *) + cm->valuesz;
}

/*
 * Deallocate cell.
 */
static void cellFree(CMap *cm, void *p)
{
	if(cm->fn != NULL) {
		cm->fn(p);
	}
	free(p);
}

CMap *cmap_create(int valuesz, int capacity_hint, CleanupValueFn fn)
{
	assert(valuesz > 0);
	assert(capacity_hint >= 0);
	cm = (struct CMapImplementation *) malloc(sizeof(struct CMapImplementation));
	assert(cm != NULL);
	cm->valuesz = valuesz;
	if (capacity_hint != 0) {
		cm->nbucket = capacity_hint;
	}
	else {
		cm->nbucket = CAPACITY_HINT;
	}
	cm->fn = fn;
	cm->count = 0;
	// initialize all buckets to point to NULL
	cm->base = calloc(cm->nbucket, sizeof(void *));
	assert(cm->base != NULL);
	return cm;
}

void cmap_dispose(CMap *cm)
{
	for (const char *keyp = cmap_first(cm); keyp != NULL; keyp = cmap_next(cm, keyp)) {
		cmap_remove(cm, keyp);
	}
	free(cm->base);
	free(cm);
}

int cmap_count(const CMap *cm)
{
	return cm->count;
}

/*
 * Initialize new cell.
 */
static void *cellMake(CMap *cm, const char *key, const void *addr)
{
	int bytes = sizeof(void *) + strlen(key) + 1 + cm->valuesz;
	void *newCell =  malloc(bytes);
	assert(newCell != NULL);
	(void)strcpy(cellKeyp(newCell), key);
	void *newCellValp = cellValp(newCell);
	(void)memcpy(newCellValp, addr, cm->valuesz);
	cellNextSet(newCell, NULL);
	return newCell;
}

/*
 * Check to see if rehash is needed. Then rehash if necessary.
 */
static void rehash(CMap *cm)
{
	if ((cm->count/cm->nbucket) < REHASH_LOAD) {
		return;
	}
	// need to rehash
	int nbucket2 = cm->nbucket*2 + 1;
	void **base2 = calloc(nbucket2, sizeof(void *));
	assert(base2 != NULL);
	for (int i = 0; i < cm->nbucket; i++) {
		void *pnext;
		for (void *p = cm->base[i]; p != NULL; p = pnext) {
			pnext = cellNextGet(p); 
			int newSlot = hash(cellKeyp(p), nbucket2); 
			cellNextSet(p, base2[newSlot]); // prepend
			base2[newSlot] = p; 
		}
	}
	cm->nbucket = nbucket2;
	free(cm->base);
	cm->base = base2;
}

void cmap_put(CMap *cm, const char *key, const void *addr)
{
	void *newCell = cellMake(cm, key, addr);
	int slot = hash(key, cm->nbucket);
	if (cm->base[slot] == NULL) { // if bucket is NULL
		cm->base[slot] = newCell;
		cm->count++;
		rehash(cm);
		return;
	}
	void *p = cm->base[slot];
	char *keyp = cellKeyp(p); // now we can dereference
	if (strcmp(keyp, key) == 0) { // first element is a match
		void *pnext = cellNextGet(p);
		cellNextSet(newCell, pnext);	
		cellFree(cm, p);
		cm->base[slot] = newCell;
		return;
	}
	for (; cellNextGet(p) != NULL; p = cellNextGet(p)) {
		void *pnext = cellNextGet(p);
		if (strcmp(cellKeyp(pnext), key) == 0) { // get the key from the next
			void *pnextnext = cellNextGet(pnext);
			cellNextSet(newCell, pnextnext);
			cellFree(cm, pnext);
			cellNextSet(p, newCell);
			return;			
		}	
	}
	// if we are here, then no match found, need to prepend
	p = cm->base[slot]; // reset p
	cellNextSet(newCell, p);
	cm->base[slot] = newCell;
	cm->count++;
	rehash(cm);
}

void *cmap_get(const CMap *cm, const char *key)
{
	int slot = hash(key, cm->nbucket);
	for (void *p = cm->base[slot]; p != NULL; p = cellNextGet(p)) {
		if (strcmp(cellKeyp(p), key) == 0) {
			return cellValp(p);	
		}
	}
	return NULL;
}

void cmap_remove(CMap *cm, const char *key)
{
	int slot = hash(key, cm->nbucket);
	if (cm->base[slot] == NULL) {
		return;
	}
	void *p = cm->base[slot];
	char *keyp = cellKeyp(p);
	if (strcmp(keyp, key) == 0) { // if we want to remove the first element
		void *pnext = cellNextGet(p);
		cm->base[slot] = pnext;
		cellFree(cm, p);
		cm->count--;
		return;
	}
	for (; cellNextGet(p) != NULL; p = cellNextGet(p)) {
		void *pnext = cellNextGet(p);
		if (strcmp(cellKeyp(pnext), key) == 0) { // get the key from next
			void *pnextnext = cellNextGet(pnext);
			cellNextSet(p, pnextnext);
			cellFree(cm, pnext);
			cm->count--;
			return;
		}
	}
}

const char *cmap_first(const CMap *cm)
{
	for (int i = 0; i < cm->nbucket; i++) {
		void *p = cm->base[i];
		if (p != NULL) {
			return cellKeyp(p);
		}
	} 
	return NULL;
}

const char *cmap_next(const CMap *cm, const char *prevkey) 
{
	void *pnext = cellNextGet(prevkey - sizeof(void *) - cm->valuesz);
	if (pnext != NULL) {
		return cellKeyp(pnext);	
	}
	// no more elements in the bucket
	int slot = hash(prevkey, cm->nbucket);
	for (int i = slot + 1; i < cm->nbucket; i++) {
		if (cm->base[i] != NULL) {
			return cellKeyp(cm->base[i]);
		}
	}
	return NULL;
}
