// limit.c
//
//   runlimit #processes #descriptors command
//
// Limit #processes and #descriptors as specified and execute command

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>

int main (int argc, char *argv[])
{
    struct rlimit r;

    ++argv;
    if (getrlimit (RLIMIT_NPROC, &r) < 0)
	perror ("runlimit: getrlimit() failed:");
    r.rlim_cur = atoi (argv[0]);
    if (setrlimit (RLIMIT_NPROC, &r) < 0)
	perror ("runlimit: setrlimit() failed:");

    ++argv;
    if (getrlimit (RLIMIT_NOFILE, &r) < 0)
	perror ("runlimit: getrlimit() failed:");
    r.rlim_cur = atoi (argv[0]);
    if (setrlimit (RLIMIT_NOFILE, &r) < 0)
	perror ("runlimit: setrlimit() failed:");

    ++argv;
    execvp (argv[0], argv);
    perror ("runlimit: execvp() failed:");
}
