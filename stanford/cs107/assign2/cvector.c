/*
 * Implementation of a vector in C.
 * Internally, the elemnts of the vector are stored contiguously in an
 * array for constant time random access.
 *
 * Author:
 * Elizabeth Howe
 *
 * Reference:
 * Stanford CS107
 */

#include "cvector.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <search.h>

static const int default_internal_length = 16;

/* Define the CVector internals */
typedef struct CVector_internals {
    void *elems; // pointer to contiguous memory in the heap
    size_t logical_length; // current number of elements in the array
    size_t internal_length; // current max num of elements the array can have
    size_t elem_size;
    free_fun clean; // cleanup function
} CVector;


CVector *cvec_create(size_t elem_size, size_t capacity_hint, free_fun fn)
{
    assert(elem_size != 0);

    CVector *cv = malloc(sizeof(CVector));
    assert(cv != NULL);

    cv->elem_size = elem_size;
    cv->internal_length = capacity_hint == 0 ?
                            default_internal_length : capacity_hint;
    cv->logical_length = 0;
    cv->clean = fn;
    cv->elems = calloc(capacity_hint, elem_size);
    assert(cv->elems != NULL);
    return cv;
}

void cvec_dispose(CVector *cv)
{
    int i;
    char *p;

    if(cv->clean != NULL) {
        for(i = 0; i < cv->logical_length; i++) {
            p = (char *)cv->elems + i * cv->elem_size;
            (cv->clean)(p);
            p = NULL;
        }
    }
    free(cv->elems);
    free(cv);
}

int cvec_count(const CVector *cv)
{
    return cv->logical_length;
}

void *cvec_nth(const CVector *cv, int index)
{
    assert(index >= 0 && index <= (cv->logical_length - 1));
    return (char *)cv->elems + index * cv->elem_size;
}

/* Double the internal_length */
static void cvec_grow(CVector *cv)
{
    cv->internal_length *= 2;
    cv->elems = realloc(cv->elems, cv->elem_size * cv->internal_length);
    assert(cv->elems != NULL);
}

void cvec_insert(CVector *cv, const void *addr, int index)
{
    char *to_insert;
    char *post_insert;
    size_t move_size;

    assert(index >= 0 && index <= cv->logical_length);

    if(cv->logical_length == cv->internal_length) {
        cvec_grow(cv);
    }
    to_insert = (char *)cv->elems + index * cv->elem_size;
    post_insert = (char *)cv->elems + (index + 1) * cv->elem_size;
    move_size = (cv->logical_length - index) * cv->elem_size;
    memmove(post_insert, to_insert, move_size);
    memcpy(to_insert, addr, cv->elem_size);
    cv->logical_length++;
}

void cvec_append(CVector *cv, const void *addr)
{
    char *to_append;

    if (cv->logical_length == cv->internal_length) {
        cvec_grow(cv);
    }
    to_append = (char *)cv->elems + cv->elem_size * cv->logical_length;
    memcpy(to_append, addr, cv->elem_size);
    cv->logical_length++;
}

void cvec_elem_replace(CVector *cv, const void *addr, int index)
{
    char *to_replace;

    assert(index >= 0 && index < cv->logical_length);

    to_replace = (char *)cv->elems + index * cv->elem_size;
    if (cv->clean != NULL) {
        (cv->clean)(to_replace);
    }
    memcpy(to_replace, addr, cv->elem_size);
}

void cvec_elem_remove(CVector *cv, int position)
{
    char *to_delete;
    char *post_delete;
    int move_size;

    assert(position >= 0 && position < cv->logical_length);

    to_delete = (char *)cv->elems + cv->elem_size * position;
    if (cv->clean != NULL) {
        (cv->clean)(to_delete);
    }
    post_delete = (char *)cv->elems + cv->elem_size * (position + 1);
    move_size = (cv->logical_length - (position + 1)) * cv->elem_size;
    memmove(to_delete, post_delete, move_size);
    cv->logical_length -= 1;
}

 /*
  * If sorted, use binary search for O(logn) time.
  * Else, use linear scan for O(n) time.
  */
int cvec_search(const CVector *cv, const void *key, comp_fun cmp,
                int start, bool sorted)
{
    char *start_p;
    void *ret;
    size_t nmemb;
    int bytes_offset;

    assert(key != NULL);
    assert(cmp != NULL);
    assert(start >= 0);
    if (cv->logical_length == 0) {
        assert(start == 0);
    }
    else {
        assert(start < cv->logical_length);
    }

    start_p = (char *)(cv->elems) + start * cv->elem_size;
    nmemb = cv->logical_length - start;
    if (sorted) {
        ret = bsearch(key, start_p, nmemb, cv->elem_size, cmp);
    }
    else {
        ret = lfind(key, start_p, &nmemb, cv->elem_size, cmp);
    }

    if (ret == NULL) {
        return -1; // not found
    }
    bytes_offset = (char *)ret - (char *)cv->elems;
    return bytes_offset / cv->elem_size;
}

void cvec_sort(CVector *cv, comp_fun cmp)
{
    assert(cmp != NULL);
    qsort(cv->elems, cv->logical_length, cv->elem_size, cmp);
}

void *cvec_first(const CVector *cv)
{
    if(cv->logical_length == 0) {
        return NULL;
    }
    else return cv->elems;
}

void *cvec_next(const CVector *cv, const void *prev)
{
    char *last_elemp;

    last_elemp = (char *)cv->elems + cv->elem_size * (cv->logical_length - 1);
    if(prev == last_elemp) {
        return NULL;
    }
    return (char *)prev + cv->elem_size;
}