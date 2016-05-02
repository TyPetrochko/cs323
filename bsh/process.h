#include "/c/cs323/Hwk5/process-stub.h"
#include <unistd.h>
#define _GNU_SOURCE

#define STATUS(x) (WIFEXITED(x) ? WEXITSTATUS(x) : 128+WTERMSIG(x))

// Process this tree in the background.
// Returns process pid or -1 if failure (check errno).
// Does not print error message, but DOES print bacgkrounded
// message.
pid_t background(CMD *tree);


// Helper function to set up all redirections accoring to tree.
// If success, return 0. If failure, return -1, set errno, and print 
// error to stderr.
int setup_redirections(CMD *tree);


// Process this tree as a simple command
int simple_sub(CMD *tree, int SIMPLE_SUB);


// Process this tree as a sep_end
int sep_end(CMD *tree);


// Process this tree as sep_and
int sep_and_or(CMD *tree, int AND_OR);


// Process this tree as sep_bg
int sep_bg(CMD *tree);


// Process this tree as a built-in command.
// Return appropriate status, i.e. 0 for successful,
// 1 for bad syntax, errno for IO failure.
int built_in_cmd(CMD *tree);


// Process this tree as a pipe command
int pipe_cmd(CMD *tree);


// Wait for all children to die. Return 0 always.
int wait_for_procs();


// Reap all zombies
void reapZombies();

