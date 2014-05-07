/*
 * File: cvector.c
 * Author: Elizabeth Howe
 * ----------------------
 * The implentation for cvector.
 */

#include "cvector.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <search.h>

/* Type: struct CVectorImplementation
 * ----------------------------------
 * This definition completes the CVector type that was declared in
 * cvector.h. 
 */
struct CVectorImplementation {   
	int elemsz;
	int capacity; // number of allocated slots
	CleanupElemFn fn;
	void *base;
	int count;
};

#define CAPACITY_HINT 50

static struct CVectorImplementation *cv;

/*
 * Given an index of the vector, returns a pointer to that element.
 */
void *i2ptr(const CVector *cv, int index) 
{
	return (char *)(cv->base) + index * cv->elemsz;
}

CVector *cvec_create(int elemsz, int capacity_hint, CleanupElemFn fn)
{
	assert(capacity_hint >= 0);
	assert(elemsz > 0);
	cv = (struct CVectorImplementation *) malloc(sizeof(struct CVectorImplementation));
	assert(cv != NULL);
	cv->elemsz = elemsz;
	if (capacity_hint != 0) {
		cv->capacity = capacity_hint;
	}
	else {
		cv->capacity = CAPACITY_HINT; // use default
	}
	cv->fn = fn;
	cv->count = 0;
	cv->base = malloc(elemsz * cv->capacity);
	assert(cv->base != NULL);
	return cv;
}

void cvec_dispose(CVector *cv)
{
	if (cv->fn != NULL) {
		for (void *p = cvec_first(cv); p != NULL; p = cvec_next(cv, p)) {
			cv->fn(p);
		}
	}	
	free(cv->base);
	free(cv);
}

int cvec_count(const CVector *cv)
{
	return cv->count;
}

void *cvec_nth(const CVector *cv, int index)
{
	assert(index >= 0);
	assert(index < cv->count);
	return i2ptr(cv, index);
}

void cvec_insert(CVector *cv, const void *addr, int index)
{
	assert(index >= 0);
	assert(index <= cv->count);
	if (index == cv->count) { // appending
		int free_slots = cv->capacity - cv->count;
		if (free_slots == 0) { // need to double the capacity
			cv->capacity *= 2;
			cv->base = realloc(cv->base, cv->capacity * cv->elemsz);		
		}
		void *p1 = i2ptr(cv, index);
		memcpy(p1, addr, cv->elemsz);
	}
	else {
		int free_slots = cv->capacity - cv->count;
		int move_slots = cv->count - index; // number of elements to move up
		if (move_slots > free_slots) { // need to double the capacity
			cv->capacity *= 2;
			cv->base = realloc(cv->base, cv->capacity * cv->elemsz);		
		}
		void *p1 = i2ptr(cv, index + 1);
		void *p2 = i2ptr(cv, index);
		memmove(p1, p2, move_slots * cv->elemsz);
		memcpy(p2, addr, cv->elemsz);
	}
	cv->count++;
}

void cvec_append(CVector *cv, const void *addr)
{
	cvec_insert(cv, addr, cv->count);
}

void cvec_replace(CVector *cv, const void *addr, int index)
{
	assert(index >= 0);
	assert(index < cv->count);
	void *dest = i2ptr(cv, index);
	if (cv->fn != NULL) {
		cv->fn(dest);
	}
	memcpy(dest, addr, cv->elemsz);
}

void cvec_remove(CVector *cv, int index)
{
	assert(index >= 0);
	assert(index < cv->count);
	void *p1 = i2ptr(cv, index);
	if (cv->fn != NULL) {
		cv->fn(p1);
	}
	int move_slots = cv->count - index;
	void *p2 = i2ptr(cv, index + 1);
	if (move_slots > 0) { // need to move elements down
		memmove(p1, p2, move_slots * cv->elemsz);
	}
	cv->count--;
}
 
int cvec_search(const CVector *cv, const void *key, CompareFn cmp, int start, bool sorted)
{
	assert(start >= 0);
	assert(start <= cv->count);
	void *newBase = i2ptr(cv, start);
	int newCount = cv->count - start;
	void *ptr;
	if (sorted) {
		ptr = bsearch(key, newBase, newCount, cv->elemsz, cmp);
	}	
	else {
		ptr = lfind(key, newBase, (size_t *)&newCount, cv->elemsz, cmp);	
	}
	if (ptr == NULL) {
		return -1;
	}	
	return ((char *)(ptr) - (char *)(cv->base)) / (cv->elemsz);
}
void cvec_sort(CVector *cv, CompareFn cmp)
{
	qsort(cv->base, cv->count, cv->elemsz, cmp);
}

void *cvec_first(const CVector *cv)
{
	if (cv->count == 0) {
		return NULL;
	}
	return cv->base;
}

void *cvec_next(const CVector *cv, const void *prev)
{
	void *next = (char *)(prev) + cv->elemsz;
	if (next >= i2ptr(cv, cv->count)) { // no more elements
		return NULL;		
	}
	return next;
} 
