#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

int main(){
    char *a = "100a";
    char *b = "100";
    char *c = NULL;
    char *d = NULL;

    long x = strtol(a, &c, 10);
    long y = strtol(b, &d, 10);

    printf("LONG_MAX = %ld\n", LONG_MAX);

    long one = 123456789012345;

    if(one > LONG_MAX){
	printf("%ld > %ld\n", one, LONG_MAX);
    }else{
	printf("%ld <= %ld\n", one, LONG_MAX);
    }

    return 0;
}
