#include "m15.h"
#define ERR_LENGTH (100)

Dict macros;
char error[ERR_LENGTH];


int main (int argc, char * argv[]){

    snprintf(error, ERR_LENGTH, "m15: an unknown error ocurred\n");

    macros = dictCreate();

    Deque input = dequeCreate();
    Deque output = dequeCreate();

    // If one argument, read from stdin. Else read from files on cmdline.
    if(argc == 1){
	Deque fcontents = dequeReadFile(NULL);
	
	// User may have mis-entered a filename.
	if(fcontents == NULL){
	    fprintf(stderr, "m15: could not read file\n");
	    dictDestroy(macros);
	    dequeDestroy(input);
	    dequeDestroy(output);
	    return -1;
	}

	// Tack on file contents to input deque.
	dequeAppend(fcontents, input, RIGHT);
	dequeDestroy(fcontents);
    }else{
	// Read all command line files to input
	for(int i = 1; i < argc; i++){
	    Deque fcontents = dequeReadFile(argv[i]);
	    
	    // User may have mis-entered a filename.
	    if(fcontents == NULL){
		fprintf(stderr, "m15: could not read file %s\n", argv[i]);
		dictDestroy(macros);
		dequeDestroy(input);
		dequeDestroy(output);
		return -1;
	    }

	    // Tack on file contents to input deque.
	    dequeAppend(fcontents, input, RIGHT);
	    dequeDestroy(fcontents);
	}
    }
    
    if(process(input, output) == -1){
	dictDestroy(macros);
	dequeDestroy(input);
	dequeDestroy(output);
	fprintf(stderr, error);
	return -1;
    }

    printResultAndEmpty(output);

    dictDestroy(macros);
    dequeDestroy(input);
    dequeDestroy(output);
    return 0;
}

// Control logic that detects backslashes, %s, and regular chars.
int process(Deque input, Deque output){
    while(!dequeIsEmpty(input)){
	char c = dequePop(input, LEFT);
	
	if(c == '\\'){
	    if(processBackslash(input, output) == -1) return -1;
	}else{
	    dequePush(output, c, RIGHT);
	}
    }
    return 0;
}

// Already got a backslash.
// Decide correct command to run after receiving backslash in input deque.
int processBackslash(Deque input, Deque output){
    if(dequeIsEmpty(input)) {
	dequePush(output, '\\', RIGHT);
	return 0;
    }

    char nextChar = dequePeek(input, LEFT);

    // Check if \ is an escape char
    if(nextChar == '{' || nextChar == '}' 
	    || nextChar == '\\' || nextChar == '#'){
	
	dequePush(output, '\\', RIGHT);
	dequePush(output, dequePop(input, LEFT), RIGHT);
	return 0;
    }

    // Check if non-alphanumeric next char
    if(!isalpha(nextChar)){
	dequePush(output, '\\', RIGHT);
	return 0;
    }

    // Copy deque into a string
    long long size = 6;
    long long length = 0;
    char *macro = malloc(size);

    while(!dequeIsEmpty(input) && dequePeek(input, LEFT) != '{'){
	// Leave an empty space for null cap, hence " + 1 "
	if(length + 1 == size) macro = DOUBLE(macro, size);
	
	macro[length] = dequePop(input, LEFT);
	
	length++;
    }

    macro[length] = '\0';
    
    // Check for errors:
    //     If the length of the macro is 0, but deque not empty
    //     If we hit end of deque, without hitting a { first
    if(strlen(macro) == 0){
	if(!dequeIsEmpty(input)) {
	    snprintf(error, ERR_LENGTH, "m15: something weird happened\n");
	    free(macro);
	    return -1;
	}else {
	    free(macro);
	    return 0;
	}
    }else if(dequeIsEmpty(input)){
        snprintf(error, ERR_LENGTH, "m15: expected {, hit end\n");
	return -1;
    }

    // Determine which macro is being called
    if(!strcmp(macro, "def")){
	// Try to define macro
	if(def(input) == -1){
	    free(macro);
	    return -1;
	}
    }else if(!strcmp(macro, "undef")){
	if(undef(input) == -1){
	    free(macro);
	    return -1;
	}
    }else if(!strcmp(macro, "ifdef")){
	if(ifdef(input) == -1){
	    free(macro);
	    return -1;
	}
    }else if(!strcmp(macro, "if")){
	if(iff(input) == -1){
	    free(macro);
	    return -1;
	}	
    }else if(!strcmp(macro, "include")){
	if(include(input) == -1){
	    free(macro);
	    return -1;
	}
    }else if(!strcmp(macro, "expandafter")){
	if(expandAfter(input) == -1){
	    free(macro);
	    return -1;
	}
    }else{
	if(expandMacro(input, macro) == -1){
	    free(macro);
	    return -1;
	}
    }

    free(macro);
    return 0;
}

// Encountered \def, so define the macro. 
// Input stream should look like {alphanumericname}{bracebalanced{string}}.
// Returns 0 if success, -1 if failure.
int def(Deque input){
    if(dequeIsEmpty(input)){
	snprintf(error, ERR_LENGTH, "m15: error defining macro, hit end of input stream\n");
	return -1;
    }else if(dequePeek(input, LEFT) != '{'){
	snprintf(error, ERR_LENGTH, "m15: expected def{, got def%c\n", dequePeek(input, LEFT));
	return -1;
    }
    
    char * macro = dequeCopyAlphanumericBracketsToString(input);
    Deque replacement = dequeCopyBalancedBracketsToDeque(input);

    // Issue error if one of them failed, and clean up memory.
    if (macro == NULL || replacement == NULL){
	if(macro != NULL) free(macro);
	if(replacement != NULL) dequeDestroy(replacement);
	snprintf(error, ERR_LENGTH, "m15: error defining macro\n");
	return -1;
    }
    
    char * sreplacement = dequeConvertToString(replacement);

    // Clean up intermediary deque
    dequeDestroy(replacement);
    dictAddMacro(macros, macro, sreplacement);
    
    // Since dictAddMacros copies strings, we can delete them noww
    free(macro);
    free(sreplacement);
    return 0;
}

// Undefine a macro from input stream. Return 0 if succes, -1 if failure.
// Format of input stream should be {macro}
int undef(Deque input){
    if(dequeIsEmpty(input)){
	snprintf(error, ERR_LENGTH, "m15: error undefining macro, hit end of input stream\n");
	return -1;
    }

    char *macro = dequeCopyAlphanumericBracketsToString(input);
    
    if(macro == NULL){
	snprintf(error, ERR_LENGTH, "m15: error retrieving macro to undefine\n");
	return -1;
    }

    if(!dictContains(macros, macro)){
	snprintf(error, ERR_LENGTH, "m15: could not undefine macro %s\n", macro);
	return -1;
    }

    dictDeleteMacro(macros, macro);
    free(macro);
    return 0;
}
// If VALUE is nonempty brace-balanced string, print THEN, else print ELS.
// Takes the format {VALUE}{THEN}{ELS}. Note 'if' is a resesrved keyword.
// Return 0 if success, -1 if failure.
int iff(Deque input){
    if(dequeIsEmpty(input)){
	snprintf(error, ERR_LENGTH, "m15: expected if arguments, reached end of input\n");
	return -1;
    }

    Deque value = dequeCopyBalancedBracketsToDeque(input);
    Deque then = dequeCopyBalancedBracketsToDeque(input);
    Deque els = dequeCopyBalancedBracketsToDeque(input);

    // A lot can go wrong.
    if (value == NULL || then == NULL || els == NULL){
	if(value != NULL) dequeDestroy(value);
	if(then != NULL) dequeDestroy(then);
	if(els != NULL) dequeDestroy(els);
	
	snprintf(error, ERR_LENGTH, "m15: had trouble constructing valid if/ifdef statement\n");
	return -1;
    }
    
    if(!dequeIsEmpty(value)){
	dequeAppend(then, input, LEFT);
    }else{
	dequeAppend(els, input, LEFT);
    }

    // Free all associated memory
    dequeDestroy(value);
    dequeDestroy(then);
    dequeDestroy(els);

    return 0;
}

// If NAME is a defined macro, print THEN, else print ELS.
// Takes the format {NAME}{THEN}{ELS}
// Return 0 if success, -1 if failure.
int ifdef(Deque input){
    if(dequeIsEmpty(input)){
	snprintf(error, ERR_LENGTH, "m15: expected ifdef arguments, reached end of input\n");
	return -1;
    }
    
    char *name = dequeCopyAlphanumericBracketsToString(input);

    // Re-use iff command by just converting to iff format
    if(dictContains(macros, name)){
	dequePush(input, '}', LEFT);
	dequePush(input, '.', LEFT);
	dequePush(input, '{', LEFT);
    }else{
	dequePush(input, '}', LEFT);
	dequePush(input, '{', LEFT);
    }

    free(name);

    if(iff(input) == -1){
	return -1;
    }else{
	return 0;
    }
}

// Include file PATH. Format should be {PATH}.
// Return 0 if success, -1 and printf's error if failure.
int include(Deque input){
    if(dequeIsEmpty(input)){
	snprintf(error, ERR_LENGTH, "m15: expected include path, reached end of input\n");
	return -1;
    }

    // Try to get path name as deque.
    Deque path = dequeCopyBalancedBracketsToDeque(input);

    if(path == NULL){
	snprintf(error, ERR_LENGTH, "m15: error reading in included file\n");
	dequeDestroy(path);
	return -1;
    }

    char *spath = dequeConvertToString(path);

    // Hold buffered file contents before we push back up input stream.
    Deque fbuf = dequeReadFile(spath);

    if(fbuf == NULL){
	snprintf(error, ERR_LENGTH, "m15: error reading in included file\n");
	dequeDestroy(path);
	free(spath);
	return -1;
    }

    dequeAppend(fbuf, input, LEFT);

    // Free all associated memory
    free(spath);
    dequeDestroy(path);
    dequeDestroy(fbuf);

    return 0;
}

// Expand AFTER recursively, then push BEFORE + AFTER back up input stream.
// Format should be {BEFORE}{AFTER}. Return 0 if success, -1 if failure.
int expandAfter(Deque input){

    if(dequeIsEmpty(input)){
	snprintf(error, ERR_LENGTH, "m15: expected expandafter statement, hit end of input\n");
	return -1;
    }

    Deque before = dequeCopyBalancedBracketsToDeque(input);
    Deque after = dequeCopyBalancedBracketsToDeque(input);

    if(before == NULL || after == NULL){
	snprintf(error, ERR_LENGTH, "m15: error reading in expandafter arguments\n");

	if(before != NULL) dequeDestroy(before);
	if(after != NULL) dequeDestroy(after);

	return -1;
    }

    // Make a deque to store recursively processed AFTER
    Deque expansion = dequeCreate();

    if(process(after, expansion) == -1){
	dequeDestroy(before);
	dequeDestroy(after);
	dequeDestroy(expansion);
	return -1;
    }

    // Stick BEFORE to expansion, then the whole thing onto input
    dequeAppend(before, expansion, LEFT);
    dequeAppend(expansion, input, LEFT);
	
    dequeDestroy(before);
    dequeDestroy(after);
    dequeDestroy(expansion);

    return 0;
}

// Expand a macro. Will not destroy macro or input.
// Return 0 if success, -1 if failure.
// Format if input should be {arg}
int expandMacro(Deque input, char *macro){
    if(!dictContains(macros, macro)){
	snprintf(error, ERR_LENGTH, "m15: could not find macro %s\n", macro);	
	return -1;
    }

    char *replacement = dictGetReplacement(macros, macro);
    Deque arg = dequeCopyBalancedBracketsToDeque(input);

    if(arg == NULL){
	return -1;
    }

    for(int i = strlen(replacement) -1; i >= 0; i--){
	char c = replacement[i];
	
	if(c == '#'){
	    dequeAppend(arg, input, LEFT); 
	}else{
	    dequePush(input, c, LEFT);
	}
    }

    dequeDestroy(arg);
    return 0;
}

// In reponse to an encountered %, fast forward to next non-whitespace/tab/newline.
// Fails silently if f is null pointer.
void fastForward(FILE *f){
    if(f == NULL) return;
    
    // Fast forward until first return char
    int c;
    while((c = getc(f)) != EOF && c != '\n');

    if(c == EOF) return;

    // Strip all spaces and tabs
    while((c = getc(f)) != EOF && (c == ' ' || c == '\t'));

    // We hit a non-space/non-tab character. Put it back.
    if(c != EOF){
	ungetc((char) c, f);
    }
}

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

// Print all contents in deque, left to right, and ignore escaping \s.
// Empties d but does not c destroy it.
void printResultAndEmpty(Deque d){

    char c;
    while(!dequeIsEmpty(d)){
	c = dequePop(d, LEFT);

	if (c == '\\'){
	    // If backslash is an escape char, skip it. Else it doesn't matter.
	    char next = dequePeek(d, LEFT);
	    if(next == '\\' || next == '#' || next == '{' || next == '}' || next == '%'){
		putchar(dequePop(d, LEFT));
	    }else{
		putchar(c);
	    }
	}else{
	    putchar(c);
	}
    }
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
	snprintf(error, ERR_LENGTH, "m15: expected {, hit end of input\n");
	return NULL;
    }

    // Try to remove first {
    char n;
    if((n = dequePop(input,LEFT)) != '{'){
	snprintf(error, ERR_LENGTH, "m15: expected {, hit %c\n", n);
	return NULL;
    }
    
    // If nothing followed {, then error
    if(dequeIsEmpty(input)){
	snprintf(error, ERR_LENGTH, "m15: bracket followed by end of inpute\n");
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
	snprintf(error, ERR_LENGTH, "m15: expected {, hit end of file\n");
	
	free(contents);
	return NULL;
    }else if(dequePeek(input, LEFT) == '}'){
	// Success
	dequePop(input, LEFT);
	return contents;
    }else{
	// Hit a non-alphanumeric char that isn't a {
	snprintf(error, ERR_LENGTH, "m15: expected {, hit %c\n", dequePeek(input, LEFT));
	
	free(contents);
	return NULL;
    }
}

// Copy contents of a bracket-balanced string to a deque.
// Returns NULL if error.
Deque dequeCopyBalancedBracketsToDeque(Deque input){

    if(dequeIsEmpty(input)){
	snprintf(error, ERR_LENGTH, "m15: expected {, hit end of input\n");
	return NULL;
    }

    // Make sure next char is {
    if(dequePeek(input,LEFT) != '{'){
	snprintf(error, ERR_LENGTH, "m15: expected {, hit %c\n", dequePeek(input, LEFT));
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
		dequePush(toReturn, '\\', RIGHT);
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
    if(dequeIsEmpty(input) && numLeftBrackets != numRightBrackets){
	dequeDestroy(toReturn);
	snprintf(error, ERR_LENGTH, "m15: expected }, hit end of input stream\n");
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
	}else if (c == '%' && dequePeek(toReturn, RIGHT) == '\\'){
	    // Strip escape char.
	    dequePush(toReturn, c, RIGHT);
	}else{
	    dequePush(toReturn, c, RIGHT);
	}
    }
    
    fclose(ifp);
    return toReturn;
}


// Return an initialized Dict
Dict dictCreate(){
    // Malloc space for a pointer to a dict element.
    Dict toReturn = malloc(sizeof(struct dict *));
    
    // It shouldn't point to anything, yet
    *toReturn = NULL;
    
    return toReturn;
}

// Is this dict empty? Return t/f value.
int dictIsEmpty(Dict  d){
    return (*d == NULL); 
}

// Add macro and its replacement to dict.
void dictAddMacro(Dict  d, char *macro, char *replacement){
    
    // Keep runtime resources proportional to number of distinct macros
    if(dictContains(d, macro)){
	return;
    }
    struct dict * toAdd = malloc(sizeof(struct dict));
    
    // Malloc extra char for null char
    toAdd->macro = malloc(strlen(macro) + 1);
    toAdd->replacement = malloc(strlen(replacement) + 1);

    strcpy(toAdd->macro, macro);
    strcpy(toAdd->replacement, replacement);

    toAdd->next = *d;

    *d = toAdd;
}

// Does this dict contain fileName? Return t/f value.
int dictContains(Dict d, char * macro){
    struct dict * iterator = *d;
    
    while (iterator != NULL){
	if (!strcmp(macro, iterator->macro)){
	    return 1;
	}else{
	    iterator = iterator->next;
	}
    }
    
    return 0;
}

// Free all memory for a given dict.
void dictDestroy(Dict d){
    struct dict * iterator = *d;

    while (iterator != NULL) {
	struct dict * toFree = iterator;
	iterator = iterator->next;

	free(toFree->macro);
	free(toFree->replacement);
	free(toFree);
    }

    // Free malloc'd address pointer to first element
    free(d);
}

// Remove macro from dict d. May cause error if d does not contain macro.
void dictDeleteMacro(Dict d, char *macro){
    struct dict *iterator = *d;

    // If first element is macro, iterating will cause an error
    if(!strcmp(iterator->macro, macro)){
	*d = iterator->next;
	free(iterator->macro);
	free(iterator->replacement);
	free(iterator);
	return;
    }else{
	struct dict *lastIterator = iterator;
	iterator = iterator->next;
	while (iterator != NULL) {
	    if(!strcmp(iterator->macro, macro)){
		lastIterator->next = iterator->next;
		free(iterator->macro);
		free(iterator->replacement);
		free(iterator);
		return;
	    }else{
		lastIterator = iterator;
		iterator = iterator->next;
	    }
	}
    }
}

// Get a pointer to malloc'd string containing the replacement for macro
char * dictGetReplacement(Dict d, char *macro){
    struct dict * iterator = *d;
    
    while (iterator != NULL){
	if (!strcmp(macro, iterator->macro)){
	    return iterator->replacement;
	}else{
	    iterator = iterator->next;
	}
    }
    
    return NULL;
}

