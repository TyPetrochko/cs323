// m15.h                                          Tyler Petrochko(09/23/15)
//
// Based off Stan Eisenstat's m15.h for Yale CPSC 323.
//
// System header files and macros for m15

#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Used to refer to left and right sibling elements in deque
// We rely on !LEFT == RIGHT and !RIGHT == LEFT
#define LEFT 0
#define RIGHT 1

// Write message to stderr using format FORMAT
#define WARN(format,...) fprintf (stderr, "m15: " format "\n", __VA_ARGS__)

// Write message to stderr using format FORMAT and exit.
#define DIE(format,...)  WARN(format,__VA_ARGS__), exit (EXIT_FAILURE)

// Double the size of an allocated block PTR with NMEMB members and update
// NMEMB accordingly.  (NMEMB is only the size in bytes if PTR is a char *.)
#define DOUBLE(ptr,nmemb) realloc (ptr, (nmemb *= 2) * sizeof(*ptr))

#define INIT_SIZE (20)

struct rbuff {
    char *buffer;
    long long length;
    long long size;
    long long start;
    long long end;
};

typedef struct rbuff * Deque;

// Deque elements comprise the nodes in a doubly linked list implementation 
// of a deque
struct dequeElement {
    // Doubly linked list elements have a left and right next
    struct dequeElement *next[2];
    char c;
};

// Create, initialize, and return an empty deque
Deque dequeCreate ();

// Is deque d empty?
int dequeIsEmpty(Deque d);

// Add char c to deque d on 'side' end. Side can be left or right.
// Fails silently if d is NULL.
void dequePush(Deque d, char c, int side);

// Pop deque element off 'side' end, return null char if empty or d is NULL.
char dequePop(Deque d, int side);

// Peek 'side'-most element, where side = left or right. 
// Return null char if d is empty or NULL.
char dequePeek(Deque d, int side);

// Free all malloc'd memory for deque d
void dequeDestroy(Deque d);

// Print all contents in deque, left to right
void dequePrint(Deque d);

// Print all contents in deque, left to right, and ignore escaping \s.
// Empties d but does not c destroy it.
void printResultAndEmpty(Deque d);

// Copy alphanumeric text contained within brackets to a malloc'd string.
// Returns NULL if error.
char *dequeCopyAlphanumericBracketsToString(Deque input);

// Copy contents of a bracket-balanced string to a malloc'd string.
// Returns NULL if error.
Deque dequeCopyBalancedBracketsToDeque(Deque input);

// Return a malloc'd string with contents of input.
// Will not alter input. Will return NULL if input empty.
char * dequeConvertToString(Deque input);

// Copy one deque onto another, maintaining L to R order.
// Side refers to side of 'to' that it will be appended to.
// Does not alter from.
void dequeAppend(Deque from, Deque to, int side);

// Read from file "path" and return a Deque with contents. 
// Return NULL if file DNE. Will factor '%'s into the equation.
Deque dequeReadFile(char *path);

// For logging macroes
struct dict {
    char * macro;
    char * replacement;
    struct dict * next;
};

typedef struct dict** Dict;

// Create and initialize a new dict
Dict dictCreate();

// Log macro in dict d, with its replacement
void dictAddMacro(Dict d, char *macro, char *replacement);

// Check if dict d contains macro
int dictContains(Dict d, char *macro);

// Free all malloc'd memory associated with dict d
void dictDestroy(Dict d);

// Return pointer to malloc'd string containing macro's replacement
char * dictGetReplacement(Dict d, char *macro);

// Free all memory associated with macro
void dictDeleteMacro(Dict D, char *macro);

// Control function that reads from input, and calls necessary macro functions.
// Returns 0 if successful, -1 if failure.
int process(Deque input, Deque output);

// Modularized function for handling the case of a backslash.
// Returns 0 if successful, -1 if failure.
int processBackslash(Deque input, Deque output);

// Remove trailing whitespace until next alphanumeric char.
void fastForward(FILE *f);

// Define a macro from input stream, return 0 if success, -1 if failure.
// Format of input stream should be {alphanumeric}{{bracketbalanced}}
int def(Deque input);

// Expand a macro. Will not destroy macro or input.
// Return 0 if success, -1 if failure.
int expandMacro(Deque input, char *macro);

// Undefine a macro from input stream. Return 0 if success, -1 if failure.
// Format of input stream should be {macro}
int undef(Deque input);

// If VALUE is nonempty brace-balanced string, print THEN, else print ELSE.
// Takes the format {VALUE}{THEN}{ELSE}. Note 'if' is a resesrved keyword.
// Return 0 if success, -1 if failure.
int iff(Deque input);

// If NAME is a defined macro, print THEN, else print ELS.
// Takes the format {NAME}{THEN}{ELS}
// Return 0 if success, -1 if failure.
int ifdef(Deque input);

// Include file PATH. Format should be {PATH}.
// Return 0 if success, -1 if failure.
int include(Deque input);

// Expand AFTER recursively, then push BEFORE + AFTER back up input stream.
// Format should be {BEFORE}{AFTER}. Return 0 if success, -1 if failure.
int expandAfter(Deque input);

