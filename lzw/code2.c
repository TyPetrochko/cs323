// code2.c                                        Stan Eisenstat (11/13/14)
//
// Stage 2 implementation of putBits/getBits described in code.h


#include <stdio.h>
#include "/c/cs323/Hwk4/code.h"


// Write code (#bits = nBits) to standard output
void putBits (int nBits, int code)
{
    if (0 <= code && code < (1 << nBits))
	printf ("%d:%d\n", nBits, code);        // code representable
    else {
	fprintf (stderr, "nBits = %d, maxCode = %d, code = %d\n",
			  nBits,      (1<<nBits)-1, code        );
    }
}


// Flush remaining bits to standard output
void flushBits (void)
{
}


// Return next code (#bits = nBits) from standard input (EOF on end-of-file).
// Print error message and return EOF if #bits is not as expected.

int getBits (int nBits)
{
    int code, nRcvd;

    if (scanf ("%d:%d\n", &nRcvd, &code) < 2)   // Demand #bits & code or
	return EOF;                             //  assume no more codes
    else if (nRcvd == nBits
	  && 0 <= code && code < (1 << nBits))
	return code;                            // #bits as expected
    else {
	fprintf (stderr, "nBits = %d, nRcvd = %d, code = %d\n",
			  nBits,      nRcvd,      code        );
	return EOF;
    }
}
