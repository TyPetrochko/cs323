// Far.h -- System header files for Far

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <errno.h>

// Write message to stderr using format FORMAT
#define WARN(format,...) fprintf (stderr, "Far: " format "\n", __VA_ARGS__)

// Write message to stderr using format FORMAT and exit.
#define DIE(format,...)  WARN(format,__VA_ARGS__), exit (EXIT_FAILURE)

// For logging files in archive
struct stack {
    char * fileName;
    struct stack * next;
};

struct stack * stackCreate();

void stackAddFile(struct stack * log, char * fileName);

int stackContains(struct stack * log, char * fileName);

int stackContainsChild(struct stack * log, char * fileName);

void stackDestroy(struct stack * log);

typedef struct stack * Stack;

// Strip leading /s in command line args
void stripLeadingSlashes(int argc, char *argv[]);

// Archive or update files based on command line args
int rcommand (int argc, char *argv[]);

// Print out files in archive
int tcommand (int argc, char *argv[]);

// Extract files from command line args
int xcommand (int argc, char *argv[]);

// Delete file and nested file from command line args
int dcommand(int argc, char * argv[]);

// Scan through archive and execute r, t, x, or d command
int scanArchive(int argc, char * argv[], int command);

// Add file or directory "fileName" to archive arkivFileDis if not already in log
int archiveFileOrDirectoryAndLog(char * fileName, struct stack * log, int arkivFileDist);

// Add file to archive
int archiveFileAndLog(char * fileName, struct stack * log, int arkivFileDist);

// Add a file's name and size to archive
int archiveFileSizeAndName(char * fileName, int arkiv, off_t fileSize);

// Move an already archived file from "fromArchive" to "toArchive"
int moveArchiveAndLog (char * fileName, struct stack * log, int fromArchive, int toArchive, off_t nbytes);

// Copy bytes from one file descriptor to another
int copyBytes(char * fileName, int fromFile, int toFile, off_t nbytes);

// Set up required directories for given path
int setupPath(char * path);

// If file not read-only, delete, else yield error message
int clearFile(char * fileName);

// Check if path is a directory
int isDir(char * path);

// Check if file exists
int fileOrDirExists(char * path);

// Extract a file from archive
int extractFileFromArchive(char * fileName, int arkiv, off_t nbytes);

// Check if a parentDir is a parent directory of fileName
int checkIfParentDirectory(char * parentDir, char * fileName);

