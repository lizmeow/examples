/*
* Author:
* Elizabeth Howe
*/

/* The internal representation of a hashset */
typedef struct {
    vector *array;
    int n_buckets; // size of the array
    int count; // number of elements that have been hashed 
    int (*comp_fun)(const void *, const void *);
    int (*hash_fun)(const void *, int);
} hashset;