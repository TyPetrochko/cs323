#include "StringTable.h"
#include "lzw.h"
#include <limits.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>


// This hash function provided by Stanley Eisenstat.
//
// Return an int from 0 to size - 1
int hash(int prefix, char k, int size){
    return (((unsigned)(prefix) << CHAR_BIT) ^ ((unsigned) (k))) % size;
}

// Take an already-malloc'ed stEntry and hash it in st.
void stHashTuple(StringTable st, stEntry *toHash){
    int index = hash(toHash->prefix, toHash->k, st->size + 1);

    toHash->next = st->hashArray[index];
    st->hashArray[index] = toHash;
}

// Create, init, and return a malloc'ed String Table.
// Note: 9 (MIN_BITS) <= maxBits <= 20 (MAX_BITS) must hold, or return NULL.
StringTable stCreateAndInit(int maxBits){
    if (maxBits < MIN_BITS || maxBits > MAX_BITS) return NULL;

    StringTable toReturn = malloc(sizeof(struct stringTable));
    
    toReturn->usage = 0;
    toReturn->size = 1 << MIN_BITS;
    toReturn->len = 0;
    toReturn->numBits = MIN_BITS;
    toReturn->maxBits = maxBits; 

    // Malloc space for the stEntries, and the hash table.
    toReturn->array = malloc(sizeof(stEntry) * toReturn->size);
    
    // Use hash array of size 1 + StringTable's size
    toReturn->hashArray = calloc(toReturn->size + 1, sizeof(stEntry *));

    // Create null tuple
    toReturn->array[0].k = '\0';
    toReturn->array[0].prefix = EMPTY;
    toReturn->array[0].code = EMPTY;
    toReturn->array[0].usage = 0;
    
    // Since empty tuple the first element, its address is toReturn->hashArray
    stHashTuple(toReturn, toReturn->array);

    toReturn->len++;

    // Now hash chars with values 0-255 in string table
    int index;
    char k;
    for (int i = 0; i < 256; i++){
	index = i + 1;
	k = (char) i;
	toReturn->array[index].k = k;
	toReturn->array[index].prefix = EMPTY;
	toReturn->array[index].code = index;
	toReturn->array[index].usage = 0;

	stHashTuple(toReturn, toReturn->array + index);
	toReturn->len++;
    }

    // Set all tuples' usage initially 0.
    for(int k = toReturn->len; k < toReturn->size; k++){
	toReturn->array[k].usage = 0;
    }

    // Remember original length so we can sync up encode and decode.
    // We know origLen + num codes received/sent is equal for both at equal
    // times.
    toReturn->origLen = toReturn->len;
    return toReturn;
}

// Check if code exists in st.
int stCodeExists(StringTable st, int code){
    return (code < st->len);
}

// Will cause error if code does not exist, 
// i.e. caller must check code exists first!
stEntry stGetTuple(StringTable st, int code){
    stEntry toReturn = st->array[code];
    return toReturn;
}

// Get the code associated with prefix, k, else return -1 if does not exist.
int stGetCode(StringTable st, int prefix, char k){
    int index = hash(prefix, k, st->size + 1);
    
    stEntry * iterator = st->hashArray[index];
    while (iterator != NULL){
	if(iterator->k == k && iterator->prefix == prefix){
	    return iterator->code;
	}else{
	    iterator = iterator->next;
	}
    }

    // Couldn't find it
    return -1;
}

// Check if st is full
int stFull(StringTable st){
    return (st->len == st->size && st->numBits == st->maxBits);
}

// This is called by decode to predict how many codes were in encode's string 
// table. This is helpful when we want to decide when to prune.
unsigned long long stSyncedNumCodes(StringTable st){
    return st->origLen + st->usage - 1;
}

// Calculate # of bits to represent a code in String Table, keeping encoding
// and decoding in mind. Partially adopted from:
// http://stackoverflow.com/questions/12349498/minimum-number-of-bits-to-represent-number
int stNumBits(StringTable st){
    if(stFull(st)){
	return(st->numBits);
    }
    unsigned long long numCodes = stSyncedNumCodes(st);
    int result = 0;
    while (numCodes){
	numCodes >>= 1;
	result++;
    }
    return result;
}

// Increment the StringTable's numBits by one. This involves reallocing its
// array, making a new hash array, rehashing all the elements, then deleting
// the old hash array.
// NOTE: Assumes that numbits < maxBits
void stIncreaseNumBits(StringTable st){
    if(st->numBits >= st->maxBits) return;

    // Increment numBits and readjust size
    st->numBits++;
    st->size = 1 << st->numBits;

    // Realloc. Note - we will need a new hash array, since new hash indices.
    st->array = realloc(st->array, sizeof(stEntry) * st->size);
    free(st->hashArray);
    st->hashArray = calloc(st->size + 1, sizeof(stEntry));

    // Re-hash all the elements in array
    for (int i = 0; i < st->len; i++){
	stHashTuple(st, st->array + i);
    }

    // Set all usages to 0
    for (int k = st->len; k < st->size; k++){
	st->array[k].usage = 0;
    }
}

// Assume not full, else an error will occur..
void stInsert(StringTable st, int prefix, char k){
    if(st->len >= st->size && st->numBits == st->maxBits) 
	fprintf(stderr, "Programming error: Forgot to check if string table full\n");
    
    // Check if we need to increase number of bits
    if(st->len == st->size && st->numBits < st->maxBits){
	stIncreaseNumBits(st);
    }

    int index = st->len;

    if(index == prefix)
	fprintf(stderr, "Programming error: Added code %d with its own prefix\n", prefix);

    st->array[index].k = k;
    st->array[index].prefix = prefix;
    st->array[index].code = index;

    stHashTuple(st, st->array + index);
    
    st->len++;
    
}

// For debugging, print 'pretty' string table
void stPrint(StringTable st, char *path){

    FILE *fp;
    if (path == NULL){
	fp = stderr;
    }
    else{
	struct stat buf;
	if(stat(path, &buf) == 0){
	    // File exists
	    return;
	}
	fp = fopen(path, "w");
    }

    
    fprintf(fp, "Printing String Table:\n\n");

    fprintf(fp, "CODE\t\tPREFIX\t\tK\t\tUSAGE\n");
    fprintf(fp, "***\t\t***\t\t***\t\t***\n\n");

    int len = st->len;
    int i;
    for(i = 0; i < len; i++){
	stEntry * e = st->array + i;
	
	if (e->code == EMPTY){
	    fprintf(fp, "EMPTY\t\t");
	}else{
	    fprintf(fp, "%d\t\t", e->code);
	}
	
	if (e->prefix == EMPTY){
	    fprintf(fp, "EMPTY\t\t");
	}else{
	    fprintf(fp, "%d\t\t", e->prefix);
	}

	fprintf(fp, "%d\t\tUsage = %d\n", (int) e->k, e->usage);
    }

    fprintf(fp, "*************\n");
    fprintf(fp, " SUMMARY: %d/%d spots in string table used\n", len, st->size);
    fprintf(fp, "*************\n");

    fclose(fp);
}

// Save string table to 'path'. Return T if success, F if failure.
// Implementation-wise, we just save the length of the table's array, followed
// by the array of stEntries, and recalculate their next pointers on restoring.
int stSerialize(StringTable st, char * path){
    FILE *f = fopen(path, "wb");

    if (f == NULL) return 0;

    for(int i = FIRST_ENTRY; i < st->len; i++){
	stEntry toWrite = st->array[i];
	fwrite(&toWrite.k, sizeof(char), 1, f);
	fwrite(&toWrite.prefix, sizeof(int), 1, f);
    }

    fclose(f);
    return 1;
}

// Recover a string table from char * path. Return NULL if failure.
// Does not verify that the string table is valid.
StringTable stRecover(char * path, int maxBits){
    FILE *f = fopen(path, "rb");

    if (f == NULL) return NULL;

    StringTable toReturn = stCreateAndInit(maxBits);

    char k;
    int prefix; 
    int err = 0;
    
    // Read in the array
    while(1){
	
	// If first one fails, we hit the end of file
	if(fread(&k, sizeof(char), 1, f) < 1)
	    break;

	// If first one doesn't fail, but second does, there is "extra leftover"
	// thus the data is corrupted
	if(fread(&prefix, sizeof(int), 1, f) < 1){
	    err = 1;
	    break;
	}

	// Do not add the same code twice
	if(stGetCode(toReturn, prefix, k) != -1)
	    err = 1;

	// Prefix must be length than code (we're about to add at index "len")
	if(prefix >= toReturn->len)
	    err = 1;

	// Prefix must already be in the string table, and not empty
	if(!stCodeExists(toReturn, prefix) || prefix == EMPTY)
	    err = 1;

	// If we ran out of space, this string table will not work (too large)
	if(stFull(toReturn))
	    err = 1;

	if(err){
	    stDestroy(toReturn);
	    return NULL;
	}
	
	stInsert(toReturn, prefix, k);
    }

    // Make sure we're at end of file
    if(fgetc(f) != EOF)
	err = 1;

    if(err){
	stDestroy(toReturn);
	return NULL;
    }

    toReturn->origLen = toReturn->len;
    fclose(f);
    
    return toReturn;
}


// Increment a code's usage, meaning either code was read from stdin, or code 
// was put to stdout.If updateTotal is set, then it also increments the 
// StringTable's total usage.
void stUpdateUsage(StringTable st, int code, int updateTotal){
    
    if(updateTotal)
	st->usage++;

    // Was this code already assigned?
    if(code < st->len){
	st->array[code].usage++;

	// Recursively increment usage of dependencies
	int i = stGetTuple(st, code).prefix;
	while(i != EMPTY){
	    st->array[i].usage++;
	    i = stGetTuple(st, i).prefix;
	}
    }else{
	// Not assigned yet, may have to grow array.
	if(st->len == st->size && !stFull(st))
	    stIncreaseNumBits(st);
	
	if(code < st->size)
	   st->array[code].usage++;
    }
}

// Prune string table, i.e. replace it with a string table containing all
// one-char strings, all strings with use count >= used, and the
// "dependencies", i.e. all its nonempty prefix strings
void stPrune(StringTable *stptr, int used){
    StringTable oldST = *stptr;
    StringTable newST = stCreateAndInit((*stptr)->maxBits);
    
    for (int k = newST->len; k < oldST->len; k++){
	// Check if this tuple should be added to the new array
	if(oldST->array[k].usage >= used){
	    stEntry old = stGetTuple(oldST, k);

	    // What's the prefix of this tuple in the new String Table?
	    int newPrefix = - 1;

	    // Was it a null string?
	    if(old.prefix < 257){
		// Prefix is single char
		newPrefix = old.prefix;
	    }else{
		// Prefix is a string with dependencies
		stEntry oldsPrefix = stGetTuple(oldST, old.prefix);
		newPrefix = oldsPrefix.usage;
	    }

	    if(newPrefix == -1){
		fprintf(stderr, "LZW: Error adding <%d, %d> at %d in oldST while pruning\n", 
			old.prefix, old.k, k);
		exit(-1);
	    }

	    if(newPrefix == newST->len){
		stInsert(newST, newPrefix, old.k);
		exit(-1);
	    }

	    stInsert(newST, newPrefix, old.k);

	    // We don't need its usage field any more.
	    // Set it equal to its index in the new string.
	    oldST->array[k].usage = newST->len - 1;
	}
    }
    
    stDestroy(oldST);

    *stptr = newST;
    newST->origLen = newST->len;
}

// Free all memory associated with st
void stDestroy(StringTable st){
   free(st->array);
   free(st->hashArray);
   free(st);
}

