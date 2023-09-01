/*
* Author:
* Elizabeth Howe
*/

/* The internal representation of the vector */
typedef struct {
  void *elems; // pointer to the underlying array
  int elem_size;
  int logical_length; // current number of elements in the array
  int internal_length; // current max number of elements the array can have
  int n_elems_grow_by;
  void (*free_fun)(void *);
} vector;