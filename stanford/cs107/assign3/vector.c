/* 
* Implementation of vector in C.
* Internally, the elements of the vector are stored contiguously in an
* array for constant time random access.
* 
* Author:
* Elizabeth Howe 
*/

#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <search.h>

static const int default_internal_length = 10;

/* O(1) */
void vector_new(vector *v, int elem_size, 
                vector_free_fun free_fun, int n_elems_grow_by) {
    assert(n_elems_grow_by >= 0);
    assert(elem_size > 0);
    
    v->logical_length = 0;
    v->internal_length = n_elems_grow_by == 0 ? 
                            default_internal_length : n_elems_grow_by;
    v->elem_size = elem_size;
    v->n_elems_grow_by = v->internal_length;
    v->elems = malloc(v->internal_length * v->elem_size);
    assert(v->elems != NULL);
    v->free_fun = free_fun;
}

/* O(n) */
void vector_dispose(vector *v) {
    int i;
    char *p;
    
    if (v->free_fun != NULL) {
        for (i = 0; i < v->logical_length; i++) {
            p = (char *)v->elems + i * v->elem_size;
            (v->free_fun)(p);
        }
    }
    free(v->elems);
}

/* O(1) */
int vector_length(const vector *v) {
    return v->logical_length;
}

/* O(1) */
void *vector_nth(const vector *v, int position) {
    assert(position >= 0 && position <= v->logical_length);
    return (char *)v->elems + position * v->elem_size;
}

/* O(1) */
void vector_elem_replace(vector *v, const void *elem_addr, int position) {
    char *to_replace;
    
    assert(position >= 0 && position < v->logical_length);
    
    to_replace = (char *)v->elems + position * v->elem_size;
    if (v->free_fun != NULL) {
        (v->free_fun)(to_replace);
    }
    memcpy(to_replace, elem_addr, v->elem_size);
}

static void vector_grow(vector *v) {
    v->internal_length += v->n_elems_grow_by;
    v->elems = realloc(v->elems, v->elem_size * v->internal_length);
}

/* O(n) neglecting any memoary reallocation time */
void vector_insert(vector *v, const void *elem_addr, int position) {
    char *to_insert;
    char *post_insert;
    int move_size;
    
    assert(position >= 0 && position <= v->logical_length);
    
    if (v->logical_length == v->internal_length) {
        vector_grow(v);
    }
    to_insert = (char *)v->elems + position * v->elem_size;
    post_insert = (char *)v->elems + (position + 1) * v->elem_size;
    move_size = (v->logical_length - position) * v->elem_size;
    memmove(post_insert, to_insert, move_size);
    memcpy(to_insert, elem_addr, v->elem_size);
    v->logical_length += 1;
}

/* O(1) neglecting any memory reallocation time */
void vector_append(vector *v, const void *elem_addr) {
    char *to_append;

    if (v->logical_length == v->internal_length) {
        vector_grow(v);
    }
    to_append = (char *)v->elems + v->elem_size * v->logical_length;
    memcpy(to_append, elem_addr, v->elem_size);
    v->logical_length++;
}

/* O(n) */
void vector_elem_delete(vector *v, int position) {
    char *to_delete;
    char *post_delete;
    int move_size;

    assert(position >= 0 && position < v->logical_length);
    
    to_delete = (char *)v->elems + v->elem_size * position;
    if (v->free_fun != NULL) {
        (v->free_fun)(to_delete);
    }
    post_delete = (char *)v->elems + v->elem_size * (position + 1);
    move_size = (v->logical_length - (position + 1)) * v->elem_size;
    memmove(to_delete, post_delete, move_size);
    v->logical_length -= 1;
}

/* O(nlogn) */
void vector_sort(vector *v, vector_cmp_fun compare) {
    assert(compare != NULL);
    qsort(v->elems, v->logical_length, v->elem_size, compare);
}

/* O(n) */
void vector_map(vector *v, vector_map_fun map_fun, void *aux_data)
 {
    int i;
    char *elem_addr;

    assert(map_fun != NULL);
    
    for (i = 0; i < v->logical_length; i++) {
        elem_addr = (char *)v->elems + i * v->elem_size;
        map_fun(elem_addr, aux_data);
    }
}

/* If sorted, O(logn) using binary search.
* Else, O(n) using linear scan.
*/
int vector_search(const vector *v, const void *key, vector_cmp_fun search_fun, 
                    int start_index, bool is_sorted)
{
    char *start_p;
    void *ret;
    size_t nmemb;   
    int bytes_offset;

    assert(key != NULL);
    assert(search_fun != NULL);
    assert(start_index >= 0);
    if (v->logical_length == 0) {
        assert(start_index == 0);
    }
    else {
        assert(start_index < v->logical_length);
    }

    start_p = (char *)(v->elems) + start_index * v->elem_size;
    nmemb = v->logical_length - start_index;
    if (is_sorted) {
        ret = bsearch(key, start_p, nmemb, v->elem_size, search_fun);
    }
    else {
        ret = lfind(key, start_p, &nmemb, v->elem_size, search_fun);
    }

    if (ret == NULL) {
        return -1; // not found
    }
    bytes_offset = (char *)ret - (char *)v->elems;
    return bytes_offset / v->elem_size;
}