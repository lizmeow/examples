/*
* Implementation of hashset in C.
* Internally, the hashset is an array of buckets where collisions are
* resolved by chaining.
* All elements hashing to the same bucket are stored in a vector.
*
* Alternative approach: 
* 1. All elements hashing to the same bucket could 
* be stored in a self-balanced tree for better performance, but for 
* simplicity, this hashset implementation layers on top of my vector 
* implementation. 
* 2. The load factor could be specified by the client to control 
* space-time tradeoff. 
*
* Author:
* Elizabeth Howe 
*/

#include "hashset.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*
* O(B)
* where B is the number of buckets
*/
void hashset_new(hashset *h, int elem_size, int num_buckets,
	             hashset_hash_fun hash_fun,
                 hashset_cmp_fun cmp_fun,
                 hashset_free_fun free_fun)
{
    int i;

	assert(elem_size > 0);
	assert(num_buckets > 0);
	assert(hash_fun != NULL);
	assert(cmp_fun != NULL);
	
    h->n_buckets = num_buckets;
	h->count = 0;
	h->hash_fun = hash_fun;
	h->comp_fun = cmp_fun;
	h->array = (vector *)malloc(h->n_buckets * sizeof(vector));
	assert(h->array != NULL);
	for (i = 0; i < h->n_buckets; i++) {
		vector_new(&h->array[i], elem_size, free_fun, 0);
	}
}

/*
* O(N + B)
* N = number of elements hashed
* B = number of buckets
*/
void hashset_dispose(hashset *h)
{
    int i;

	for (i = 0; i < h->n_buckets; i++) {
		vector_dispose(&h->array[i]);
	}
	free(h->array);
}

/* O(1) */
int hashset_count(const hashset *h)
{
	return h->count;
}

/* O(N + B)
* N = number of elements hashed
* B = number of buckets
*/
void hashset_map(hashset *h, hashset_map_fun mapfn, void *aux_data)
{
    int i;

	assert (mapfn != NULL);

	for (i = 0; i < h->n_buckets; i++) {
		vector_map(&h->array[i], mapfn, aux_data);
	}
}

/* 
* Expected: O(N / B), assuming hash fun evenly distributes elements 
* N = number of elements hashed
* B = number of buckets
*/
void hashset_enter(hashset *h, const void *elem_addr)
{
    int vec_position;
    int bucket;

	bucket = (h->hash_fun)(elem_addr, h->n_buckets);
	assert(bucket >= 0 && bucket < h->n_buckets);

	vec_position = vector_search(&h->array[bucket], elem_addr, h->comp_fun, 0, true);
	if (vec_position == -1) {
		vector_append(&h->array[bucket], elem_addr);
		h->count++;
		vector_sort(&h->array[bucket], h->comp_fun);
	}
	else {
		vector_elem_replace(&h->array[bucket], elem_addr, vec_position);
	}
}

/* 
* Expected: O(log(N / B)), assuming hash fun evenly distributes elements 
* N = number of elements hashed
* B = number of buckets
*/
void *hashset_lookup(const hashset *h, const void *elem_addr)
{
    int vec_position;
    int bucket;

	bucket = (h->hash_fun)(elem_addr, h->n_buckets);
	assert(bucket >= 0 && bucket < h->n_buckets);

	vec_position = vector_search(&h->array[bucket], elem_addr, h->comp_fun, 0, true);
	if (vec_position == -1) {
		return NULL;
	}

    return vector_nth(&h->array[bucket], vec_position);
}