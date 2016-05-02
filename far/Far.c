// Far.c 
// Written by Tyler Petrochko for Yale class CPSC 323, this program is an 
// implementation of the Far archive manager.

#include "Far.h"
// Constant for each command
#define R 0
#define D 1
#define X 2
#define T 3
#define WRITE_BUFFER_SIZE 1024
#define ERR_STATUS_LENGTH 100



// Describing error task
char errDesc[ERR_STATUS_LENGTH];

int main (int argc, char *argv[]){
    
    // Normalize names from input
    stripLeadingSlashes(argc, argv);

    int command = -1;

    if (argc < 3){
	fprintf(stderr, "Far: Far r|x|d|t archive [filename]*\n");
	return -1;
    } else if (!strcmp("r", argv[1])){
	command = R;
    } else if (!strcmp("t", argv[1])){
	command = T;
    } else if (!strcmp("x", argv[1])){
	command = X;
    } else if(!strcmp("d", argv[1])){
	command = D;
    }else {
	fprintf(stderr, "Far: Far r|x|d|t archive [filename]*\n");
	return -1;
    }

    if(scanArchive(argc, argv, command) == -1){
	fprintf(stderr, "Error %s: %s\n", errDesc, strerror(errno));
    }

    return 0;
}

// Remove leading /s from each command line 'name'.
void stripLeadingSlashes(int argc, char * argv[]){
    for(int i = 3; i < argc; i ++){
	// Start at last char and replace all /s with null chars
	int currentChar = strlen(argv[i]) - 1;

	// Make sure we're not excluding '/' directory case
	while(currentChar > 0 && argv[i][currentChar] == '/'){
	    argv[i][currentChar] = '\0';
	    currentChar--;
	}
    }
}

// Return a pointer to a created stack.
struct stack * stackCreate(){
    struct stack * toReturn = malloc(sizeof(struct stack));
    toReturn->fileName = malloc(1);
    toReturn->fileName[0] = '\0';
    toReturn->next = NULL;
    return toReturn;
}

// Is this stack empty? Return t/f value.
int stackIsEmpty(struct stack * log){
   return (log->next == NULL); 
}

// Add fileName to stack.
void stackAddFile(struct stack * log, char * fileName){
    // New element to be inserted
    struct stack * toAdd = malloc(sizeof(struct stack));

    // Make sure string will stay around
    char *fileNameBuffer = malloc(strlen(fileName) + 1);
    strcpy(fileNameBuffer, fileName);
    
    // Perform the 'switch'
    toAdd->fileName = log->fileName;
    toAdd->next = log->next;
    log->fileName = fileNameBuffer;
    log->next = toAdd;
}

// Does this stack contain fileName? Returns t/f value.
int stackContains(struct stack * log, char * fileName){
    // Iterator element
    struct stack * iterator = log;
    while (iterator != NULL){
	// If this stack element contains the filename, return true
	if (iterator->fileName != '\0' 
		&& !strcmp(iterator->fileName, fileName)){
	    return 1;
	}
	iterator = iterator->next;
    }
    // log does not contain fileName
    return 0;
}

// Does this stack contain a child of fileName? Assume it's a directory.
// Returns t/f value.
int stackContainsChild(struct stack * log, char * fileName){
    // We will test if it exists as a directory, too
    char dirName[PATH_MAX];
    strcpy(dirName, fileName);
    if(dirName[strlen(fileName) - 1] != '/') strcat(dirName, "/");

    // Iterator element
    struct stack * iterator = log;
    while (iterator != NULL){
	// If this stack element contains the filename, return true
	if (iterator->fileName != '\0' 
		&& ((!strcmp(iterator->fileName, fileName) 
		    || !strcmp(iterator->fileName, dirName))
		    || checkIfParentDirectory(fileName, iterator->fileName))) {
	    return 1;
	}
	iterator = iterator->next;
    }
    // log does not contain fileName
    return 0;
}

// Free all memory for a given stack.
void stackDestroy(struct stack * log){
    // Iterator element, just like in stackContains()
    struct stack * iterator = log;
    while (iterator != NULL) {
	// Save pointer to stack element to free
	struct stack * toFree = iterator;
	iterator = iterator->next;
	free(toFree->fileName);
	free(toFree);
    }
}

// Scan through an archive and perform necessary commands based on 
// command line input. Return 0 if successful, -1 if failure.
int scanArchive(int argc, char *argv[], int command){
    // Get archive name from command line
    char * arkName = argv[2];
    int arkiv;

    // Open existing archive
    if(!strcmp(arkName, "-")){
	// If archive name is '-' then read from std_in
	arkiv = STDIN_FILENO;
    } else if (command == R && (arkiv = open(arkName, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1){
	snprintf(errDesc, ERR_STATUS_LENGTH, "opening/creating initial archive %s", arkName);
	return -1;
    }else if ((arkiv = open(arkName, O_RDONLY, S_IRUSR | S_IWUSR)) == -1){
	snprintf(errDesc, ERR_STATUS_LENGTH, "opening/creating initial archive %s", arkName);
	return -1;
    }
    
    /* Make new archive (temporary) to copy to
     * ... if it succeeds, we will keep temp archive
     * else we will unlink it. */
    int newArkiv;
    char tmpName [PATH_MAX];
    memset(tmpName, 0, PATH_MAX);
    // For D and R command, make a backup file
    if (command == D || command == R){
	if(!strcmp(arkName, "-")){
	    newArkiv = STDOUT_FILENO;
	}else{
	    snprintf(tmpName, PATH_MAX, "%s.bak", arkName);
	    if ((newArkiv = open(tmpName, 
			    O_WRONLY | O_CREAT | O_EXCL, 
			    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) == -1){
		snprintf(errDesc, ERR_STATUS_LENGTH, 
			"making new tmp archive with name %s", tmpName);
		return -1;
	    }
	}
    }
    
    // Use stack to log added files
    Stack filesAdded = stackCreate();

    // Variable to store filesize of current archive
    off_t fileSize;

    // Read through archive 'arkiv'
    // For each file, filesize is stored, then file's name, terminated by null char
scanloop:
    while(read(arkiv, &fileSize, sizeof(off_t)) > 0){
	
	// Read into nameBuffer until null char detected in archive
	char nameBuffer[PATH_MAX];
	int posInNameBuff = 0;
	while(read(arkiv, nameBuffer + posInNameBuff, 1) == 1 
		&& nameBuffer[posInNameBuff++] != '\0');

	// Do we have a valid file?
	if(strlen(nameBuffer) < 1){
	    // Abort mission. Roll back.
	    snprintf(errDesc, ERR_STATUS_LENGTH, 
		    "reading file from archive %s", arkName);
	    stackDestroy(filesAdded);
	    if(arkiv != STDIN_FILENO) 
		unlink(tmpName);
	    return -1;
	}

	// Should we extract the file at the end of this loop?
	int shouldExtract = (command == X && argc == 3);

	if (command == T){
	    printf("%8d %s\n", (int) fileSize, nameBuffer);
	}else{
	// Is this file in command line args?
	    for(int i = 3; i < argc; i++){
		// If 'zoo' is in command line, also check for 'zoo/'
		char asDirectoryName[PATH_MAX];
		strcpy(asDirectoryName, argv[i]);
		if(asDirectoryName[strlen(asDirectoryName) - 1] != '/') strcat(asDirectoryName, "/");

		if((!strcmp(nameBuffer, argv[i])
			|| !strcmp(nameBuffer, asDirectoryName)
			|| checkIfParentDirectory(asDirectoryName, nameBuffer))){
		    // It IS listed in command line args, or some derivative of it is
		    
		    if(command == R){
			// Avoid updating a file with directory and vice versa
			if((!isDir(argv[i]) && nameBuffer[strlen(nameBuffer) - 1] == '/') 
				|| (isDir(argv[i]) && nameBuffer[strlen(nameBuffer) - 1] != '/')
				|| (!isDir(asDirectoryName) 
				    && checkIfParentDirectory(asDirectoryName, nameBuffer))){
			    continue;
			}

			// Replace file in new archive
			if(!fileOrDirExists(argv[i])){
			    if(moveArchiveAndLog(nameBuffer, filesAdded, arkiv, newArkiv, fileSize) == -1){
				// Could not copy this archived file to new archive. Rollback.
				if(arkiv != STDIN_FILENO) unlink(tmpName);
				return -1;
			    }
			    goto scanloop;
			    break;
			}
			// Make sure we log it with the right name
			char * properName = (!isDir(argv[i])) ? argv[i] : asDirectoryName;
			if(archiveFileOrDirectoryAndLog(properName, filesAdded, newArkiv) == -1){
			    // Couldn't log this file. Issue error and move on.
			    fprintf(stderr, "Far: could not add %s\n", properName);
			    if(moveArchiveAndLog(nameBuffer, filesAdded, arkiv, newArkiv, fileSize) == -1){
				// Could not copy this archived file to new archive. Rollback.
				if(arkiv != STDIN_FILENO) unlink(tmpName);
				return -1;
			    }
			    goto scanloop;
			}else {
			    if(lseek(arkiv, fileSize, SEEK_CUR) == -1){
				// Couldn't skip ahead in file. Rollback.
				if(arkiv != STDIN_FILENO) unlink(tmpName);
				stackDestroy(filesAdded);
				return -1;
			    }
			    goto scanloop;
			}
		    }else if (command == D){
			// Skip ahead to next file
			if(lseek(arkiv, fileSize, SEEK_CUR) == -1){
			    return -1;
			}else {
			    stackAddFile(filesAdded, argv[i]);
			    goto scanloop;
			}
		    }else if (command == X){
			// Extract file from archive after we exit
			shouldExtract = 1;
			break;
		    }
		}
	    }

	}

	// Should we extract?
	if(shouldExtract){
	    if(extractFileFromArchive(nameBuffer, arkiv, fileSize) == -1){
		// Couldn't extract. Skip ahead to next file.
		fprintf(stderr, "Far: cannot extract %s\n", nameBuffer);
		if(lseek(arkiv, fileSize, SEEK_CUR) == -1){
		    // Fatal error
		    if(arkiv != STDIN_FILENO) 
			unlink(tmpName);
		    stackDestroy(filesAdded);
		    return -1;
		}else{
		    goto scanloop;
		}
	    }else{
		// Successfully extracted file
		stackAddFile(filesAdded, nameBuffer);
		goto scanloop;
	    }
	}
	
	if(command == T || command == X){
	    if(lseek(arkiv, fileSize, SEEK_CUR) == -1){
		// Fatal error
		if(arkiv != STDIN_FILENO) 
		    unlink(tmpName);
		stackDestroy(filesAdded);
		return -1;
	    }else{
		// Move to next file
		goto scanloop;
	    }
	}else if(command == R || command == D){
	    // Move this file to new archive
	    if(moveArchiveAndLog(nameBuffer, filesAdded, arkiv, newArkiv, fileSize) == -1){
		// Could not copy this archived file to new archive. Rollback.
		if(arkiv != STDIN_FILENO) unlink(tmpName);
		return -1;
	    }
	}
    }

    if(command == R){
	// Start adding command line args
	for(int i = 3; i < argc; i++){
	    if(!fileOrDirExists(argv[i])){
		fprintf(stderr, "Far: %s does not exist\n", argv[i]);
		continue;
	    }else {
		char asDirectoryName[PATH_MAX];
		strcpy(asDirectoryName, argv[i]);
		if(asDirectoryName[strlen(asDirectoryName) - 1] != '/') strcat(asDirectoryName, "/");
		char * properName = (!isDir(argv[i])) ? argv[i] : asDirectoryName; 
		
		if(archiveFileOrDirectoryAndLog(properName, filesAdded, newArkiv) == -1){
		    // Couldn't archive
		    fprintf(stderr, "Far: could not archive %s\n", properName);
		    continue;
		}
	    }
	}
    }

    if(command == X || command == D){
	for (int i = 3; i < argc; i++){
	    if(!stackContainsChild(filesAdded, argv[i])){
		fprintf(stderr, "Far: cannot find %s\n", argv[i]);
	    }
	}
    }
    // Done adding to archive. Switch old archive and temp archive
    stackDestroy(filesAdded);
    if(arkiv != STDIN_FILENO && close(arkiv) == -1){
	// Can't close original archive.
	snprintf(errDesc, ERR_STATUS_LENGTH, "closing old archive file %s", arkName);
	return -1;
    }

    // Close additional files and switch for r and d commands
    if(command == R || command == D){
	if(arkiv != STDIN_FILENO && unlink(arkName) == -1){
	snprintf(errDesc, ERR_STATUS_LENGTH, 
		"unlinking original archive %s", arkName);
	}
	if(arkiv != STDIN_FILENO && rename(tmpName, arkName) == -1){
	    // Can't make tmpfile permanent. Rollback.
	    snprintf(errDesc, ERR_STATUS_LENGTH, 
		    "second step switching archives, renaming %s to %s", tmpName, arkName);
	    unlink(tmpName);
	    return -1;
	}
	if(arkiv != STDIN_FILENO && close(newArkiv) == -1){
	    // Can't close original archive.
	    snprintf(errDesc, ERR_STATUS_LENGTH, "closing archive file %s", tmpName);
	    return -1;
	}
    }
    return 0;
}

// If file or directory hasn't been logged already, add it to archive and log 
// it. Return 0 if sucess, -1 if failure.
int archiveFileOrDirectoryAndLog(char * fileName, struct stack * log,
	int arkivFileDis){


    // Did we already add 'fileName' to archive? If so, don't add it
    if(stackContains(log, fileName)) return 0;


    // Find some information about fileName
    struct stat statBuf;
    if (lstat(fileName, &statBuf) == -1){
	// File does not exist, but not a fatal error. Log as to not repeat.
	snprintf(errDesc, ERR_STATUS_LENGTH, "running lstat on %s", fileName);
	return -1;
    }
    
    // Will this create a feedback loop?
    if(arkivFileDis != STDOUT_FILENO){
	struct stat arkivStatBuf;
	if(fstat(arkivFileDis, &arkivStatBuf) == -1) {
	    snprintf(errDesc, ERR_STATUS_LENGTH, "running fstat on new archive");
	    return -1;
	}
	if(statBuf.st_ino == arkivStatBuf.st_ino){
	    // They refer to same file
	    off_t size = 0;
	    if(archiveFileSizeAndName(fileName, arkivFileDis, size) == -1){
		fprintf(stderr, "Far: could not archive %s\n", fileName);
	    }
	    return 0;
	}
    }

    if(S_ISREG(statBuf.st_mode)){
	// 'fileName' refers to a regular file
	if(archiveFileAndLog(fileName, log, arkivFileDis) == -1){
	    return -1;
	}
    }else if (S_ISDIR(statBuf.st_mode)){
	// Add directory as a normal file, make sure it ends in '/'
	char dirName[PATH_MAX];
	strcpy(dirName, fileName);
	if(dirName[strlen(fileName) - 1] != '/') strcat(dirName, "/");

	// Filesize of folder is zero
	off_t zero = 0;
	if(archiveFileSizeAndName(dirName, arkivFileDis, zero) == -1){
	    return -1;
	}
	stackAddFile(log, dirName);


	// Add files from this directory recursively
	DIR *dir;
	struct dirent *entry;
	
	if ((dir = opendir(dirName)) == NULL){
	    snprintf(errDesc, ERR_STATUS_LENGTH, "opening directory %s", dirName);
	    return -1;
	}

	while((entry = readdir(dir)) != NULL){
	    // Make sure file "file" in directory "dir" is "dir/file"
	    char entFullName[PATH_MAX];
	    snprintf(entFullName, PATH_MAX, "%s%s", dirName, entry->d_name);
	    // Make sure we're not on . or .. directories
	    if(strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")){
		if (archiveFileOrDirectoryAndLog(entFullName, log, arkivFileDis) == -1){
		    fprintf(stderr, "Far: could not add %s\n", entFullName);
		}
	    } 
	}
    }
    
    return 0;
}

// Only add the fileName and fileSize to archive. Don't worry about logging.
// Return 0 if successful, -1 if failure.
int archiveFileSizeAndName(char * fileName, int arkiv, off_t fileSize){
    // Write file size to archive first
    if(write(arkiv, &fileSize, sizeof(off_t)) != sizeof(off_t)){
	snprintf(errDesc, ERR_STATUS_LENGTH, 
		"writing to archive filesize of %s", fileName);
	return -1;
    }

    // Now write name of file, punctuated by null char
    if (write(arkiv, fileName, strlen(fileName) + 1) 
	    != strlen(fileName) + 1){
	snprintf(errDesc, ERR_STATUS_LENGTH, 
		"writing to name of filesize of %s ",  fileName);
	return -1;
    }
    return 0;
}

// Archive fileName, add to log, and abort if already logged. 
// Return 0 on success, -1 on failure
int archiveFileAndLog (char * fileName, struct stack * log, int arkiv){
    // Did we already add 'fileName' to archive? If so, don't add it
    if(stackContains(log, fileName)) return 0;


    char block[WRITE_BUFFER_SIZE];
    int in;
    int nread;

    if ((in = open(fileName, O_RDONLY)) == -1){
	snprintf(errDesc, ERR_STATUS_LENGTH, "opening %s", fileName);
	return -1;
    }
    
    struct stat statBuf;
    if (lstat(fileName, &statBuf) == -1){
	snprintf(errDesc, ERR_STATUS_LENGTH, "running lstat on %s", fileName);
	return -1;
    }
    
    if(archiveFileSizeAndName(fileName, arkiv, statBuf.st_size) == -1){
	return -1;
    }

    while ((nread = read(in, block, WRITE_BUFFER_SIZE)) > 0){
	if (write(arkiv, block, nread) < 0){
	    snprintf(errDesc, ERR_STATUS_LENGTH, 
		    "Writing to archive contents of %s", fileName);
	    return -1;
	}
    }
    // Don't write this file twice
    stackAddFile(log, fileName);
    return 0;

}

// Move archive from fromArchive to toArchive, log if successful, 
// and abort if already logged. Return 0 if success, -1 if failure.
int moveArchiveAndLog(char * fileName, struct stack * log, 
	int fromArchive, int toArchive, off_t nbytes){
    if(stackContains(log, fileName)) return 0;
    
    if(archiveFileSizeAndName(fileName, toArchive, nbytes) == -1){
	return -1;
    }
    if(copyBytes(fileName, fromArchive, toArchive, nbytes) == -1){
	return -1;
    }
    stackAddFile(log, fileName);
    return 0;
}

// Copy 'nbytes' from fromFile to toFile, return -1 if error
int copyBytes(char *toFileName, int fromFile, int toFile, off_t nbytes){
    // toFileName is just for debugging purposes
    // How many more bytes are left to copy from 'fromFile' to 'toFile'
    off_t bytesLeft = nbytes;
    // Copy in 1kb chunks, or whatever is decided in top of file
    char block [WRITE_BUFFER_SIZE];

    while(bytesLeft > WRITE_BUFFER_SIZE){
	if(read(fromFile, block, WRITE_BUFFER_SIZE) != WRITE_BUFFER_SIZE){
	    snprintf(errDesc, ERR_STATUS_LENGTH, 
		    "reading to copy buffer for archived file %s", toFileName);
	    return -1;
	}
	if(write(toFile, block, WRITE_BUFFER_SIZE) != WRITE_BUFFER_SIZE){
	    snprintf(errDesc, ERR_STATUS_LENGTH, 
		    "writing copy buffer to archive for archived file %s", toFileName);
	    return -1;
	}
	// Record how many bytes are left to move
	bytesLeft -= WRITE_BUFFER_SIZE;
    }

    // Move remainder of bytes over
    if(read(fromFile, block, bytesLeft) != bytesLeft){
	snprintf(errDesc, ERR_STATUS_LENGTH, 
		"reading last bytes to copy buffer for archived file %s", toFileName);
	return -1;
    }
    if(write(toFile, block, bytesLeft) != bytesLeft){
	snprintf(errDesc, ERR_STATUS_LENGTH, 
		"writing last bytes from copy buffer to archive for archived file %s", 
		toFileName);
    }
    return 0;
}

// Extract fileName from archive. File is nbytes long. 
// Return 0 if successful, -1 if failure.
int extractFileFromArchive(char * fileName, int arkiv, off_t nbytes){
    // Set up required paths for fileName
    if(setupPath(fileName) != 0){
	return -1;
    }
    
    if(fileName[strlen(fileName) - 1] == '/' && isDir(fileName) == 1){
	// setupPath already took care of it for us
	return 0;
    }

    // Check if file is read-only
    if(clearFile(fileName) == 0){
	snprintf(errDesc, ERR_STATUS_LENGTH, "removing read-only %s", fileName);
	return -1;
    }

    // File descriptor for file to extract
    int outputFile;

    if ((outputFile= open(fileName, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1){
	snprintf(errDesc, ERR_STATUS_LENGTH, "opening initial %s", fileName);
	return -1;
    }
    
    // Write to file
    if(copyBytes(fileName, arkiv, outputFile, nbytes) == -1){
	return -1;
    }

    return 0;
}

// Try to delete this file to make way for new file.
// Returns -1 if error, 0 if read-only, 1 if deleted or doesn't exist
int clearFile(char * fileName){

    // Check if file is read-only
    struct stat statBuf;
    if(lstat(fileName, &statBuf) == -1){
	// File must not exist
	errno = 0;
	return 1;
    }else if (statBuf.st_mode & S_IWUSR){
	// File is writeable, so delete it
	if(unlink(fileName) == -1){
	    snprintf(errDesc, ERR_STATUS_LENGTH, "deleting file %s", fileName);
	    return -1;
	}
	return 1;
    }

    return 0;
}


// Create all necessary directories for 'path'. For example, 'a/b/c'
// will create a/, then a/b/.
int setupPath(char * path){
    char currentPath[PATH_MAX];
    int currentCharIndex = 0;

    while((currentPath[currentCharIndex] = path[currentCharIndex]) 
	    != '\0'){
	if(currentPath[currentCharIndex] == '/'){
	    currentPath[currentCharIndex + 1] = '\0';
	    if(mkdir(currentPath, S_IXUSR | S_IRUSR | S_IWUSR
			| S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0){
		// Directory already exists, but that's okay so reset errno
		errno = 0;
	    }

	    // Verify directory has been made
	    switch (isDir(currentPath)){
		case -1:
		    return -1;
		    break;
		case 0:
		    snprintf(errDesc, ERR_STATUS_LENGTH, "validating %s is a directory", currentPath);
		    return -1;
		    break;
	    }
	}
	currentCharIndex++;
    }

    return 0;
}

// Check if file/directory exists. Return t/f value.
int fileOrDirExists(char * fileName){
    struct stat statBuf;
    if(lstat(fileName, &statBuf) == -1){
	// Doesn't exist
	errno = 0;
	return 0;
    }else if (S_ISREG(statBuf.st_mode) || S_ISDIR(statBuf.st_mode)){
	// It is a regular file
	return 1;
    }else {
	// Not a regular file
	return 0;
    }
	    
}

// Check if 'path' is a directory.
// Return -1 for error, 0 for no, 1 for yes
int isDir(char * path){
    struct stat statBuf;
    if(lstat(path, &statBuf) == -1){
	// File doesn't exist, so probably not a directory
	return 0;
    }

    if(S_ISDIR(statBuf.st_mode)){
	return 1;
    } else {
	return 0;
    }
}

// Check if parentDir is a parent directory of filename. Return t/f value.
int checkIfParentDirectory(char * parentDir, char * fileName){
    int currentCharIndex = 0;
    if(strlen(parentDir) >= strlen(fileName)){
	// Parent can't be longer than or equal to file in length
	return 0;
    }
    while(parentDir[currentCharIndex] != '\0' 
	    && fileName[currentCharIndex] != '\0'){
	if(parentDir[currentCharIndex] != fileName[currentCharIndex]){
	    // They don't match at some point
	    return 0;
	}
	currentCharIndex++;
    }

    // Make sure the last char of parentDir or the next char of fileName is '/'
    if (parentDir[strlen(parentDir) - 1] == '/' 
	    || (strlen(fileName) > strlen(parentDir) 
		&& fileName[strlen(parentDir)] == '/')){
	return 1;
    }else {
	return 0;
    }
}
