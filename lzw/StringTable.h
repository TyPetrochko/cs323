#include <string.h>
#include <stdlib.h>

#define EMPTY (0)	    // Represent empty string
#define MIN_BITS (9)	    // Min number of bits to represent a code
#define MAX_BITS (20)	    // Max number of bits to represent a code
#define FIRST_ENTRY (257)   // Index of first non-char string in table

typedef struct stEntry {
    char k;	// Extension char
    int prefix;	// Prefix code
    int code;	// Code - also index in strtable

    int usage;	// Debug: how many times is this entry used?

    // Also must function as a linked-list node
    struct stEntry *next;
} stEntry;

struct stringTable {
    int origLen;
    int size;
    int len;
    int maxBits;
    int numBits;
    unsigned long long usage;

    struct stEntry * array;	    // For indexing by code
    struct stEntry ** hashArray;    // For indexing by (prefix, k)
};

typedef struct stringTable * StringTable;

// Check if 'code' is a known code in the string table? Return T/F
int stCodeExists(StringTable st, int code);

// Insert a tuple into String Table
void stInsert (StringTable st, int prefix, char k);

// Get num of bits added to string table in encode or decode
unsigned long long stSyncedNumCodes(StringTable st);

// Calculate number of bits necessary to represent a code in String Table.
int stNumBits(StringTable st);

// Get a (prefix, k, code) tuple, along with some extra info.
// Make sure to check code exists first!
stEntry stGetTuple (StringTable st, int code);

// Get the code for a (prefix, k) pair. Return -1 if doesn't exist.
int stGetCode (StringTable st, int prefix, char k);

// Create, init, and return a malloc'ed String Table.
// Note: 9 <= maxBits <= 20 must hold, or return NULL.
StringTable stCreateAndInit(int maxBits);

// Save string table to 'path'. Return T if success, F if failure.
int stSerialize(StringTable st, char * path);

// Recover a string table from char * path. Return NULL if failure.
StringTable stRecover(char * path, int maxBits);

// Is string table full? Return T/F
int stFull(StringTable st);

// Prune string table, i.e. replace it with a string table containing all
// one-char strings, all strings with use count >= used, and the
// "dependencies", i.e. all its nonempty prefix strings
void stPrune(StringTable *stptr, int used);

// Debug - print the string table to stdout
void stPrint(StringTable st, char *path);

// Either code was read from stdin, or code was put to stdout.
// If updateTotal is set, then it also increments the StringTable's total 
// usage.
void stUpdateUsage(StringTable st, int code, int updateTotal);

// Free all memory associated with st.
void stDestroy(StringTable st);

