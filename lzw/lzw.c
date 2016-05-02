#include "lzw.h"
#include "/c/cs323/Hwk4/code.h"
#include "StringTable.h"

StringTable table;

int maxBits;
long prune;
char *opath;
char *ipath;


int main (int argc, char *argv[]){
    
    int flen = strlen(argv[0]);
    if (flen < 6){
	fprintf(stderr, "LZW: Expected [encode | decode], got %s\n", argv[0]);
    }
    
    // Make sure last six characters match "encode" or "decode"
    if(!strcmp(argv[0] + flen - 6, "encode")){
	// Try to set flags
	if(setEncodeFlags(argc, argv)){
	    encode(maxBits, opath, ipath, prune);
	}else{
	    return 0;
	}
    }else if(!strcmp(argv[0] + flen - 6, "decode")){
	if(setDecodeFlags(argc, argv)){
	    decode(opath);
	}
    }else{
	fprintf(stderr, "LZW: Expected [encode | decode], got %s\n", argv[0] + flen - 6);
    }

    return 0;
}

int setEncodeFlags(int argc, char *argv[]){
    // Num. params must be odd
    if(argc % 2 == 0){
	fprintf(stderr, "LZW: invalid num of args\n");
	return 0;
    }

    // Set defaults
    ipath = opath = NULL;
    maxBits = 12;
    prune = 0;
    
    for(int i = 1; i < argc; i += 2){
	if(!strcmp(argv[i], "-m")){
	    char *shouldBeSetToEmpty = NULL;
	   
	    errno = 0;
	    long result = strtoll(argv[i + 1], &shouldBeSetToEmpty, 10);

	    if(result > 0 && isdigit(argv[i + 1][0]) && !errno 
		    && !strcmp(shouldBeSetToEmpty, "")){
		maxBits = (int) result;
		if(maxBits < 9 || maxBits > 20){
		    maxBits = 12;
		}
	    }else{
		fprintf(stderr, "LZW: -m cannot accept %s\n", argv[i + 1]);
		return 0;
	    }
	    // Maxbits must be in range 9-20 inclusive
	}else if(!strcmp(argv[i], "-o")){
	    opath = argv[i + 1];
	}else if(!strcmp(argv[i], "-i")){
	    ipath = argv[i + 1];	    
	}else if(!strcmp(argv[i], "-p")){
	    char *shouldBeSetToEmpty = NULL;
	    
	    errno = 0;
	    long result = strtoll(argv[i + 1], &shouldBeSetToEmpty, 10);

	    if(result > 0 && isdigit(argv[i + 1][0]) && !errno 
		    && !strcmp(shouldBeSetToEmpty, "")){
		prune = result;
	    }else{
		fprintf(stderr, "LZW: -p cannot accept %s\n", argv[i + 1]);
		return 0;
	    }
	}else{
	    fprintf(stderr, "LZW: invalid argument: %s\n", argv[i]);
	    return 0;
	}
    }
    return 1;
}

int setDecodeFlags(int argc, char *argv[]){
    // Num. params must be odd
    if(argc % 2 == 0){
	fprintf(stderr, "LZW: invalid num of args\n");
	return 0;
    }
    // Set defaults
    opath = NULL;
    for(int i = 1; i < argc; i += 2){
	if(!strcmp(argv[i], "-o")){
	    opath = argv[i + 1];
	}else{
	    fprintf(stderr, "LZW: invalid argument: %s\n", argv[i]);
	    return 0;
	}
    }
    return 1;
}

// Encode from stdin to stdout. Return T/F on success/failure. 
// Do not print error on failure (yet). Prune = 0 represents -p flag not set.
int encode(int maxbits, char *opath, char *ipath, long prune){ 
    if(maxbits > 20){
	fprintf(stderr, "Programming error: did not check maxbits > 20");
	return 0;
    }

    // Recover string table from ipath, or init. a new one.
    if(ipath){
	table = stRecover(ipath, maxbits);
	if(table == NULL){
	    fprintf(stderr, "LZW: could not read/verify file %s\n", ipath);
	    exit(-1);
	}
    }else{
	table = stCreateAndInit(maxbits);
    }

    // Print header
    printf("%d:%ld:", maxbits, prune);

    if(ipath){
	printf("%zu:%s", strlen(ipath), ipath);
    }else{
	printf("0:");
    }

    int c = EMPTY;
    int k;
    while((k = getchar()) != EOF){
	int code = stGetCode(table, c, k);
	
	// Check if code exists
	if(code != -1){
	    c = code;
	}else{
	    putBits(stNumBits(table), c);
	    stUpdateUsage(table, c, 1);

	    // If table is full, we don't add or prune. Else, add the code.
	    if(stFull(table) && prune){
		stPrune(&table, prune);
		c = stGetCode(table, EMPTY, k);
		continue;
	    }
	    if(!stFull(table)){
		if(stCodeExists(table, c))
		    stInsert(table, c, k);
		
	    }

	    c = stGetCode(table, EMPTY, k);
	}
    }

    if (c != EMPTY){
	putBits(stNumBits(table), c);
	stUpdateUsage(table, c, 1);
    }

    flushBits();

    // Serialize string table if specified
    if(opath){
	int success = stSerialize(table, opath);
	if(!success){
	    stDestroy(table);
	    fprintf(stderr, "Could not write to %s\n", opath);
	    return 0;
	}
    }
    stDestroy(table);
    return 1;
}

// Just some code for our stack implementation. Might move this to a separate
// file if lzw.c becomes too cluttered.
struct stack {
    int len;
    int size;
    char *array;
};

typedef struct stack * Stack;

Stack stackInit(){
    Stack toReturn = malloc(sizeof(struct stack));
    toReturn->len = 0;
    toReturn->size = 4;
    toReturn->array = malloc(toReturn->size);
    return toReturn;
}

void stackPush(Stack s, char k){
    if (s->len == s->size){
	s->size *= 2;
	s->array = realloc(s->array, s->size);
    }

    s->array[s->len++] = k;
}

char stackPop(Stack s){
    return s->array[--(s->len)];
}

int stackEmpty(Stack s){
    return (s->len == 0);
}

void stackDestroy(Stack s){
    free(s->array);
    free(s);
}

// Decode from stdin to stdout.
int decode (char *opath){
    
    // First read in header
    int len;
    if(scanf("%d:%ld:%d:", &maxBits, &prune, &len) < 3 || maxBits< 9 
	    || maxBits> 20 || prune < 0 || len < 0){
	fprintf(stderr, "LZW: invalid header\n");
	return 0;
    }
    
    // Check if we need to read in a string table
    if(len){
	char ipath[len + 1];
	for(int i = 0; i < len; i ++){
	    ipath[i] = getchar();
	}
	ipath[len] = '\0';
	
	// Read in string table
	table = stRecover(ipath, maxBits);
	if(table == NULL){
	    fprintf(stderr, "LZW: could not read/verify file %s\n", ipath);
	    return 0;
	}
    }else{
	// Just start with a blank string table
	table = stCreateAndInit(maxBits);
    }
    
    
    Stack Kstack = stackInit();
    
    int c;	    // Code used to iteratively form output
    int oldC;	    // Remember last code read in 
    int newC;	    // New code read from stdin
    char finalK;    // Last K output

    oldC = EMPTY;

    while((newC = c = getBits(stNumBits(table))) != EOF){
	// 
	// Update its usage. 
	//
	// But, if it's full, then we're about to prune.
	// Encode is AHEAD of us, so it ALREADY pruned.
	// Therefore, this new code should not have an increased use count.
	// We'll re-update usage afterwards.
	//
	// Obviously, if we're not pruning, then this will throw off the counts
	// since it stops measuring counts once its full. But if we're not
	// pruning, why do we care?
	//
	//
	if(!stFull(table))
	    stUpdateUsage(table, c, 1);
	
	if(!stCodeExists(table, c)){
	    // If code is way too big, or it looks like a kwkwk case but oldC is 
	    // empty, then this code could not have been sent.
	    if(c > table->len || (c == table->len && oldC == EMPTY)){
		fprintf(stderr, "LZW: invalid code: %d\n", c);
		stackDestroy(Kstack);
		stDestroy(table);
		return 0;
	    }
	    
	    stackPush(Kstack, finalK);
	    c = oldC;
	    if(!stFull(table))
		stUpdateUsage(table, c, 0);
	}

	stEntry s;
	while((s = stGetTuple(table, c)).prefix != EMPTY){
	    stackPush(Kstack, s.k);
	    c = s.prefix;
	}

	finalK = s.k;
	putchar(finalK);

	while(!stackEmpty(Kstack)){
	    putchar(stackPop(Kstack));
	}

	// Decide whether or not to prune
	if(oldC != EMPTY){
	    if (stSyncedNumCodes(table) == table->size 
		    && table->numBits == table->maxBits && prune){
		
		// Try to sync the tables, and then prune!
		if(!stFull(table) && stCodeExists(table, oldC))
		    stInsert(table, oldC, finalK);
		stPrune(&table, prune);
		
		// Skip adding this code
		oldC = EMPTY;
		continue;
	    }
	    
	    // If table is full, we don't add or prune. Else, add the code.
	    if(!stFull(table)){
		if(stCodeExists(table, oldC))
		    stInsert(table, oldC, finalK);
	    }

	}

	oldC = newC;
    }

    stackDestroy(Kstack);

    // Serialize string table if specified
    if(opath){
	int success = stSerialize(table, opath);
	if(!success){
	    stDestroy(table);
	    fprintf(stderr, "Could not write to %s\n", opath);
	    return 0;
	}
    }
    stDestroy(table);
    return 1;
}
