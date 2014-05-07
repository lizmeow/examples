/*
 * File: reassemble.c
 * Author: Elizabeth Howe
 * ----------------------
 * This program takes a file of text fragments, and reassembles the file into a single fragment. It does 
 * this by looking for the pair of fragments that has the most overlap, merges these two fragments into 
 * a single fragment, and finally getting rid of the original fragment pair. Concatenation of fragments
 * occurs when no overlap is found in any pair. The program takes an 
 * argument that is the name of the file to be reassembled and prints the final merged fragment to
 * console.  
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wctype.h>

// These are the limits on maximum fragment length and maximum number of frags in a file
#define MAX_FRAG_LEN 1000
#define MAX_FRAG_COUNT 20000

void AtoStr(char *a[], int elems) {
	for (int i = 0; i < elems; i ++) {
		printf("DUMPING array[%d]=%s\n",i,a[i]);
	} 	
}


/* Function: read_frag
 * -------------------
 * Reads a single well-formed fragment from the opened FILE * and returns a
 * dynamically-allocated string (or NULL on a read failure/malformed/EOF).
 * Takes as argument a FILE * and a malformedFlag pointer to an int, which 
 * will become 1 if the frag is malformed, 2 if we have a fragment that is 
 * too big, and remaining 0 otherwise.
 * The returned string is created using strdup, which has a malloc call
 * hidden inside of it, thus the returned pointer will need to be freed
 * by the caller when done to avoid memory leaks.
 */

char *read_frag(FILE *fp, int *malformedFlag)
{
	int i, ch;
	char frag[MAX_FRAG_LEN + 1];

	*malformedFlag = 0;

	// find beginning of frag
	while(1) {
		ch = fgetc(fp);
		if(ferror(fp) || feof(fp))
			return NULL;
		if(iswspace(ch))
			continue;
		if(ch == '#')
			break;
	}

	// read in frag
	for(i = 0; i < MAX_FRAG_LEN; i++) {
		ch = fgetc(fp);
		if(ferror(fp) || feof(fp)) {
			*malformedFlag = 1; // couldn't complete
			return NULL;
		}
		if(ch == '#') { // end of frag found
			if(i == 0) { // empty frag
				*malformedFlag = 1;	
				return NULL;
			}
			frag[i] = '\0';
			return strdup(frag);
		}
		frag[i] = ch;
	}

	// frag too big
	*malformedFlag = 2;	
	return NULL;
}

/* Function: read_all_frags
 * ------------------------
 * Reads a sequence of correctly-formed fragments from the opened FILE * and
 * stores them into the array.  Stops when there are no more fragments to read
 * or when the array is filled to capacity (whichever comes first). The fragment
 * strings are dynamically-allocated and will need to be freed by the caller when
 * done to avoid memory leaks. The return value is the count of strings read. 
 */

int read_all_frags(FILE *fp, char *arr[], int maxfrags) 
{
	int malformedFlag = 0;	

	for (int i = 0; i < maxfrags; i++) { 
		char *frag = read_frag(fp, &malformedFlag);
		if (frag == NULL) {
			if (malformedFlag == 0) { // we are at the end of the file
				return i;
			}
			else if (malformedFlag == 1)  {
				fprintf(stderr, "./reassemble: read failed. (read %d well-formed fragments from file, subsequent fragment is malformed).", i);
			exit(3);
			}
			else {
				fprintf(stderr, "./reassemble: read failed. (read %d well-formed fragments from file, subsequent fragment is longer than 1000 characters).", i);
				exit(4);
			}			
		}
		arr[i] = frag;
	}
	return maxfrags;
}

/*
 * Function: FindMaxOverlap
 * ----------------------------
 * FindMaxOverlap sees if any prefixes in s1 equal any suffixes in s2.
 * Takes a pair of strings as argument. 
 * FindMaxOverlap checks for overlaps with longer lengths first.
 * Returns an int (overlap found).
 */

int findMaxOverlap(char *s1, char *s2)
{
	// Clone s1 so prefixes can be found in s1
	int s1len = strlen(s1);
	char s1Clone[s1len + 1];
	strcpy(s1Clone, s1);

	// Moving current rightward in s2
	char *current = s2;

	// Find occurrences of s1[0] in s2
	while (1) {
		current = strchr(current, s1[0]);
		if (current == NULL) {
			return 0; // no occurrence
		}
		int n = strlen(current);
		s1Clone[n] = '\0'; // moving period to the left 
		if (strcmp(current, s1Clone) == 0) { // if match
			return n;
		}
		current++;
	}
}

/* Function: collapseEntry
 * ------------------------
 * Takes an array of pointers to strings, and a given index (int) of the array. 
 * Frees the entry of the array.
 */

void collapseEntry(char *a[], int elems, int ndx)
{
	free(a[ndx]);
	a[ndx] = NULL;
}

/* Function: merge
 * ----------------
 * Merges two fragments in the given array.
 * Takes as argument an array of pointers to strings, the 2 indices of the 
 * fragments we want to merge, and overlap (int) of these two fragments.
 * Assumed that the prefix of the first string 
 * overlaps with the suffix of the second string (overlap can be 0).
 */

void merge(char *a[], int ndx1, int ndx2, int overlap)
{
	if (ndx1 < 0 || ndx2 < 0) {
		return;		
	}

	char *s1 = a[ndx1];
	char *s2 = a[ndx2];

	int n1 = strlen(s1);
	int n2 = strlen(s2);

	char *result = malloc(n1 + n2 - overlap + 1);
	strcpy(result, s2);  // take all of s2
	strcat(result, s1 + overlap);  // prefix of s1 overlaps suffix of s2

	free(s1);
	a[ndx1] = result;
}

/* Function: squeeze
 * ------------------
 * Takes an array of pointers to strings and the size (int) of the array.
 * Iterates through the elements of the array, and for each spot 
 * that doesn't have an element, takes the bottom-most element and puts 
 * it in the spot. Array will have all non-NULL elements at the top.
 * Returns an int representing the number of actual elements in the array.
 */
int squeeze(char *a[], int elems)
{
	for (int i = 0; i < elems; i++) {
		if (a[i] == NULL) {
			for (int j = elems - 1; j > i; j--) {
				if (a[j] != NULL) {
					a[i] = a[j];
					a[j] = NULL;
					break;
				}
			}
		}
	}
	int elemsNew = 0;
	for (int i = 0; i < elems; i++) {
		if(a[i] != NULL) {
			elemsNew++;
		}
	}
	return elemsNew;
}

/*
 * Function: reassemblePass
 * -------------------------
 * Examines all pairs of fragments in the array to find the fragment with the maximal overlap.
 * Then updates the array with the merge.
 * Takes an array of pointers to chars and an int representing the number of frags in the array. 
 * Returns the new number of elements in the array (int).
 */

int reassemblePass(char *a[], int elems)
{ 
	char *s = NULL;
	int maxOverlap = -1;
	int firstInMerge = -1;
	int secondInMerge = -1;

	// Get all pairs of elems (no repeated pairs)
	for (int i = 0; i < elems; i++) {
		if (a[i] == NULL) {
			continue;
		}
		for (int j = i + 1; j < elems; j++) {
			if (a[j] == NULL || a[i] == NULL) {
				continue;
			}
			char *s1 = a[i];
			char *s2 = a[j];
			int n1 = strlen(s1);
			int n2 = strlen(s2);
			int ndx1 = i;
			int ndx2 = j;
			int overlap1;
			int overlap2;
				
			// check if one string contains the other
			if (n1 >= n2) {
				s = strstr(s1, s2); // s2 contained in s1
				if (s != NULL) {
					collapseEntry(a, elems, ndx2);
					continue;
				}
			}
			else {
				s = strstr(s2, s1); // s1 contained in s2
				if (s != NULL) {
					collapseEntry(a, elems, ndx1);
					continue;
				}
			}
			
			if (n1 <= maxOverlap && n2 <= maxOverlap) {
				continue;	// can't possibly beat maxOverlap
			}
			overlap1 = findMaxOverlap(s1, s2);
			if (overlap1 > maxOverlap) {
				maxOverlap = overlap1;
				firstInMerge = ndx1;
				secondInMerge = ndx2;
			}

			overlap2 = findMaxOverlap(s2, s1);
			if (overlap2 > maxOverlap) {
				maxOverlap = overlap2;
				firstInMerge = ndx2;
				secondInMerge = ndx1;
			}
		}
	}	
	merge(a, firstInMerge, secondInMerge, maxOverlap);
	collapseEntry(a, elems, secondInMerge);
	elems = squeeze(a, elems);

	return elems;
}
/*
 * function: reassemble
 * ---------------------
 * Continues to call reassemblePass until we have just one fragment left in the array.
 * Takes as argument an array of char * , and an int representing the number of elements in 
 * the array.
 */

void reassemble(char *a[], int elems)
{
	while (elems > 1) {
		elems = reassemblePass(a, elems);
	}
}

/* Function: main
 * --------------
 * This main program uses the command-line argument as a filename and opens and reads 
 * all fragments from that file.
 */

int main(int argc, char *argv[]) 
{
	if (argc == 1) { // if no file was specified 
		fprintf(stderr, "./reassemble: you must specify a filename argument\n");
		exit(1);
	}	
	if (argc > 2) { // if excess arguments
	   fprintf(stderr, "./reassemble: ignoring excess arguments...\n"); 
	}

	char *filename = argv[1]; // always take the first argument
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		fprintf(stderr, "./reassemble: cannot open file \"%s\"\n", filename);
		exit(2);   	
	}		
	char *frags[MAX_FRAG_COUNT];  
	int nfrags = read_all_frags(fp, frags, MAX_FRAG_COUNT);
	fclose(fp);
	
	reassemble(frags, nfrags);
	printf("%s\n", frags[0]);
	free(frags[0]);
	return 0;
}

