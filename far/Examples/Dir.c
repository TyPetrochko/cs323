// Dir.c                                         Stan Eisenstat (10/19/07)
//
// An example program that uses opendir(), readdir(), and closedir()
// to  list the files in the current working directory and their types

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <fnmatch.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>


// Write message to stderr using format FORMAT
#define WARN(format,...) fprintf (stderr, "Dir: " format "\n", __VA_ARGS__)

// Write message to stderr using format FORMAT and exit.
#define DIE(format,...)  WARN(format,__VA_ARGS__), exit (EXIT_FAILURE)


int main (int argc, char *argv[])
{
    char *name = ".";
    DIR *dp;
    struct dirent *entry;
    struct stat buf;

    if ((dp = opendir (name)) == NULL)                // Open the directory
	DIE ("cannot opendir(%s)\n", name);

    while ((entry = readdir (dp))) {                  // Read & print filenames
	if (lstat (entry->d_name, &buf) < 0)
	    DIE ("lstat(%s) failed\n", entry->d_name);
	if (S_ISDIR(buf.st_mode))
	    printf ("%s/\n", entry->d_name);          //  Add / to directory
	else if (S_ISLNK(buf.st_mode))
	    printf ("%s@\n", entry->d_name);          //  Add @ to symbolic link
	else
	    printf ("%s\n", entry->d_name);
    }

    closedir (dp);                                    // Close the directory

    return EXIT_SUCCESS;
}
