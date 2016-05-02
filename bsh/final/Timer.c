#include <stdio.h>
#include <unistd.h>

int main (int argc, char *argv[]){
    if (argc > 2) return -1;
    
    int timer = 3;
    char * msg;

    if(argc > 1){
	msg = argv[1];
    }else{
	msg = "and counting";
    }

    while(timer > 0){
	printf("%d %s\n", timer, msg);
	sleep(2);
	timer --;
    }
    return 0;
}
