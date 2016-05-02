#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "m15.h"

// Used to refer to left and right sibling elements in deque
// We rely on !LEFT == RIGHT and !RIGHT == LEFT
#define LEFT 0
#define RIGHT 1

// Double the size of an allocated block PTR with NMEMB members and update
// NMEMB accordingly.  (NMEMB is only the size in bytes if PTR is a char *.)
#define DOUBLE(ptr,nmemb) realloc (ptr, (nmemb *= 2) * sizeof(*ptr))

#define INIT_SIZE (20)

// Create, initialize, and return an empty deque
Deque dequeCreate(){
    Deque toReturn = malloc(sizeof(struct rbuff));

    toReturn->buffer	= malloc(INIT_SIZE);
    toReturn->length	= 0;
    toReturn->size	= INIT_SIZE;
    toReturn->start	= 0;
    toReturn->end	= 0;

    return toReturn;
}

// Is deque d empty?
int dequeIsEmpty(Deque d){
    return (d == NULL || d->length == 0);
}

// Add char c to deque d on 'side' end. Side can be left or right.
// Fails silently if d is NULL.
void dequePush(Deque d, char c, int side){
    if(d == NULL) return;

    // If buffer is full, copy over to a new array.
    if(d->length == d->size){
	// Malloc a new place to hold chars
	size_t start = d->start;
	size_t size = d->size;
	
	char *newbuffer = malloc(2 * size);

	for(int i = 0; i < d->size; i++){
	    newbuffer[i] = d->buffer[(start + i) % size];
	}

	// Readjust the size, start, and end
	d->size = 2 * size;
	d->start = 0;
	d->end = d->length - 1;

	// Dispose of old buffer, and reassign to new one.
	free(d->buffer);
	d->buffer = newbuffer;
    }

    if(side == RIGHT){
	if(d->length != 0) d->end = (d->end + 1) % d->size;
	d->buffer[d->end] = c;
    }else if(side == LEFT){
	if(d->length != 0) d->start = ((d->start - 1) + d->size) % d->size;
	d->buffer[d->start] = c;
    }
    
    d->length++;
}

// Pop deque element off 'side' end, return null char if empty or d is NULL.
char dequePop(Deque d, int side){
    if(dequeIsEmpty(d)) return '\0';

    char toReturn;
    
    if(side == RIGHT){
	toReturn = d->buffer[d->end];
	if(d->length != 1) d->end = (d->end - 1)  % d->size;
    }else{
	toReturn = d->buffer[d->start];
	if(d->length != 1) d->start = (d->start + 1) % d->size;
    }

    d->length --;
    return toReturn;
}

// Peek 'side'-most element, where side = left or right. 
// Return null char if d is empty or NULL.
char dequePeek(Deque d, int side){
    if(dequeIsEmpty(d)) return '\0';
    
    char toReturn;
    
    if(side == RIGHT){
	toReturn = d->buffer[d->end];
    }else{
	toReturn = d->buffer[d->start];
    }

    return toReturn;
}

// Free all malloc'd memory for deque d
void dequeDestroy(Deque d){
    if(d == NULL) return;

    free(d->buffer);
    free(d);
}

// Print all contents in deque, left to right
void dequePrint(Deque d){
    if(d == NULL) return;

    char *buffer = d->buffer;
    size_t size = d->size;
    size_t start = d->start;
    size_t length = d->length;

    for(int i = 0; i < length; i++){
	putchar(buffer[(start + i) % size]);
    }
    
    putchar('\n');
}

// Return a malloc'd string with contents of input.
// Will not alter input. Will return NULL if input empty.
char * dequeConvertToString(Deque input){
    // Malloc a new place to hold chars
    size_t start = input->start;
    size_t size = input->size;
    
    char *toReturn = malloc(input->length + 1);

    for(int i = 0; i < input->length; i++){
	toReturn[i] = input->buffer[(start + i) % size];
    }

    // Cap return string
    toReturn[input->length] = '\0';

    return toReturn;
}


// Copy alphanumeric text contained within brackets to a malloc'd string
// Returns NULL if error.
char *dequeCopyAlphanumericBracketsToString(Deque input){
    if(dequeIsEmpty(input)){
	fprintf(stderr, "m15: expected {, hit end of input\n");
	return NULL;
    }

    // Try to remove first {
    char n;
    if((n = dequePop(input,LEFT)) != '{'){
	fprintf(stderr, "m15: expected {, hit %c\n", n);
	return NULL;
    }
    
    // If nothing followed {, then error
    if(dequeIsEmpty(input)){
	fprintf(stderr, "m15: bracket followed by end of inpute\n");
	return NULL;
    }

    off_t size = 6;
    off_t length = 0;
    char *contents = malloc(size);

    // Copy alphanumeric portion to contents string
    while(!dequeIsEmpty(input) && isalpha(dequePeek(input, LEFT))){
	// Leave room for null char, hence ' + 1 '
	if(length + 1 == size){
	    contents = DOUBLE(contents, size);
	}

	contents[length] = dequePop(input, LEFT);
	length++;
    }

    // Cap contents string
    contents[length] = '\0';

    // Next char should be {, else error
    if(dequeIsEmpty(input)){
	// Hit end of file before {, issue error
	fprintf(stderr, "m15: expected {, hit end of file\n");
	
	free(contents);
	return NULL;
    }else if(dequePeek(input, LEFT) == '}'){
	// Success
	dequePop(input, LEFT);
	return contents;
    }else{
	// Hit a non-alphanumeric char that isn't a {
	fprintf(stderr, "m15: expected {, hit %c\n", dequePeek(input, LEFT));
	
	free(contents);
	return NULL;
    }
}

// Copy contents of a bracket-balanced string to a deque.
// Returns NULL if error.
Deque dequeCopyBalancedBracketsToDeque(Deque input){

    if(dequeIsEmpty(input)){
	fprintf(stderr, "m15: expected {, hit end of input\n");
	return NULL;
    }

    // Make sure next char is {
    if(dequePeek(input,LEFT) != '{'){
	fprintf(stderr, "m15: expected {, hit %c\n", dequePeek(input, LEFT));
	return NULL;
    }
    

    // Now add contents of brackets, including brackets
    Deque toReturn = dequeCreate();

    //Pop first { into toReturn. Already made sure it's {.
    dequePush(toReturn, dequePop(input, LEFT), RIGHT);
    
    // Track num of left and right brackets. Already hit one left bracket.
    int numLeftBrackets = 1;
    int numRightBrackets = 0;

    while(!dequeIsEmpty(input) && numLeftBrackets > numRightBrackets){
	char c = dequePop(input, LEFT);
	
	// Be careful regarding escape chars
	if(c == '\\'){
	    char next = dequePeek(input, LEFT);

	    // Only escape these characters
	    if(next == '{' || next == '}' || next == '\\'){
		dequePush(toReturn, dequePop(input, LEFT), RIGHT);
		continue;
	    }else{
		dequePush(toReturn, c, RIGHT);
		continue;
	    }
	}

	if(c == '}'){
	    numRightBrackets++;
	}else if(c == '{'){
	    numLeftBrackets++;
	}

	dequePush(toReturn, c, RIGHT);
    }

    // Check if end of input stream prematurely reached
    if(dequeIsEmpty(input) && dequePeek(toReturn, RIGHT) != '}'){
	dequeDestroy(toReturn);
	fprintf(stderr, "m15: expected }, hit end of input stream\n");
	return NULL;
    }

    // Remove left and right brackets
    dequePop(toReturn, LEFT);
    dequePop(toReturn, RIGHT);

    return toReturn;
}

// Copy one deque onto another, maintaining L to R order.
// Side refers to side of 'to' that it will be appended to.
// Does not alter from.
void dequeAppend(Deque from, Deque to, int side){
    if(dequeIsEmpty(from)) return;
    


    char *buffer    = from->buffer;
    
    size_t size	    = from->size;
    size_t start    = from->start;
    size_t end	    = from->end;
    size_t length   = from->length;

    if(side == RIGHT){
	for(int i = 0; i < length; i++){
	    dequePush(to, buffer[(start + i) % size], RIGHT);
	}
    }else if (side == LEFT){
	for(int i = 0; i < length; i++){
	    dequePush(to, buffer[((end - i) + size) % size], LEFT);
	}
    }
}

// Read in file 'path' and return a deque with the contents of file.
// Return NULL if file doesn't exist. Does not issue an error.
// Pass NULL to have it read from stdin.
Deque dequeReadFile(char *path){
    
    FILE *ifp;

    if(path == NULL) {
	ifp = stdin;
    }else{
	ifp = fopen(path, "r");
    }

    if(ifp == NULL){
	return NULL;
    }

    Deque toReturn = dequeCreate();

    int c;
    while((c = getc(ifp)) != EOF){
	// If we hit a '%' that is not escaped, fast forward.
	// If we hit a '%' that IS escaped, strip escape char.
	if(c == '%' && dequePeek(toReturn, RIGHT) != '\\'){
	    fastForward(ifp);
	}else if (c == '%' && dequePeek(toReturn, RIGHT)){
	    // Strip escape char.
	    dequePop(toReturn, RIGHT);
	    dequePush(toReturn, c, RIGHT);
	}else{
	    dequePush(toReturn, c, RIGHT);
	}
    }
    
    fclose(ifp);
    return toReturn;
}



int test (int argc, char *argv[]){
    Deque d = dequeCreate();

    int c;
    while((c = getchar()) != EOF){
	dequePush(d, (char) c, RIGHT);
    }

    putchar('\n');
    dequePrint(d);

    printf("Leftmost char is: %c\n", dequePeek(d, LEFT));
    printf("Rightmost char is: %c\n", dequePeek(d, RIGHT));
    
    printf("Pop off 5 chars\n");
    for(int i = 0; i < 5; i++){
	dequePop(d, i % 2);
    }

    dequePrint(d);
    
    printf("Now as a string: %s\n", dequeConvertToString(d));
    
    Deque holder = dequeCreate();

    dequePush(holder, 'N', LEFT);
    dequePush(holder, 'o', LEFT);
    dequePush(holder, 'w', LEFT);
    dequePush(holder, ' ', LEFT);
    dequePush(holder, 'a', LEFT);
    dequePush(holder, 's', LEFT);
    dequePush(holder, ' ', LEFT);
    dequePush(holder, 'a', LEFT);
    dequePush(holder, ' ', LEFT);
    dequePush(holder, 'd', LEFT);
    dequePush(holder, 'e', LEFT);
    dequePush(holder, 'q', LEFT);
    dequePush(holder, 'u', LEFT);
    dequePush(holder, 'e', LEFT);
    dequePush(holder, '\n', LEFT);

    dequeAppend(d, holder, RIGHT);
    dequePrint(holder);


    return 0;
}

