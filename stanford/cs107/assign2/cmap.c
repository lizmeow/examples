/*
 * Implementation of dynamically-allocated hashmap in C.
 * Use an array where each bucket in the array points to a linked list.
 * Each entry in a linked list is a contiguous dynamically allocated chunk
 * of memory consisting of:
 * 1. void * pointing to the next entry in the linked list
 * 2. array of chars representing the key
 * 3. the value of the current entry
 * Use pointer arithmetic as necessary to get/set.
 *
 * An alternative approach would be to use a struct for each entry. 
 * Since the value type is unknown, this would require a malloc for each value.
 * Although this approach would enhance the readability of the code, it could
 * dramatically slow performance for cases where value size is small.
 * 
 * Author:
 * Elizabeth Howe
 */

#include "cmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static const int default_capacity = 1023;

typedef struct CMap_internals {
    void **buckets;
    size_t n_buckets;
    size_t value_size;
    int count;
    free_fun clean;
} CMap;

/*
 * This hash function adapted from Eric Roberts' _The Art and Science of C_
 * Derive a hash code from an input string.
 * A hash code is an integer in the range [0-n_buckets-1].
 * It is computed using linear congruence.
 * A hash function is always stable.
 */
static int hash(const char *s, int n_buckets)
{
    int i;
    const unsigned long MULTIPLIER = 2630849305L;
    unsigned long hashcode = 0;

    for (i = 0; s[i] != '\0'; i++)
        hashcode = hashcode * MULTIPLIER + s[i];
    return hashcode % n_buckets;
}

static void set_next_in_entry(void *entry, void *next_entry)
{
    *(void **)entry = next_entry;
}

static char *get_key_from_entry(void *entry)
{
    return (char *)entry + sizeof(void *);
}

static void set_key_in_entry(void *entry, const char *key)
{
    strcpy(get_key_from_entry(entry), key);
}

static void *get_value_from_entry(void *entry)
{
    size_t key_size;
    void *value;

    key_size = strlen(get_key_from_entry(entry)) + 1;
    value = (char *)entry + sizeof(void *) + key_size;
    return value;
}

static void set_value_in_entry(CMap *cm, void *entry, const void *value)
{
    memcpy(get_value_from_entry(entry), value, cm->value_size);
}

/*
 * Each node in a linked list is a contiguous dynamically allocated chunk
 * of memory consisting of:
 * 1. void * pointing to the next entry in the linked list
 * 2. array of chars representing the key
 * 3. void * pointing to the value of the current entry
 */
static void *create_node(CMap *cm, const char *key, const void *value)
{
    size_t key_size;
    void *node;

    key_size = strlen(key) + 1;
    node = malloc(sizeof(void *) + key_size + cm->value_size); // include '\0'
    assert(node != NULL);
    set_next_in_entry(node, NULL);
    set_key_in_entry(node, key);
    set_value_in_entry(cm, node, value);
    return node;
}

CMap *cmap_create(size_t value_size, size_t capacity_hint, free_fun fn)
{
    CMap *cm;

    assert(value_size != 0);
    cm = malloc(sizeof(CMap));
    assert(cm != NULL);
    cm->value_size = value_size;
    cm->n_buckets = capacity_hint == 0 ? default_capacity : capacity_hint;
    cm->count = 0;
    cm->clean = fn;
    cm->buckets = calloc(capacity_hint, sizeof(void *));
    assert(cm->buckets != NULL);
    return cm;
}

void cmap_dispose(CMap *cm)
{
    int i;
    void *entry;

    for (i = 0; i < cm->n_buckets; i++) {
        while (cm->buckets[i] != NULL) {
            entry = cm->buckets[i];
            cm->buckets[i] = *(void **)(entry);
            if (cm->clean != NULL) {
                cm->clean(get_value_from_entry(entry));
            }
            free(entry);
        }
    }
    free(cm->buckets);
    free(cm);
}

int cmap_count(const CMap *cm)
{
    return cm->count;
}

void cmap_put(CMap *cm, const char *key, const void *addr)
{
    int bucket_num;
    void *node;
    void *front_node;

    bucket_num = hash(key, cm->n_buckets);

    // Check if key already exists in the linked list at bucket.
    // If found, deallocate memory at the current value and copy in new value.
    node = cm->buckets[bucket_num];
    while (node != NULL) {
        if (strcmp(get_key_from_entry(node), key) == 0) {  // found
            if (cm->clean != NULL) {
                cm->clean(get_value_from_entry(node));
            }
            set_value_in_entry(cm, node, addr);
            return;
        }
        node = *(void **)node;
    }

    // Key does not exist in the linked list at bucket.
    // Prepend the newly created node to the linked list at bucket.
    node = create_node(cm, key, addr);
    front_node = cm->buckets[bucket_num];
    if (front_node != NULL) {
        set_next_in_entry(node, front_node);
    }
    cm->buckets[bucket_num] = node;
    cm->count++;
}

void *cmap_get(const CMap *cm, const char *key)
{
    int bucket_num;
    void *node;

    bucket_num = hash(key, cm->n_buckets);

    node = cm->buckets[bucket_num];
    while (node != NULL) {
        if (strcmp(get_key_from_entry(node), key) == 0) {
            return get_value_from_entry(node);
        }
        node = *(void **)node;
    }
    return NULL;
}

const char *cmap_first(const CMap *cm) {
    int i;
    void *bucket;

    for (i = 0; i < cm->n_buckets; i++) {
        bucket = cm->buckets[i];
        if (bucket != NULL) {
            return get_key_from_entry(bucket);
        }
    }
    return NULL;
}

const char *cmap_next(const CMap *cm, const char *prev_key) {
    int i, prev_bucket;
    void *prev_entry;
    void *next_entry;

    // Check if there are more entries in the current linked list
    prev_entry = (char *)prev_key - sizeof(void *);
    next_entry = *(void **)prev_entry;
    if (next_entry != NULL) {
        return get_key_from_entry(next_entry);
    }
    // Return key from the first entry in a bucket after prev_bucket
    prev_bucket = hash(prev_key, cm->n_buckets);
    for (i = prev_bucket + 1; i < cm->n_buckets; i++) {
        if (cm->buckets[i] != NULL) {
            return get_key_from_entry(cm->buckets[i]);
        }
    }
    // No next entry
    return NULL;
}