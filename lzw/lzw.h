// lzw.h                                          Stan Eisenstat (10/19/15)
//
// Header files and macros for encode and decode

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "/c/cs323/Hwk4/code.h"

// Write message to stderr using format FORMAT
#define WARN(format,...) fprintf (stderr, "LZW: " format "\n", __VA_ARGS__)

// Write message to stderr using format FORMAT and exit.
#define DIE(format,...)  WARN(format,__VA_ARGS__), exit (EXIT_FAILURE)

// Encode from stdin. Return T/F on success/failure.
// Do not print error on failure (yet). Prune = 0 represents -p not set.
// NULL paths represent -i or -o flag not set.
int encode(int maxbits, char *opath, char *ipath, long prune);

// Decode from stdin. Return T/F on success/failure.
// Do not print error statements (yet). NULL path represents -o not set.
int decode(char *opath);

// Set values for global variables maxBits, prune, opath, and ipath
int setEncodeFlags(int argc, char *argv[]);

// Set values for opath global var
int setDecodeFlags(int argc, char *argv[]);

