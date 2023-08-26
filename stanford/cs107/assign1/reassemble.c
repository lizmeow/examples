/*
 * File: reassemble.c
 * Author: Elizabeth Howe
 * ----------------------
 * Summary:
 * Read an input of text fragments and reassemble them.
 * Text fragments are created by duplicating a document many times over 
 * and chopping each copy into pieces. 
 * 
 * Algorithm:
 * Search for the pair of fragments that has the most overlap.
 * That is, the prefix of one fragment matches the suffix of the other,
 * or one fragment is entirely within the other. 
 * Merge these two fragments into a single fragment. 
 * Decrease the count of fragments by one. 
 * Concatenation of fragments occurs when no overlap is found in any pair. 
 * Continue until there is a single fragment.
 *
 * Args:
 * Name of the file to be reassembled.
 *
 * Result: 
 * Print the final merged fragment to the console.
 *
 * Referance:
 * CS107 Stanford  
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wctype.h>
#include <assert.h>

enum {
    MAX_FRAG_LEN = 1000,
    MAX_FRAG_COUNT = 20000,
    START_FRAG_TOKEN = '{',
    END_FRAG_TOKEN = '}',
};

void print_arr(char *arr[], int n) { // debugging
    for (int i = 0; i < n; i++) {
        printf("%d %s\n", i, arr[i]);
    }
    printf("%d total fragments\n", n);
}

/* Function: read_frag
 * -------------------
 * Read a single well-formed fragment from the opened FILE *.
 * Return a dynamically-allocated string 
 * (or NULL on a read failure/malformed/EOF).
 * The returned string is created using strdup, which has a malloc call
 * hidden inside of it. The returned pointer will need to be freed
 * by the caller when done to avoid memory leaks.
 */

char *read_frag(FILE *fp)
{
    int i, ch;
    char frag[MAX_FRAG_LEN + 1];

    // find beginning of frag
    while (1) {
        ch = fgetc(fp);
        if (ferror(fp)) {
            perror("fgetc");
            fclose(fp);
            exit(1);
        }
        if (feof(fp)) {
            // no more frags left to read
            return NULL;
        }
        if (ch == START_FRAG_TOKEN) {
            break;
        }
        if (!iswspace(ch)) {
            fprintf(stderr, "Non white space char in between fragments.\b");
            fclose(fp);
            exit(1);
        }
    }

    memset(frag, '\0', sizeof(frag)); // debugging
    // read in frag
    for (i = 0; i <= MAX_FRAG_LEN; i++) {
        ch = fgetc(fp);
        if (ferror(fp)) {
            perror("fgetc");
            fclose(fp);
            exit(1);
        }
        if (feof(fp)) {
            fprintf(stderr, "Detected malformed fragment.\n");
            fclose(fp);
            exit(1);
        }
        if (ch == START_FRAG_TOKEN) {
            fprintf(stderr, "%c not allowed in fragments.\n", START_FRAG_TOKEN);
            fclose(fp);
            exit(1);
        }
        if (ch == END_FRAG_TOKEN) {
            if (i == 0) { // empty frag, not allowed
                fprintf(stderr, "Detected empty fragment.\n"); 
                fclose(fp);  
                exit(1);
            }
            frag[i] = '\0';
            return strdup(frag);
        }
        frag[i] = ch;
    }

    // frag too big
    fprintf(stderr, "Detected fragment lenngth longer than %d.\n", MAX_FRAG_LEN);
    fclose(fp);
    exit(1);    
}

/* Function: read_all_frags
 * ------------------------
 * Read a sequence of correctly-formed fragments from the opened FILE * and
 * store them into the array. Stop when there are no more fragments to read
 * or when the array is filled to capacity (whichever comes first). The fragment
 * strings are dynamically-allocated and will need to be freed by the caller when
 * done to avoid memory leaks. The return value is the count of strings read. 
 */

int read_all_frags(FILE *fp, char *arr[]) 
{
    int i;
    char *frag;    

    for (i = 0; i < MAX_FRAG_COUNT; i++) { 
        frag = read_frag(fp);
        if (frag == NULL) {
            // No more frags to read
            break;
        }
        arr[i] = frag;
    }

    print_arr(arr, i);

    return i;
}

/*
 * Function: find_max_overlap
 * ----------------------------
 * See if any prefixes in s1 equal any suffixes in s2.
 * Take a pair of strings as argument. 
 * Check for overlaps with longer lengths first.
 * Return an int (overlap found).
 */

int find_max_overlap(char *s1, char *s2)
{
    char *current;
    char s1_clone[strlen(s1) + 1];
    int n_current;
    int n_s1_clone;
    
    // Clone s1 so we are free to manipulate it 
    strcpy(s1_clone, s1);

    current = s2;

    // Find occurrences of s1[0] in s2
    while (1) {
        current = strchr(current, s1[0]);
        if (current == NULL) {
            break;
        }
        // Move up the period of s1_clone if possible
        n_current = strlen(current);
        n_s1_clone = strlen(s1_clone);
        if (n_current < n_s1_clone) {
            s1_clone[n_current] = '\0';
        }

        // Check if there is a match
        if (strcmp(current, s1_clone) == 0) {
            return n_current;
        }
        current++;
    }

    // No occurrence
    return 0;
}

/* Function: nullify_entry
 * ------------------------
 * Take an array of pointers to strings and an index of the array. 
 * Free the corresponding entry of the array.
 */

void nullify_entry(char *a[], int n_elems, int ndx)
{
    assert(ndx < n_elems);
    free(a[ndx]);
    a[ndx] = NULL;
}

/* Function: merge
 * ----------------
 * Merge two fragments in the given array.
 * Arguments: 
 * an array of pointers to strings, 
 * the 2 indices of the fragments to merge, 
 * and overlap (int) of these two fragments.
 */
void merge(char *a[], int ndx1, int ndx2, int overlap)
{
    char *s1, *s2, *result;
    int n1, n2;

    if (ndx1 < 0 || ndx2 < 0) {
        return;        
    }

    s1 = a[ndx1];
    s2 = a[ndx2];

    n1 = strlen(s1);
    n2 = strlen(s2);

    result = malloc(n1 + n2 - overlap + 1);
    strcpy(result, s2);  // take all of s2
    strcat(result, s1 + overlap);  // prefix of s1 overlaps suffix of s2

    free(s1);
    a[ndx1] = result;
}


/* Function: count_logical_elems
* ------------------------------
* Precondition: all non-empty elements will be at the beginning of the 
* array.
*/
int count_logical_elems(char *a[], int n_elems)
{
    int i;
    for (i = 0; i < n_elems; i++) {
        if (a[i] == NULL) {
            break;
        }
    }
    return i;
}


/* Function: squeeze
 * ------------------
 * Take an array of pointers to strings and the size (int) of the array.
 * Iterate through the elements of the array, and for each spot 
 * that doesn't have an element, take the bottom-most element and put 
 * it in the spot. Array will have all non-NULL elements at the top.
 */
void squeeze(char *a[], int n_elems)
{
    int i, j;

    for (i = 0; i < n_elems; i++) {
        if (a[i] == NULL) {
            for (j = n_elems - 1; j > i; j--) {
                if (a[j] != NULL) {
                    a[i] = a[j];
                    a[j] = NULL;
                    break;
                }
            }
        }
    }
}

/*
 * Function: reassemble_pass
 * -------------------------
 * Examine all pairs of fragments in the array to find
 * the pair with the maximal overlap, assuming 
 * 
 * Update the array with the merge.
 * Take an array of char *s and the number of frags in the array. 
 * Return the new number of elements in the array.
 */

int reassemble_pass(char *a[], int n_elems)
{ 
    char *s, *s1, *s2;
    int i, j, n1, n2, overlap1, overlap2, n_elems_updated;
    int max_overlap = -1;
    int first_in_merge = -1;
    int second_in_merge = -1;

    // Get all pairs of elems (no repeated pairs)
    for (i = 0; i < n_elems; i++) {
        if (a[i] == NULL) {
            continue;
        }
        for (j = i + 1; j < n_elems; j++) {
            if (a[j] == NULL || a[i] == NULL) {
                continue;
            }
            s1 = a[i];
            s2 = a[j];
            n1 = strlen(s1);
            n2 = strlen(s2);
                
            // Check if one string contains the other
            // Get rid of redundant fragments
            if (n1 >= n2) {
                s = strstr(s1, s2);
                if (s != NULL) {  // s2 contained in s1
                    nullify_entry(a, n_elems, j);
                    continue;
                }
            }
            else {
                s = strstr(s2, s1);
                if (s != NULL) { // s1 contained in s2
                    nullify_entry(a, n_elems, i);
                    break; // no need to find any other pair with s1
                }
            }
            
            if (n1 <= max_overlap && n2 <= max_overlap) {
                continue;    // can't possibly beat max_overlap
            }
            overlap1 = find_max_overlap(s1, s2);
            if (overlap1 > max_overlap) {
                max_overlap = overlap1;
                first_in_merge = i;
                second_in_merge = j;
            }

            overlap2 = find_max_overlap(s2, s1);
            if (overlap2 > max_overlap) {
                max_overlap = overlap2;
                first_in_merge = j;
                second_in_merge = i;
             }
        }
    }    
    merge(a, first_in_merge, second_in_merge, max_overlap);
    nullify_entry(a, n_elems, second_in_merge);
    squeeze(a, n_elems);
    n_elems_updated = count_logical_elems(a, n_elems);

    return n_elems_updated;
}
/*
 * function: reassemble
 * ---------------------
 * Continue to call reassemble_pass until there is a one fragment in the array.
 * Args:
 * an array of char *s, 
 * an int representing the number of elements in the array.
 */

void reassemble(char *a[], int n_elems)
{
    while (n_elems > 1) {
        n_elems = reassemble_pass(a, n_elems);
    }
}

/* Function: main
 * --------------
 * Use the command-line argument as a filename. 
 * Open and read all fragments from that file.
 */

int main(int argc, char *argv[]) 
{
    char *filename;
    char *frags[MAX_FRAG_COUNT]; 
    FILE *fp;
    int n_frags;

    if (argc == 1) { 
        fprintf(stderr, "You must specify a filename argument\n");
        exit(1);
    }    
    if (argc > 2) {
       fprintf(stderr, "Ignoring excess arguments...\n"); 
    }

    filename = argv[1]; // always take the first argument
    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open file \"%s\"\n", filename);
        exit(1);       
    }        
    n_frags = read_all_frags(fp, frags);
    fclose(fp);

    if (n_frags == 0) {
        fprintf(stderr, "File must contain at least 1 fragment\n");
        exit(1);
    }

    reassemble(frags, n_frags);
    printf("%s\n", frags[0]);
    free(frags[0]);
    return 0;
}
