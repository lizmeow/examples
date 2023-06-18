/*
* This is an assignment from a CS107 Stanford course (Programming paradigms).
* Implementation of hashset in C.
* By Elizabeth Howe 
**/

#include "hashset.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void HashSetNew(hashset *h, int elemSize, int numBuckets,
		HashSetHashFunction hashfn, HashSetCompareFunction comparefn, HashSetFreeFunction freefn) {
	assert(elemSize > 0);
	assert(numBuckets > 0);
	assert(hashfn != NULL);
	assert(comparefn !=NULL);
	h->n_buckets = numBuckets;
	h->count = 0;
	h->hash_fun = hashfn;
	h->comp_fun = comparefn;
	h->array = (vector *)malloc(h->n_buckets * sizeof(vector));
	assert(h->array != NULL);
	for (int i = 0; i < h->n_buckets; i++) {
		VectorNew(h->array + i, elemSize, freefn, 0);
	}
}

void HashSetDispose(hashset *h) {
	for (int i = 0; i < h->n_buckets; i++) {
		VectorDispose(h->array + i);
	}
	free(h->array);
}

int HashSetCount(const hashset *h) {
	return h->count;
}

void HashSetMap(hashset *h, HashSetMapFunction mapfn, void *auxData) {
	assert (mapfn != NULL);
	for (int i = 0; i < h->n_buckets; i++) {
		VectorMap(h->array + i, mapfn, auxData);
	}
}

void HashSetEnter(hashset *h, const void *elemAddr) {
	assert(elemAddr != NULL);
	int bucket = (h->hash_fun)(elemAddr, h->n_buckets);
	assert(bucket >= 0 && bucket < h->n_buckets);
	int position = VectorSearch(h->array + bucket, elemAddr, h->comp_fun, 0, true);
	if (position == -1) {
		VectorAppend(h->array + bucket, elemAddr);
		h->count += 1;
		VectorSort(h->array + bucket, h->comp_fun);
	}
	else {
		VectorReplace(h->array + bucket, elemAddr, position);
	}
}

void *HashSetLookup(const hashset *h, const void *elemAddr) {
	assert(elemAddr != NULL);
	int bucket = (h->hash_fun)(elemAddr, h->n_buckets);
	assert(bucket >= 0 && bucket < h->n_buckets);
	int position = VectorSearch(h->array + bucket, elemAddr, h->comp_fun, 0, true);
	if (position == -1) {
		return NULL;
	}
	else {
		return VectorNth(h->array + bucket, position);
	}
}
