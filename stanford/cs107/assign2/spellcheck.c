/*
 * File: spellcheck.c
 * Author: Elizabeth Howe
 * ----------------------
 * Program calculates the edit distance between 2 words. Program expects 2 arguments - a corpus file
 * and a document to check, or a word to check. The edit distance is the number of inertions, 
 * substitutions, or deletions that must happen to change one word to the next word. The program
 * records the top 5 words with the closest edit distance per input word. To break ties, we 
 * also use the frequencies of the words in the corpus, and finally break persisting ties 
 * alphabetically. Prints out each misspelled word (word not found in the corpus), and their 
 * suggested corrections. 
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include "cvector.h" 
#include "cmap.h"  

#ifndef MAX_RESULTS
#define MAX_RESULTS 5
#endif

#define CMAP_CAPACITY_HINT 10000
#define LEADER_BOARD_CAPACITY_HINT 5
#define WORDS_CAPACITY_HINT 50
#define MAX_STRING_LENGTH 30
#define min(x1, x2) \
(x1 < x2 ? x1 : x2)
#define min3(x1, x2, x3) \
(x1 < x2 ? min(x1, x3) : min(x2, x3))

/* Function: read_word
 * -------------------
 * Reads the next word from the opened FILE *.  A word is defined as
 * a contiguous sequence of lower/uppercase letters ended by whitespace.
 * If such a word is successfully read, it is stored in buf and the 
 * function returns true. If there are no more words to be read, the
 * function returns false and the contents of buf are unreliable. The 
 * client is responsible for ensuring buf is large enough to hold the 
 * maximum length word (including 1 more for null char).
 * The function skips over non-words, e.g. sequences of letters mixed
 * with non-letters (punctuation, digits) and truncates sequences of 
 * letters longer than the maximum length. 
 */
bool read_word(FILE *fp, char buf[])
{
	while (true) { // keep reading until good word or EOF
		if (fscanf(fp, " %30[a-zA-Z]", buf) == 1) { // read at most 30 letters
			int ch = getc(fp);                      // peek at next char
			if (isspace(ch) || ch == EOF) return true; // accept if end space/EOF
			ungetc(ch, fp);                         // otherwise, put char back
		}
		if (fscanf(fp,"%*s") == EOF) return false; //%*s reads rest of token and discards
    }
}

/*
 * Builds a map based on the frequencies of each word in the corpus. The keys are the words 
 * in the corups, and the values are the corresponding frequencies. Takes a file handle as 
 * argument, and returns a pointer to the resulting map.
 */
CMap *buildMap(FILE *fp)
{
	int freq;
	char buf[MAX_STRING_LENGTH + 1];
		
	CMap *m = cmap_create(sizeof(int), CMAP_CAPACITY_HINT, NULL); // XXX hint
	if (m == NULL) {
		printf("can't create map\n");
		exit(1);
	}
	while (read_word(fp, buf)) {
		char *s = strdup(buf);
		for (int i = 0; s[i] != '\0'; i++) {
			s[i] = tolower(s[i]);
		}
		int *p = (int *)cmap_get(m, s);
		if (p == NULL) { // if word is not in map already
			freq = 1;
			cmap_put(m, s, &freq);
		}
		else { // otherwise update frequency
			freq = *p;
			freq++;
			cmap_put(m, s, &freq);
		}
		free(s);
	}
	return m;
}

/*
 * Checks to see if the word is found in the corpus (map). Returns a bool, 
 * and takes as arguments a pointer ot a map and a pointer to a word to check.
 */
bool isFound(CMap *m, const char *word) 
{
	int *p = (int *)cmap_get(m, word);
	if (p == NULL) {
		return false;
	}
	return true;
}

/*
 * Calculates the edit distance between two given words. Takes as input two char *s and 
 * a cutoff int of the number to beat. Function stops calculating once it knows it cannot 
 * beat the int beatThis. Returns an int representing the edit distance, or else a number 
 * greater than the edit distance if it knows it can't beat it. 
 */
int edist(const char *s1, const char *s2, int beatThis)
{
	if ((beatThis == 0) && (strcmp(s1, s2) != 0)) { // can't beat this
		return MAX_STRING_LENGTH + 1;
	}
	if (*s1 == '\0') {
		return strlen(s2);
	}
	if (*s2 == '\0') {
		return strlen(s1);
	}

	int insertion1, insertion2, subOrMatch;

	insertion1 = edist(s1 + 1, s2, beatThis - 1) + 1; // insertion in s1
	insertion2 = edist(s1, s2 + 1, beatThis - 1) + 1; // insertion in s2

	if (*s1 == *s2) {
		subOrMatch = edist(s1 + 1, s2 + 1, beatThis); // match
	}
	else {
		subOrMatch = edist(s1 + 1, s2 + 1, beatThis - 1) + 1; // substitution
	}
	
	int d =  min3(insertion1, insertion2, subOrMatch);
	return d;
}

/* 
 * Defining the struct that populates the leader board. Correction keeps track of the edit distance,
 * the frequency of the word in the corpus, and the suggested correction.
 */
typedef struct correction {
	int dist;
	int freq;
	char *s;
} Correction;

/*
 * To compare two corrections and see which is better. Takes two void *s and 
 * returns an int. It returns a positive number if the better correction is 
 * the second argument, zero if they are equally better, and negative if the 
 * first argument is better.
 */
int cmp_correction(const void *p1, const void *p2) 
{
	const Correction *c1, *c2;
	c1 = p1;
	c2 = p2;

	if (c1->dist != c2->dist) {
		return c1->dist - c2->dist;
	}
	if (c1->freq != c2->freq) {
		return c2->freq - c1->freq;
	}
	return strcmp(c1->s, c2->s);
}

/*
 * The compare function pointer to use in the vector search function. Compares 
 * two words using hte strcmp function. If a given word is found in the vector 
 * already, will return a 0.
 */
int cmp_words(const void *p1, const void *p2)
{
	const char *s1, *s2;
	s1 = *(char **)p1;
	s2 = *(char **)p2;

	return strcmp(s1, s2);
}

/*
 * Updates the leader board according to whether the leader board is partially full
 * or whether it is full. If it is full, we relapce the last struct and sort. If it 
 * is partially full, we append and sort. Takes a pointer to a map, a pointer to 
 * vector (leader board), a pointer to the word in question, and a bool saying if we 
 * should append or not. Returns an int representing the next edit distance to beat.
 */
int updateLeaderBoard(CMap *m, CVector *v, const char *s, int d, bool doAppend)
{
	int beatThis = INT_MAX;
	Correction c, *p;

	c.dist = d;
	c.s = strdup(s);
	c.freq = *(int *) cmap_get(m, s);

	if (doAppend) { // leader board partial
		cvec_append(v, &c);
		cvec_sort(v, cmp_correction);
	}
	else { // leader board is full
		p = cvec_nth(v, MAX_RESULTS - 1);
		if (cmp_correction(&c, p)  < 0)	 { // if better, replace last struct and sort
			cvec_replace(v, &c, MAX_RESULTS - 1);
			cvec_sort(v, cmp_correction);
		}
		else { // not appending or replacing
			free(c.s); // undo strdup
		}
		p = (Correction *)cvec_nth(v, MAX_RESULTS - 1);
		beatThis = p->dist; 
	}
	return beatThis;
}

/*
 * Find the best alternate spellings for a known misspelled word. Takes as argument
 * a pointer ot a map built from the corpus, a pointer to the word in question, a pointer 
 * to the vector storing the leader board, and a bool representing if we should print success.
 */ 
void spellCheck(CMap *m, const char *word, CVector *v, bool printOk)
{
	int beatThis = INT_MAX;

	if (isFound(m, word)) {
		if (printOk) {
			printf("\'%s\' spelled correctly.\n", word);
		}
		return;
	}	
	for (const char *s = cmap_first(m); s !=NULL; s = cmap_next(m, s)) {
		int d = edist(s, word, beatThis);
		if (d > MAX_STRING_LENGTH)
			continue;
		int count = cvec_count(v);
		if (count < MAX_RESULTS) {
			beatThis = updateLeaderBoard(m, v, s, d, true);
		}
		else { // we already have max elements in our vector
			beatThis = updateLeaderBoard(m, v, s, d, false);
		}
	}
	printf("%s:", word);
	for (Correction *cv = (Correction *)cvec_first(v); cv != NULL; cv = (Correction *)cvec_next(v, cv)) {
		printf(" %s",cv->s);
	}
	printf("\n");
}

/*
 * Cleanup function for the leader board vector. Takes a void *.
 */
void cleanup_cvec(void *p)
{
	Correction *c;
	c = p;

	free(c->s); // undo strdup
}

/*
 * Cleanup function for the misspelled words vector. Takes a void *.
 */
void cleanup_cvec2(void *p)
{
	char *s;
	s = *(char **)p;

	free(s); // undo strdup
}

/*
 * Finds all the misspellings in the document. Makes sure our missellings are unique.
 * Takes a pointer to the corpus map, a FILE * for the second document, and a pointer to 
 * the vector holding the misspellings.
 */
void collectMisspellings(CMap *m, FILE *fp2, CVector *words)
{
	char buf[MAX_STRING_LENGTH + 1];
	while(read_word(fp2, buf)) {
		char *s = strdup(buf);
		for (int i = 0; s[i] != '\0'; i++) {
			s[i] = tolower(s[i]);
		}			
		int match = cvec_search(words, &s, cmp_words, 0, true);
		if (match == -1) { // not already in the vector
			if (!isFound(m, s)) { // if not in corpus, we have misspellling
				cvec_append(words, &s); // add to our misspelling vector, words
				cvec_sort(words, cmp_words); // XXX does this speed up cvec_search
			}
		}		
	}
}

int main(int argc, char *argv[]) 
{     
	if (argc != 3) {
		fprintf(stderr, "%s: you must specify the corpus and what-to-check. The what-to-check argument can be a single word or document.\n", argv[0]);
		exit(1);
	} 
	FILE *fp = fopen(argv[1], "r");
	if (fp == NULL) {
		fprintf(stderr, "%s: can't open\n", argv[1]);
		exit(1);
	}
	CMap *m = buildMap(fp);
	fclose(fp);	

	CVector *words = cvec_create(sizeof(char *), WORDS_CAPACITY_HINT, cleanup_cvec2);
	if (words == NULL) {
		fprintf(stderr, "can't create vector of misspellings\n");
		exit(1);
	}
		
	bool printOk; // flag to print correctly spelled word

	FILE *fp2 = fopen(argv[2], "r");
	if (fp2 != NULL) {
		printOk = false;
		collectMisspellings(m, fp2, words);
	}
	else {
		printOk = true;
		if (strlen(argv[2]) > MAX_STRING_LENGTH) {
			fprintf(stderr, "word longer than limit of %d \n", MAX_STRING_LENGTH);
			exit(1);
		}
			
		char *s = strdup(argv[2]);
		for (int i = 0; s[i] != '\0'; i++) {
			s[i] = tolower(s[i]);
		}			
		cvec_append(words, &s);
	}	

	char *s;
	for(char **sp = (char **)cvec_first(words); sp != NULL; sp = (char **)cvec_next(words, sp)) {
		s = *sp;
		CVector *v = cvec_create(sizeof(Correction), LEADER_BOARD_CAPACITY_HINT, cleanup_cvec);
		spellCheck(m, s, v, printOk); 
		cvec_dispose(v);
	}
	cvec_dispose(words);
	cmap_dispose(m);
	return 0;
}

