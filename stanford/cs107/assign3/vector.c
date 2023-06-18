/*
* This is an assignment from a CS107 Stanford course (Programming paradigms).
* Implementation of vector in C.
* By Elizabeth Howe 
*/

#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static const int default_alloc_length = 10;
void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation) {
    assert(initialAllocation >= 0);
    assert(elemSize > 0);
    v->logical_length = 0;
    if (initialAllocation == 0) {
        v->alloc_length = default_alloc_length;
    }
    else {
        v->alloc_length = initialAllocation;
    }
    v->elem_size = elemSize;
    v->grow_by_length = v->alloc_length;
    v->elems = malloc(v->alloc_length * v->elem_size);
    assert(v->elems != NULL);
    v->free_fun = freeFn;
}

void VectorDispose(vector *v) {
    if (v->free_fun == NULL) {
        free(v->elems);
    }
    else {
        for (int i = 0; i < v->logical_length; i++) {
            char *p = (char *)v->elems + i * v->elem_size;
            (v->free_fun)(p);
        }
    }
}

int VectorLength(const vector *v) {
    return v->logical_length;
}

void *VectorNth(const vector *v, int position) {
    assert(position >= 0 && position <= v->logical_length);
    return (char *)v->elems + position * v->elem_size;
}

void VectorReplace(vector *v, const void *elemAddr, int position) {
    assert(position >= 0 && position < v->logical_length);
    char *to_replace = (char *)v->elems + position * v->elem_size;
    if (v->free_fun != NULL) {
        (v->free_fun)(to_replace);
    }
    memcpy(to_replace, elemAddr, v->elem_size);
}

static void vector_grow(vector *v) {
    v->alloc_length += v->grow_by_length;
    v->elems = realloc(v->elems, v->elem_size * v->alloc_length);
}

void VectorInsert(vector *v, const void *elemAddr, int position) {
    assert(position >= 0 && position <= v->logical_length);
    if (v->logical_length == v->alloc_length) {
        vector_grow(v);
    }
    char *to_insert = (char *)v->elems + position * v->elem_size;
    char *rest = (char *)v->elems + (position + 1) * v->elem_size;
    int move_size = (v->logical_length - position) * v->elem_size;
    memmove(rest, to_insert, move_size);
    memcpy(to_insert, elemAddr, v->elem_size);
    v->logical_length += 1;
}

void VectorAppend(vector *v, const void *elemAddr) {
    if (v->logical_length == v->alloc_length) {
        vector_grow(v);
    }
    char *to_append = (char *)v->elems + v->elem_size * v->logical_length;
    memcpy(to_append, elemAddr, v->elem_size);
    v->logical_length += 1;
}

void VectorDelete(vector *v, int position) {
    assert(position >= 0 && position < v->logical_length);
    char *to_delete = (char *)v->elems + v->elem_size * position;
    if (v->free_fun != NULL) {
        (v->free_fun)(to_delete);
    }
    char *rest = (char *)v->elems + v->elem_size * (position + 1);
    int move_size = (v->logical_length - (position + 1)) * v->elem_size;
    memmove(to_delete, rest, move_size);
    v->logical_length -= 1;
}

void VectorSort(vector *v, VectorCompareFunction compare) {
    assert(compare != NULL);
    qsort(v->elems, v->logical_length, v->elem_size, compare);
}

void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData) {
    assert(mapFn != NULL);
    for (int i = 0; i < v->logical_length; i++) {
        char *p = (char *)v->elems + i * v->elem_size;
        mapFn(p, auxData);
    }
}

void *my_lfind(const void *key, const void *base, size_t num, size_t width, 
                VectorCompareFunction fn) {
    for (int i = 0; i < num; i++) {
        char *p = (char *)base + i * width;
        if (fn(key, p) == 0) {
            return p;
        }
    }
    return NULL;
}

static const int kNotFound = -1;
int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted) {
    assert(key != NULL);
    assert(searchFn != NULL);
    assert(startIndex >= 0);
    if (v->logical_length == 0) {
        assert(startIndex == 0);
    }
    else {
        assert(startIndex < v->logical_length);
    }

    char *start_p = (char *)(v->elems) + startIndex * v->elem_size;
    void *ret;
    if (isSorted) {
        ret = bsearch(key, start_p, v->logical_length - startIndex, v->elem_size, searchFn);
    }
    else {
        ret = my_lfind(key, start_p, v->logical_length - startIndex, v->elem_size, searchFn);
    }
    if (ret == NULL) {
        return kNotFound;
    }
    else {
        int bytes_offset = (char *)ret - (char *)v->elems;
        return bytes_offset / v->elem_size;
    }
}
