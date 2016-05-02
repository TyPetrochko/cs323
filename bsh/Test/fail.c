// fail.c
//
//   fail [-open|-pipe|-fork|-utime] command
//
// Use ptrace to cause specified system call to fail

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <errno.h>

// Write message to stderr using FORMAT that may reference VALUE
#define WARN(format,...) fprintf (stderr, "fail: " format "\n", __VA_ARGS__)

// Print message to stderr using FORMAT that may reference VALUE, then exit
#define DIE(format,...)  fprintf (stderr, "fail: " format "\n", __VA_ARGS__),\
			 exit (EXIT_FAILURE)

// Call perror(STRING), then exit
#define PERROR(string)   perror ("fail " string), exit (EXIT_FAILURE)

// Print message only if DebugLevel is sufficiently high
int DebugLevel = -1;
#define DEBUG(level,msg,...)                                    \
    do {                                                        \
	if (level <= DebugLevel)                                \
	    fprintf(stderr, "fail: " msg "\n", __VA_ARGS__);    \
    } while (0)


int main (int argc, char *argv[])
{
    pid_t pid;
    int status, nprocs;
    struct proc {int pid; int error; int syscall;} procs[10000];
    struct user_regs_struct regs;
     
    char *syscall[10000];
    #include "syscall.h"
     
    // Create child to execute command and trace it
    if ((pid = fork()) < 0) {
	PERROR ("fork() failed");
     
    } else if (pid == 0) {
	if (ptrace (PTRACE_TRACEME, 0, NULL, NULL) < 0)
	    PERROR ("ptrace(TRACEME)");
	DEBUG (0, "%d: child running %s", getpid(), argv[2]);
	execvp (argv[2], &argv[2]);
	PERROR ("execvp() failed");
    }
     
    // Wait for child to begin and trace grandchildren as well
    pid = waitpid (pid, &status, 0);
    if (WIFEXITED (status))
	DIE ("%d: died with status = %d", pid, WEXITSTATUS (status));
    else if (WIFSIGNALED (status))
	DIE ("%d: died with signal = %d", pid, WTERMSIG (status));
     
    if (ptrace (PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_TRACEFORK |
		PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE) < 0)
	PERROR ("ptrace (SETOPTIONS)");
    if (ptrace (PTRACE_SYSCALL, pid, NULL, NULL) < 0)
	PERROR ("ptrace (SYSCALL)");
     
    DEBUG (0, "%d: parent suppressing %s()", getpid(), argv[1]);
     
    // Process all system calls
    nprocs = 0;
    while ((pid = wait (&status)) > 0) {
	// Descendant called exit() or terminated by signal?
	if (WIFEXITED (status)) {
	    DEBUG (0, "%d: exit status = %d", pid, WEXITSTATUS(status));
	    continue;
	} else if (WIFSIGNALED (status)) {
	    DEBUG (0, "%d: died by signal = %d", pid, WTERMSIG(status));
	    continue;
	}
     
	// Get system call
	if (ptrace (PTRACE_GETREGS, pid, NULL, &regs) < 0)
	    PERROR ("ptrace (GETREGS)");
	DEBUG (2, "%d: stopped = %d, syscall = %lld / %s",
	       pid, WSTOPSIG(status), regs.orig_rax, syscall[regs.orig_rax]);
     
	if (WSTOPSIG(status) == SIGTRAP) {
	    procs[nprocs].pid = pid;
	    procs[nprocs].syscall = regs.orig_rax;

	    // Is it a modified system call?
	    int i;
	    for (i = 0; i < nprocs; i++) {
		if (pid == procs[i].pid)
		    break;
	    }
     
	    if (i < nprocs) {
		DEBUG (1, "%d: Changing return value of %s() to %d",
		       pid, syscall[procs[i].syscall], procs[i].error);
		regs.rax = -procs[i].error;
		if (ptrace (PTRACE_SETREGS, pid, NULL, &regs) < 0)
		    PERROR ("ptrace (SETREGS)");
		procs[i] = procs[--nprocs];
     
	    } else if (!strcmp (argv[1], "-open") &&
		  regs.orig_rax == SYS_open) {
		DEBUG (1, "%d: Replacing open() by getpid()", pid);
		regs.orig_rax = SYS_getpid;
		if (ptrace (PTRACE_SETREGS, pid, NULL, &regs) < 0)
		    PERROR ("ptrace (SETREGS)");
		procs[nprocs++].error = EMFILE;
     
	    } else if (!strcmp (argv[1], "-pipe") &&
		  regs.orig_rax == SYS_pipe) {
		DEBUG (1, "%d: Replacing pipe() by getpid()", pid);
		regs.orig_rax = SYS_getpid;
		if (ptrace (PTRACE_SETREGS, pid, NULL, &regs) < 0)
		    PERROR ("ptrace (SETREGS)");
		procs[nprocs++].error = EMFILE;
			    
	    } else if (!strcmp (argv[1], "-fork") &&
		  (regs.orig_rax == SYS_fork || regs.orig_rax == SYS_clone)) {
		DEBUG (1, "%d: Replacing fork() by getpid()", pid);
		regs.orig_rax = SYS_getpid;
		if (ptrace (PTRACE_SETREGS, pid, NULL, &regs) < 0)
		    PERROR ("ptrace (SETREGS)");
		procs[nprocs++].error = EAGAIN;

	    } else if (!strcmp (argv[1], "-utime") &&
		  (regs.orig_rax == SYS_utime || regs.orig_rax == SYS_utimes)) {
		DEBUG (1, "%d: Replacing utime() by getpid()", pid);
		regs.orig_rax = SYS_getpid;
		if (ptrace (PTRACE_SETREGS, pid, NULL, &regs) < 0)
		    PERROR ("ptrace (SETREGS)");
		procs[nprocs++].error = EAGAIN;
	    }
	}
     
	if (ptrace (PTRACE_SYSCALL, pid, NULL, NULL) < 0)
	    PERROR ("ptrace (SYSCALL)");
    }
     
    return EXIT_SUCCESS;
}
