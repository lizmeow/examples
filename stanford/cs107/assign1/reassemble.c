/*
 * File: reassemble.c
 * Author: Elizabeth Howe
 * ----------------------
 * Background:
 * Text fragments are created by duplicating a document many times over
 * and chopping each copy into pieces.
 *
 * Summary:
 * Read an input of text fragments and reassemble them.
 * Optimal reassembly is the shortest common superstring problem.
 * The shortest common superstring problem is NP-hard.
 * This program uses a greedy strategy that will find a common superstring,
 * but is not guaranteed to find the optimal reassembly.
 *
 * Algorithm:
 * Search for the pair of fragments that has the most overlap.
 * An overlap is when one prefix of one fragment matches the
 * suffix of the second fragment, or one fragment is entirely
 * within the other fragment.
 * Merge these two fragments into a single fragment.
 * Decrease the count of fragments by one.
 * Concatenation of fragments occurs when no overlap is found in any pair.
 * Continue until there is a single fragment.
 *
 * Args:
 * Path of the file to be reassembled.
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
    MAX_FRAG_LEN = 10000,
    MAX_FRAG_COUNT = 5000,
    START_FRAG_TOKEN = '{',
    END_FRAG_TOKEN = '}',
};

#ifdef DEBUG
# define print_arr debug_print_arr
# define clear_buf debug_clear_buf
#else
# define print_arr {}
# define clear_buf {}
#endif

#ifdef DEBUG
void debug_print_arr(char *arr[], int n_elems)
{
    int i;
    for (i = 0; i < n_elems; i++) {
        printf("%s\n", arr[i]);
    }
    printf("%d total elements\n", n_elems);
}

void debug_clear_buf(char *buf, int bsize)
{
    memset(buf, '\0', bsize);
}
#endif

void free_all_memory(char *a[], int n_elems)
{
    int i;
    for (i = 0; i < n_elems; i++) {
        free(a[i]);
        a[i] = NULL;
    }
}

int seek_frag_start(FILE *fp)
{
    int ch;

    while (1) {
        ch = fgetc(fp);
        if (ferror(fp)) {
            perror("fgetc");
            return -1;
        }
        if (feof(fp)) {
            return 0; // no more frags
        }
        if (ch == START_FRAG_TOKEN) {
            return START_FRAG_TOKEN;
        }
        if (!iswspace(ch)) {
            fprintf(stderr, "Detected non white space in between fragments.\n");
            return -1;
        }
    }
}

/* Assign the next fragment to the specified pointer */
int read_frag_body(FILE *fp, char **fragp)
{
    int i, ch;
    char frag[MAX_FRAG_LEN + 1];

    clear_buf(frag, sizeof(frag));

    for (i = 0; i <= MAX_FRAG_LEN; i++) {
        ch = fgetc(fp);
        if (ferror(fp)) {
            perror("fgetc");
            return -1;
        }
        if (feof(fp)) {
            fprintf(stderr, "Detected malformed fragment.\n");
            return -1;
        }
        if (ch == START_FRAG_TOKEN) {
            fprintf(stderr, "%c not allowed in fragments.\n", START_FRAG_TOKEN);
            return -1;
        }
        if (ch == END_FRAG_TOKEN) {
            if (i == 0) { // empty frag not supported
                fprintf(stderr, "Detected empty fragment.\n");
                return -1;
            }
            frag[i] = '\0';
            *fragp = strdup(frag);
            if (*fragp == NULL) {
                perror("strdup");
                return -1;
            }
            return 0;
        }
        frag[i] = ch;
    }
    // Frag too big
    fprintf(stderr, "Detected fragment length longer than %d.\n", MAX_FRAG_LEN);
    return -1;
}

/*
 * Read next single well-formed fragment as a dynamically allocated string.
 * Return 0 if fragment or end of file is found.
 * Return -1 if an error was detected.
 */
int read_frag(FILE *fp, char **fragp)
{
    int r;
    *fragp = NULL;

    r = seek_frag_start(fp);
    if (r <= 0) {
        return r;
    }
    r = read_frag_body(fp, fragp);
    return r;
}

/*
 * Read a sequence of fragments from the opened FILE * and
 * store them in an array as dynamically allocated strings.
 * Stop when there are no more fragments to read
 * or when the array is filled to capacity.
 * Return the number of strings read.
 */
int read_all_frags(FILE *fp, char *arr[])
{
    int i, r;
    char *frag;

    for (i = 0; i < MAX_FRAG_COUNT; i++) {
        r = read_frag(fp, &frag);
        if (r < 0) {
            free_all_memory(arr, i);
            return r;
        }
        if (frag == NULL) {
            break; // no more frags to read
        }
        arr[i] = frag;
    }
    print_arr(arr, i);
    return i;
}

/*
 * Check if any prefixes in s1 equal any suffixes in s2.
 * Return an int representing the length of the longest overlap found.
 */
int n_prefix_suffix_overlap(char s1[], char s2[])
{
    char *s2_suffix;
    char s1_prefix[strlen(s1) + 1];
    int n_s2, n_s2_suffix;
    int n_s1_prefix;

    // Clone s1 so we are free to manipulate it
    strcpy(s1_prefix, s1);

    s2_suffix = s2;
    n_s2 = strlen(s2);
    n_s1_prefix = strlen(s1_prefix);

    // Find occurrences of s1[0] in s2
    while (1) {
        s2_suffix = strchr(s2_suffix, s1[0]);
        if (s2_suffix == NULL) {
            break;
        }
        // Move up the '\0' of s1_prefix as much as possible
        n_s2_suffix = n_s2 - (int)(s2_suffix - s2);
        if (n_s2_suffix < n_s1_prefix) {
            n_s1_prefix = n_s2_suffix;
            s1_prefix[n_s2_suffix] = '\0'; // terminate string
        }
        // Check if there is a match
        if (strcmp(s2_suffix, s1_prefix) == 0) {
            return n_s2_suffix;
        }
        s2_suffix++;
    }
    // No occurrence
    return 0;
}

/*
 * The prefix of a[i] overlaps with the suffix of a[j] for n_overlap chars.
 * Merge a[i] and a[j] into an new string.
 * Free a[i].
 * Store the new string in a[i].
 */
void merge(char *a[], int i, int j, int n_overlap, int n_elems)
{
    char *result;
    int n1, n2;

    n1 = strlen(a[i]);
    n2 = strlen(a[j]);

    result = malloc(n1 + n2 - n_overlap + 1);
    if (result == NULL) {
        perror("malloc");
        free_all_memory(a, n_elems);
        exit(1);
    }
    strcpy(result, a[j]);
    strcat(result, a[i] + n_overlap);

    free(a[i]);
    a[i] = result;
}

/*
 * The logical length of the array is n_elems.
 * Examine all pairs of fragments in the array to find
 * the pair (i_save, j_save) with the maximal overlap.
 * Update a[i_save] to point to the merged string.
 * Free a[j_save].
 * Update a[j_save] to point to a[n_elems - 1].
 * Decrease the logical length of the array by 1.
 * Return the new logical length of the array.
 */
int reassemble_pass(char *a[], int n_elems)
{
    char *s;
    int i, j; 
    int i_save = -1, j_save = -1;
    int curr_overlap;
    int max_overlap = -1;
    int contained_flag = 0;

    // Search through all pairs of frags
    for (i = 0; i < n_elems; i++) {
        for (j = 0; j < n_elems; j++) {
            if (i == j) {
                continue;
            }
            // Check if a[j] is contained in a[i]
            s = strstr(a[i], a[j]);
            if (s != NULL) {
                curr_overlap = strlen(s);
                if (curr_overlap > max_overlap) {
                    max_overlap = curr_overlap;
                    i_save = i;
                    j_save = j;
                    contained_flag = 1;
                }
                continue; // no need to check for prefix/suffix overlap
            }
            // Check for the longest a[i] prefix that is also an a[j] suffix
            curr_overlap = n_prefix_suffix_overlap(a[i], a[j]);
            if (curr_overlap > max_overlap) {
                max_overlap = curr_overlap;
                i_save = i;
                j_save = j;
                contained_flag = 0;
            }
        }
    }
    assert(max_overlap >= 0);
    assert(i_save >= 0);
    assert(j_save >= 0);

    if (!contained_flag) {
        merge(a, i_save, j_save, max_overlap, n_elems);
    }
    // a[i_save] points to the fragment to keep
    // a[j_save] can be overwritten with the last fragment
    free(a[j_save]);
    a[j_save] = a[n_elems - 1];
    a[n_elems - 1] = NULL;
    n_elems--;
    return n_elems;
}

/* Continue to call reassemble_pass until there is a one fragment in the array */
void reassemble(char *a[], int n_frags)
{
    while (n_frags > 1) {
        n_frags = reassemble_pass(a, n_frags);
    }
}

/*
 * Use the command-line argument as a file path.
 * Open and read all fragments from that file.
 * On error, close the file and exit.
 * Print the assembled solution to the console.
 */
int main(int argc, char *argv[])
{
    char *filename;
    char *frags[MAX_FRAG_COUNT];
    FILE *fp;
    int n_frags;

    if (argc == 1) {
        fprintf(stderr, "You must specify a filename argument.\n");
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
    if (n_frags < 0) {
        exit(1);
    }
    else
    if (n_frags == 0) {
        fprintf(stderr, "File must contain at least 1 fragment.\n");
        exit(1);
    }
    reassemble(frags, n_frags);
    printf("%s\n", frags[0]);
    free(frags[0]);
    return 0;
}