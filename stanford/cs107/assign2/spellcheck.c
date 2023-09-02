/*
 * File: spellcheck.c
 * Author: Elizabeth Howe
 * ----------------------
 * Background:
 * The edit distance is the number of insertions, substitions, or deletions
 * that must happen to change one word to the next word.

 * Summary:
 * Given a corpus file and a document of input words (or a single input word 
 * on a command line), print out the top 3 words in the corpus file with the 
 * closest edit distance per input word. 
 * Break ties according to the frequencies of the words in the corpus. 
 * Break persisting ties alphabetically.
 * Skip any input words that are found in the corpus file.
 *
 * Args:
 * 1. A corpus file 
 * 2. Document file of input words or a single input word
 *
 * Result:
 * For each input word not found in the corpus, print to stdout the top 3 
 * "closest" words.
 *
 * Reference:
 * Stanford CS107 
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include "cvector.h" 
#include "cmap.h"  

enum {
    MAX_RESULTS = 3,
    CMAP_CAPACITY_HINT = 10000,
    LEADER_BOARD_CAPACITY_HINT = 5,
    WORDS_CAPACITY_HINT = 50,
    MAX_STRING_LENGTH = 30,
};

#define min(x1, x2) (x1 < x2 ? x1 : x2)
#define min3(x1, x2, x3) (x1 < x2 ? min(x1, x3) : min(x2, x3))

/* 
 * Define the struct that populates the leader board of suggested corrected 
 * spellings. 
 * Keep track of the edit distance,
 * the frequency of the word in the corpus, 
 * and the suggested corrected word spelling.
 */
typedef struct {
    int dist;
    int freq;
    char *s;
} Correction;

void s_tolower(char *s) {
    for (int i = 0; s[i] != '\0'; i++) {
        s[i] = tolower(s[i]);
    }
}

/* 
 * Read the next word from the opened FILE *.  
 * A word is defined as a contiguous sequence of lower/uppercase letters 
 * ended by whitespace.
 * Skip over non-words, e.g. sequences of letters mixed with non-letters 
 * (punctuation, digits) and truncates sequences of letters longer than 
 * the maximum length.
 * Return true if the next word is successfully read and store this word in 
 * buf.
 * Return false if there are no more words left to read or if there is a file 
 * error, in which case the contents of buf are unreliable. 
 */
bool read_word(FILE *fp, char buf[])
{
    int ch;
    char format_buf[16];
    sprintf(format_buf, " %%%d[a-zA-Z]", MAX_STRING_LENGTH);

    while (true) {
        // Read at most 30 letters, consume but not store leading white space
        if (fscanf(fp, format_buf, buf) == 1) {
            ch = getc(fp); // peek at next char
            if (isspace(ch) || ch == EOF) {
                return true; // accept if space/EOF
            }
            ungetc(ch, fp); // otherwise, put char back
        }
        if (fscanf(fp,"%*s") == EOF) {
            return false; //%*s reads rest of token and discards
        }
    }
}

/*
 * Return a pointer to a map. 
 * Return NULL on read error.
 * The keys in the map are the words corpus file. 
 * The values are the corresponding frequencies in the corpus file.
 */
CMap *build_map(FILE *fp)
{
    int freq;
    int *p;
    char buf[MAX_STRING_LENGTH + 1];
        
    CMap *m = cmap_create(sizeof(int), CMAP_CAPACITY_HINT, NULL);
    while (read_word(fp, buf)) {
        s_tolower(buf);
        p = (int *)cmap_get(m, buf);
        if (p == NULL) { // if word is not in map already
            freq = 1;
        }
        else { // otherwise update frequency
            freq = *p + 1;
        }
        cmap_put(m, buf, &freq);
    }
    if (ferror(fp)) {
        perror("read corpus");
        cmap_dispose(m);
        return NULL;
    }
    return m;
}

/* Return true if a word is a key in the map, else return false. */
bool is_found(CMap *m, const char *word) 
{
    int *p = (int *)cmap_get(m, word);
    if (p == NULL) {
        return false;
    }
    return true;
}

/*
 * Return edit distance between two words as long as this edit distance is 
 * less than max_edit_dist_allowed.
 * Return an int greater than MAX_STRING_LENGTH if the edit distance is 
 * at least the max_edit_dist_allowed.
 * Here, a substitution is penalized by 1 (in some definitions, a substitution
 * is penalized by 2).
 * XXX TO DO: cache intermediate calculations in a table and avoid recursion. 
 */
int edit_dist(const char *s1, const char *s2, int max_edit_dist_allowed)
{
    int insertion_s1, insertion_s2, sub, match;
    
    if (max_edit_dist_allowed == 0 && strcmp(s1, s2) != 0) {
        // Edit distance between s1 and s2 is at least max_edit_dist_allowed,
        // stop calculating early.
        return MAX_STRING_LENGTH + 1;
    }
    if (*s1 == '\0') {
        return strlen(s2);
    }
    if (*s2 == '\0') {
        return strlen(s1);
    }
    insertion_s1 = edit_dist(s1 + 1, s2, max_edit_dist_allowed - 1) + 1;
    insertion_s2 = edit_dist(s1, s2 + 1, max_edit_dist_allowed - 1) + 1;

    if (*s1 == *s2) {
        match = edit_dist(s1 + 1, s2 + 1, max_edit_dist_allowed);
        return min3(insertion_s1, insertion_s2, match);
    }
    else {
        sub = edit_dist(s1 + 1, s2 + 1, max_edit_dist_allowed - 1) + 1;
        return min3(insertion_s1, insertion_s2, sub);
    }
}

/*
 * Return a positive number if p2 is the better correction.
 * Return zero if the corrections are equal.
 * Return a negative number if p1 is the better correction.
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

int cmp_words(const void *p1, const void *p2)
{
    const char *s1, *s2;

    s1 = *(char **)p1;
    s2 = *(char **)p2;

    return strcmp(s1, s2);
}

/*
 * If the leader board is full, compare the last correction in the leader board
 * with a new correction containing word. If the new correction is better, 
 * replace the last correction with the new correction and sort. 
 * Otherwise if the leader board is not full, append a new correction containing
 * s and sort.
 * Return the maximum edit distance contained in the leader board.
 * Precondition: corpus_map is known to contain word.
 */
int update_leader_board(CMap *corpus_map, CVector *leader_board, 
                        const char *word, int d)
{
    int max_edit_dist_allowed, leader_board_count;
    Correction c, *p;

    leader_board_count = cvec_count(leader_board);
    memset(&c, 0, sizeof(c));
    c.dist = d;
    c.s = strdup(word);
    c.freq = *(int *)cmap_get(corpus_map, word);
    
    if (leader_board_count == MAX_RESULTS) { // leader_board is full
        p = cvec_nth(leader_board, MAX_RESULTS - 1);
        if (cmp_correction(&c, p)  < 0)	 { // if better, replace last struct
            cvec_replace(leader_board, &c, MAX_RESULTS - 1);
            cvec_sort(leader_board, cmp_correction);
        }
        else { // not appending or replacing
            free(c.s); // undo strdup
        }
        p = cvec_nth(leader_board, MAX_RESULTS - 1);
        max_edit_dist_allowed = p->dist; 
    }
    else { // leader board is not full
        cvec_append(leader_board, &c);
        cvec_sort(leader_board, cmp_correction);
        // Max possible edit distance between any pair of words in this program
        max_edit_dist_allowed = MAX_STRING_LENGTH;
    }
    return max_edit_dist_allowed;
}

/* Print the best alternate spellings for a word to stdout. */ 
void spellcheck(CMap *corpus_map, const char *word, 
                CVector *leader_board, bool print_correct_words)
{
    int max_edit_dist_allowed = MAX_STRING_LENGTH;
    int d;
    const char *corpus_word;
    Correction *correctionp;

    if (is_found(corpus_map, word)) {
        if (print_correct_words) {
            printf("\'%s\' spelled correctly.\n", word);
        }
        return;
    }	
    for (corpus_word = cmap_first(corpus_map); corpus_word !=NULL; 
         corpus_word = cmap_next(corpus_map, corpus_word)) {

        d = edit_dist(corpus_word, word, max_edit_dist_allowed);
        if (d > MAX_STRING_LENGTH) {
            // Edit dist between word and corpus word >= max_edit_dist_allowed
            continue;
        }
        max_edit_dist_allowed = update_leader_board(corpus_map, leader_board, 
                                                    corpus_word, d);
    }
    printf("%s:", word);
    for (correctionp = cvec_first(leader_board); correctionp != NULL; 
         correctionp = cvec_next(leader_board, correctionp)) {
        
        printf(" %s",correctionp->s);
    }
    printf("\n");
}

/* Cleanup function for the leader board vector. */
void cleanup_leader_board(void *p)
{
    Correction *c = p;
    free(c->s); // undo strdup
}

/* Cleanup function for the misspelled words vector. */
void cleanup_misspellings(void *p)
{
    char *s = *(char **)p;
    free(s); // undo strdup
}

/* Find all unique misspellings in the document. 
* XXX TO DO: misspellings should be a map 
*/
void collect_misspellings(CMap *corpus_map, FILE *fp, CVector *misspellings)
{
    char *word;
    int i, match;
    char buf[MAX_STRING_LENGTH + 1];

    while(read_word(fp, buf)) {
        word = strdup(buf);
        s_tolower(word);
        match = cvec_search(misspellings, &word, cmp_words, 0, true);
        if (match == -1) {
            if (!is_found(corpus_map, word)) {
                cvec_append(misspellings, &word);
                cvec_sort(misspellings, cmp_words);
            }
        }		
    }
}

int main(int argc, char *argv[]) 
{   
    bool print_correct_words;
    char *word; 
    char **wordp;
    FILE *fp;
    CMap *corpus_map;
    CVector *misspellings;

    if (argc != 3) {
        fprintf(stderr, "%s: you must specify the corpus and what-to-check. "
                        "The what-to-check argument can be a single word or " 
                        "document.\n", argv[0]);
        exit(1);
    } 
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        perror(argv[1]);
        exit(1);
    }
    corpus_map = build_map(fp);
    fclose(fp);
    if (corpus_map == NULL) {
        perror(argv[1]);
        exit(1);
    }
    misspellings = cvec_create(sizeof(char *), WORDS_CAPACITY_HINT, 
                               cleanup_misspellings);
        
    fp = fopen(argv[2], "r");
    if (fp != NULL) {
        print_correct_words = false;
        collect_misspellings(corpus_map, fp, misspellings);
        fclose(fp);
    }
    else {
        print_correct_words = true;
        if (strlen(argv[2]) > MAX_STRING_LENGTH) {
            fprintf(stderr, "word longer than limit of %d \n", MAX_STRING_LENGTH);
            cvec_dispose(misspellings);
            cmap_dispose(corpus_map);
            exit(1);
        }
        word = strdup(argv[2]);
        s_tolower(word);
        cvec_append(misspellings, &word);
    }	

    for(wordp = (char **)cvec_first(misspellings); wordp != NULL; 
        wordp = (char **)cvec_next(misspellings, wordp)) {
        
        CVector *leader_board = cvec_create(sizeof(Correction), 
                                            LEADER_BOARD_CAPACITY_HINT, 
                                            cleanup_leader_board);
        spellcheck(corpus_map, *wordp, leader_board, print_correct_words); 
        cvec_dispose(leader_board);
    }
    cvec_dispose(misspellings);
    cmap_dispose(corpus_map);
    return 0;
}