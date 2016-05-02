#include "process.h"

int process(CMD *tree){
    // Debug purposes
    // dumpTree(tree, 0);

    // Slay any zombies
    reapZombies();
    int status = 11; // Default
    if(tree->type == SIMPLE){
	status = simple_sub(tree, SIMPLE);
    }else if(tree->type == SEP_END){
	status = sep_end(tree);
    }else if(tree->type == SEP_AND){
	status = sep_and_or(tree, SEP_AND);
    }else if(tree->type == SEP_OR){
	status = sep_and_or(tree, SEP_OR);
    }else if(tree->type == SEP_BG){
	status = sep_bg(tree);
    }else if(tree->type == SUBCMD){
	status = simple_sub(tree, SUBCMD);
    }else if(tree->type == PIPE){
	status = pipe_cmd(tree);
    }

    // Assume status is fewer than 24 digits long
    char status_str[25];
    snprintf(status_str, 25, "%d", status);
    setenv("?", status_str, 1);
    return status;
}

// Background this tree and return its pid, or -1 if fail (check errno).
// Does not print error message, but DOES print backgrounded message.
pid_t background(CMD *tree){
    pid_t pid;
    if((pid = fork()) < 0){
	perror("Bsh");
	return -1;
    }else if (pid == 0){
	// In child
	exit(process(tree));
    }else{
	// In parent
	fprintf(stderr, "Backgrounded: %d\n", pid);
	return pid;
    }
}

// Helper function used to "plunge" through nested background commands.
// Root is only set to 1 the first time we call it, as in sep_bg();
// Returns 0 or whatever the last command processed returns.
int recursively_spawn_bgs(CMD *tree, int root){
    // If we're at a & sign, background the right half. If necessary, keep
    // recursing down to the left.
    if(tree->type == SEP_BG){
	// Do we keep recursing left?
	if(tree->left->type == SEP_BG || tree->left->type == SEP_END){
	    recursively_spawn_bgs(tree->left, 0);
	}else{
	    // We're done, just background left half!
	    background(tree->left);
	}
	
	if(tree->right != NULL){
	    if(root){
		return(process(tree->right));
	    }else{
		background(tree->right);
		return 0;
	    }
	}else{
	    return 0;
	}
    }else {
	// We have a semicolon (SEP END). Process left before backgrounding right.
	process(tree->left);
	background(tree->right);
	return 0;
    }
}

// Process this command as a sep_bg (background) command, i.e. "./Timer &"
int sep_bg(CMD *tree){
    return recursively_spawn_bgs(tree, 1);
}

// Process this tree as a sep_end (semicolon)
int sep_end(CMD *tree){
    int toReturn = - 1;
    if(tree->left != NULL){
	toReturn = process(tree->left);
    }
    if(tree->right != NULL){
	toReturn = process(tree->right);
    }
    return toReturn;
}

// Process this tree as a sep_and (&&) or a sep_or (||) depending on AND_OR value
int sep_and_or(CMD *tree, int AND_OR){
    int returnVal = process(tree->left);
    int success = (AND_OR == SEP_AND) ? returnVal == 0 : returnVal != 0;
    
    if(success){
	return process(tree->right);
    }else{
	return returnVal;
    }
}

// Set up any redirections according to tree.
// Return 0 if success, -1 if failure, issues error, 
// (and set errno).
int setup_redirections (CMD *tree){
    // Check if redirecting in
    if(tree->fromType == RED_IN){
	int in;
	if((in = open(tree->fromFile, O_RDONLY)) == -1){
	    fprintf(stderr, "Bsh: cannot read from file %s\n", tree->fromFile);
	    return -1;
	}
	dup2(in, STDIN_FILENO);
	close(in);
    }
    // Check if redirecting out (regular or append mode)
    if(tree->toType != NONE){
	int out, flags;
	mode_t mode;
	flags = O_WRONLY | O_CREAT;
	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
	if(tree->toType == RED_OUT_APP){
	    flags |= O_APPEND;
	}else{
	    flags |= O_TRUNC;
	}
	if((out = open(tree->toFile, flags, mode)) == - 1){
	    fprintf(stderr, "Bsh: cannot write to file %s\n", tree->toFile);
	    return -1;
	}
	dup2(out, STDOUT_FILENO);
	close(out);
    }
    return 0;
}

// Process this tree as a simple command, OR as a subcommand.
// To determine which, check value of SIMPLE_SUB with appropriate macros.
// Does print error message.
int simple_sub(CMD *tree, int SIMPLE_SUB){
    
    // Check for built - in command, i.e. cd/dirs/wait
    if(SIMPLE_SUB == SIMPLE){
	if(!strcmp(tree->argv[0], "cd") 
		|| !strcmp(tree->argv[0], "dirs") 
		|| !strcmp(tree->argv[0], "wait")){
	    return built_in_cmd(tree);
	}
    }


    pid_t pid;
    if((pid = fork()) < 0){
	perror("Bsh");
	// If fork fails, then we shouldn't be setting any variable
	return 0;
    }else if (pid == 0){
	// In child process
	
	// Set environment variables
	for(int n = 0; n < tree->nLocal; n++){
	    setenv(tree->locVar[n], tree->locVal[n], 1);
	}

	// Set up redirections
	if(setup_redirections(tree) == -1){
	    exit(errno);
	}
	
	// Execute simple or sub command
	if(SIMPLE_SUB == SIMPLE){
	    // Make sure to catch failure
	    if(execvp(tree->argv[0], tree->argv)){
		fprintf(stderr, "Bsh: could not execute %s\n", tree->argv[0]);
		exit(errno);
	    }
	}else {
	    // Execute subcommand, i.e. process left subtree
	    int retVal = process(tree->left);
	    exit(retVal);
	}
	// End child process
    }else{
       // In parent process
       signal(SIGINT, SIG_IGN);
       int status;
       waitpid(pid, &status, 0);
       signal(SIGINT, SIG_DFL);
       return STATUS(status);
    }

    // Control must not get here
    return 0;
}

// Process this tree as a built-in command, and return appropriate status.
// Returned statuses can be 0 (success), 1 (improper usage) or errno.
int built_in_cmd(CMD *tree){
    int is_cd_cmd   = !strcmp(tree->argv[0], "cd");
    int is_dirs_cmd = !strcmp(tree->argv[0], "dirs");
    int is_wait_cmd = !strcmp(tree->argv[0], "wait");

    if(is_cd_cmd && tree->argc > 2){
	fprintf(stderr, "Bsh: improper use of cd\n");
	return 1;
    }

    if((is_dirs_cmd || is_wait_cmd) && tree->argc > 1){
	fprintf(stderr, "Bsh: improper use of %s\n", tree->argv[0]);
	return 1;
    }

    // While a built-in command would never require a redirection in, it's
    // important that we try to set this up 
    if(tree->fromType == RED_IN){
	int in;
	if((in = open(tree->fromFile, O_RDONLY)) == -1){
	    fprintf(stderr, "Bsh: cannot read from file %s\n", tree->fromFile);
	    return errno;
	}
	close(in);
    }

    // The only command that can print to stdout is dirs, but it's imporant
    // to try anway (same reason as above).
    int new_std_out = STDOUT_FILENO;
    if(tree->toType != NONE){
	int out, flags;
	mode_t mode;
	flags = O_WRONLY | O_CREAT;
	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
	if(tree->toType == RED_OUT_APP){
	    flags |= O_APPEND;
	}else{
	    flags |= O_TRUNC;
	}
	if((out = open(tree->toFile, flags, mode)) == - 1){
	    fprintf(stderr, "Bsh: cannot write to file %s\n", tree->toFile);
	    return errno;
	}
	new_std_out = out;
    }

    int retval;
    if(!strcmp(tree->argv[0], "cd")){
	
	// Try to change directory
	char *dest;
	if(tree->argc == 1){
	    dest = getenv("HOME");
	}else{
	    dest = tree->argv[1];
	}

	// We may have set environment variables to give illusion of other paths
        for(int n = 0; n < tree->nLocal; n++){
	    if(!strcmp(tree->locVar[n], "HOME") && tree->argc == 1){
		dest = tree->locVal[n]; 
	    }
	}
                                                                                     
	retval = chdir(dest);
	// Now check if it worked
	if(retval == - 1){
	    perror("Bsh");
	    retval =  errno;
	}else{
	    retval =  0;
	}
    }else if (!strcmp(tree->argv[0], "dirs")){
	// It's a dirs command
	char buf[PATH_MAX + 1];
	char * cwd = getcwd(buf, PATH_MAX + 1);
	if(cwd == NULL){
	    perror("Bsh");
	    retval = errno;
	}else{
	    dprintf(new_std_out, "%s\n", cwd);
	    retval =  0;
	}
    }else if (!strcmp(tree->argv[0], "wait")){
	wait_for_procs();
	retval = 0;
    }

    if(new_std_out != STDOUT_FILENO){
	close(new_std_out);
    }
    return retval;
}

int pipe_cmd(CMD *tree){
    // First we pipe and make two file descriptors
    // 
    // Then we fork to make left process
    // the left process must set his stdout to the write end of 
    // the pipe, i.e. fd[1]. He then closes his write end AND 
    // read end of the pipe since he has two copies of it.
    //
    // Then we fork to make right process. He makes his stdin
    // the read end of the pipe, then closes both ends of the pipe.
    //
    // Then the original parent process closes both ends of the pipe,
    // and both child processes process their respective commands.

    int fd[2];
    pid_t pid_left, pid_right;

    // Create left subshell
    if(pipe(fd) || (pid_left = fork()) < 0){
	perror("Bsh");
	return errno;
    }else if(pid_left == 0){
	// Child process
	// i.e. fd[1] != 1
	close(fd[0]);
	dup2(fd[1], STDOUT_FILENO);
	close(fd[1]);

	exit(process(tree->left));
    }else{
	// Parent process
	close(fd[1]);
    }

    // Create right subshell
    if((pid_right = fork()) < 0){
	perror("Bsh");
	return errno;
    }else if(pid_right == 0){
	// Child process
	dup2(fd[0], STDIN_FILENO);
	close(fd[0]);

	exit(process(tree->right));
    }else{
	close(fd[0]);

	int lstatus, rstatus;
	signal(SIGINT, SIG_IGN);
	waitpid(pid_left, &lstatus, 0);
	waitpid(pid_right, &rstatus, 0);
	signal(SIGINT, SIG_DFL);

	lstatus = STATUS(lstatus);
	rstatus = STATUS(rstatus);

	return (rstatus ? rstatus : lstatus);
    }
}

// Wait for all children processes to die
int wait_for_procs(){
    int status;
    pid_t pid;

    signal(SIGINT, SIG_IGN);
    while((pid = wait(&status)) > 0){
	// A process finished
	fprintf(stderr, "Completed: %d (0)\n", pid);
    }
    signal(SIGINT, SIG_DFL);
    return 0;
}

// Smite thy unholy rotten Servants to the Musty Earth from whence They came
void reapZombies(){
    pid_t pid;
    int status;

    while(((pid = waitpid(-1, &status, WNOHANG)) > 0)){
	// Killed a zombie
	fprintf(stderr, "Completed: %d (0)\n", pid);
    }
}

