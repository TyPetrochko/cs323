#include "Far.h"
#define WRITE_BUFFER_SIZE 1024
#define ERR_STATUS_LENGTH 100

// Describing error task
char errDesc[ERR_STATUS_LENGTH];

int main (int argc, char *argv[]){
    
    // Normalize names from input
    stripLeadingSlashes(argc, argv);

    if (argc < 3){
	fprintf(stderr, "Far: Far r|x|d|t archive [filename]*\n");
    } else if (!strcmp("r", argv[1])){
	// User wants to add or update files to directory
	if(rcommand(argc, argv) == 0){
	    // R command finished successfully
	}else {
	    fprintf(stderr, "Error %s: %s\n", errDesc, strerror(errno));
	}
    } else if (!strcmp("t", argv[1])){
	if(tcommand(argc, argv) == 0){
	    // T command finished successfully
	}else{
	    fprintf(stderr, "Error %s: %s\n", errDesc, strerror(errno));
	}
    } else if (!strcmp("x", argv[1])){
	if(xcommand(argc, argv) == 0){
	    // X command finished successfully
	}else {
	    fprintf(stderr, "Error %s: %s\n", errDesc, strerror(errno));
	}
    } else if(!strcmp("d", argv[1])){
	if(dcommand(argc, argv) == 0){
	    // D command finished successfully
	}else {
	    fprintf(stderr, "Error %s: %s\n", errDesc, strerror(errno));
	}
    }else {
	fprintf(stderr, "Far: Far r|x|d|t archive [filename]*\n");
    }
    return 0;
}

// Remove leading /s from each command line name
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

struct stack * stackCreate(){
    struct stack * toReturn = malloc(sizeof(struct stack));
    toReturn->fileName = malloc(1);
    toReturn->fileName[0] = '\0';
    toReturn->next = NULL;
    return toReturn;
}
    
int stackIsEmpty(struct stack * log){
   return (log->next == NULL); 
}

void stackAddFile(struct stack * log, char * fileName){
#ifdef DEBUG
    printf("stackAddFile(%s)\n", fileName);
#endif
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

int stackContains(struct stack * log, char * fileName){
    // We will test if it exists as a directory, too
    char dirName[PATH_MAX];
    strcpy(dirName, fileName);
    if(dirName[strlen(fileName) - 1] != '/') strcat(dirName, "/");

#ifdef DEBUG
    printf("stackContains(%s or %s)\n", fileName, dirName);
#endif
    // Iterator element
    struct stack * iterator = log;
    while (iterator != NULL){
#ifdef DEBUG
	printf("    Checking %s\n", iterator->fileName);
#endif
	// If this stack element contains the filename, return true
	if (iterator->fileName != '\0' 
		&& (!strcmp(iterator->fileName, fileName) 
		    || !strcmp(iterator->fileName, dirName))) {
#ifdef DEBUG
	    printf("    Found it! Return true\n");
#endif
	    return 1;
	}
	iterator = iterator->next;
    }
    // log does not contain fileName
    return 0;
}

int stackContainsChild(struct stack * log, char * fileName){
    // We will test if it exists as a directory, too
    char dirName[PATH_MAX];
    strcpy(dirName, fileName);
    if(dirName[strlen(fileName) - 1] != '/') strcat(dirName, "/");

#ifdef DEBUG
    printf("stackContains(%s or %s)\n", fileName, dirName);
#endif
    // Iterator element
    struct stack * iterator = log;
    while (iterator != NULL){
#ifdef DEBUG
	printf("    Checking %s\n", iterator->fileName);
#endif
	// If this stack element contains the filename, return true
	if (iterator->fileName != '\0' 
		&& ((!strcmp(iterator->fileName, fileName) 
		    || !strcmp(iterator->fileName, dirName))
		    || checkIfParentDirectory(fileName, iterator->fileName))) {
#ifdef DEBUG
	    printf("    Found it! Return true\n");
#endif
	    return 1;
	}
	iterator = iterator->next;
    }
    // log does not contain fileName
    return 0;
}

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

// Archive or update files supplied by command line
int rcommand (int argc, char *argv[]){
#ifdef DEBUG
    printf("rcommand\n");
#endif
    // Get archive name from command line
    char * arkName = argv[2];

    // File descriptor for existing archive
    int arkiv;

    // Make sure 'open' is workinging
    if(!strcmp(arkName, "-")){
	// If archive name is '-' then read from std_in
	arkiv = STDIN_FILENO;
    } else if ((arkiv = open(arkName, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1){
	snprintf(errDesc, ERR_STATUS_LENGTH, "opening initial archive %s", arkName);
	return -1;
    }
    
    /* Make new archive (temporary) to copy to
     * ... if it succeeds, we will keep temp archive
     * else we will unlink it. */
    int newArkiv;
    char tmpName [PATH_MAX];
    if(!strcmp(arkName, "-")){
	newArkiv = STDOUT_FILENO;
    }else{
	snprintf(tmpName, PATH_MAX, "%sXXXXXX", arkName);
	if ((newArkiv = mkstemp(tmpName)) == -1){
	    snprintf(errDesc, ERR_STATUS_LENGTH, 
		    "making new tmp archive with name %s", tmpName);
	    return -1;
	}
    }
    
    // Use stack to log added files
    Stack filesAdded = stackCreate();

    // Variable to store filesize of current archive
    off_t fileSize;

    // Read through archive 'arkiv'
    // For each file, filesize is stored, then file's name, terminated by null char
#ifdef DEBUG
    printf("    scanning through archive\n");
#endif
rreadloop:
    while(read(arkiv, &fileSize, sizeof(off_t)) > 0){
	// File's name
	char nameBuffer[PATH_MAX];
	// Read into nameBuffer until null char
	int posInNameBuff = 0;
	while(read(arkiv, nameBuffer + posInNameBuff, 1) == 1 
		&& nameBuffer[posInNameBuff++] != '\0');

	// Do we have a valid file?
	if(strlen(nameBuffer) < 1){
	    // Abort mission. Roll back.
	    snprintf(errDesc, ERR_STATUS_LENGTH, 
		    "reading file with name less than one char from archive %s", arkName);
	    stackDestroy(filesAdded);
	    if(arkiv != STDIN_FILENO) unlink(tmpName);
	    return -1;
	}
#ifdef DEBUG
	printf("    scanning file %s, checking if it should be updated or moved\n", nameBuffer);
#endif

	// If this file is in command line args, update it
	for(int i = 3; i < argc; i++){
	    // If 'zoo' is in command line, also check for 'zoo/'
	    char asDirectoryName[PATH_MAX];
	    strcpy(asDirectoryName, argv[i]);
	    if(asDirectoryName[strlen(asDirectoryName) - 1] != '/') strcat(asDirectoryName, "/");

	    if((!strcmp(nameBuffer, argv[i])
		    || !strcmp(nameBuffer, asDirectoryName)
		    || checkIfParentDirectory(asDirectoryName, nameBuffer))){
		// Update file and log it to avoid repeats
#ifdef DEBUG
		printf("        it is listed in command line args, so try to update it\n");
#endif
		if(!fileOrDirExists(argv[i])){
		    // File does not exist, but not a fatal error. Log as to not repeat.
		    fprintf(stderr, "Far: %s does not exist\n", argv[i]);
		    continue;
		}

		if(archiveFileOrDirectoryAndLog(argv[i], filesAdded, newArkiv) == -1){
		    // Something went wrong
		    if(arkiv != STDIN_FILENO) unlink(tmpName);
		    stackDestroy(filesAdded);
		    return -1;
		}
#ifdef DEBUG
		printf("        skipping to next file\n");
#endif
		// Skip ahead to next file
		if(lseek(arkiv, fileSize, SEEK_CUR) == -1){
		    // Couldn't skip ahead in file. Rollback.
		    if(arkiv != STDIN_FILENO) unlink(tmpName);
		    stackDestroy(filesAdded);
		    return -1;
		}

		goto rreadloop;

	    }
	}
#ifdef DEBUG
	printf("        %s is NOT in command line, so just move it to new archive\n", nameBuffer);
#endif
	// If we're here, then file is NOT in command line - move it to new archive...
	if(moveArchiveAndLog(nameBuffer, filesAdded, arkiv, newArkiv, fileSize) == -1){
	    // Could not copy this archived file to new archive. Rollback.
	    if(arkiv != STDIN_FILENO) unlink(tmpName);
	    return -1;
	}

    }

    // Start adding command line args
#ifdef DEBUG
    printf("    done scanning through existing archive, start adding command line files\n");
#endif
    for(int i = 3; i < argc; i++){
	if(!fileOrDirExists(argv[i])){
	    // File does not exist. We already issued an error so don't repeat
	    continue;
	}

#ifdef DEBUG
	printf("        adding file from commandline: %s\n", argv[i]);
#endif
	if(archiveFileOrDirectoryAndLog(argv[i], filesAdded, newArkiv) == -1){
	    if(arkiv != STDIN_FILENO)  unlink (tmpName);
	    return -1;
	}
    }

    // Done adding to archive. Switch old archive and temp archive
    stackDestroy(filesAdded);
    char tmpBackupName[PATH_MAX];
    snprintf(tmpBackupName, PATH_MAX, "%s.bak", arkName);
    if(arkiv != STDIN_FILENO && rename(arkName, tmpBackupName) == -1){
	// Can't rename first file with .bak. Rollback.
	snprintf(errDesc, ERR_STATUS_LENGTH, 
		"first step switching archives, renaming %s to %s", arkName, tmpBackupName);
	return -1;
    }
    if(arkiv != STDIN_FILENO && rename(tmpName, arkName) == -1){
	// Can't make tmpfile permanent. Rollback.
	snprintf(errDesc, ERR_STATUS_LENGTH, 
		"second step switching archives, renaming %s to %s", tmpName, arkName);
	rename(tmpBackupName, arkName);
	unlink(tmpName);
	return -1;
    }
    if(arkiv != STDIN_FILENO && close(arkiv) == -1){
	// Can't close original archive.
	snprintf(errDesc, ERR_STATUS_LENGTH, "closing old archive file %s", arkName);
	return -1;
    }
    if(arkiv != STDIN_FILENO && close(newArkiv) == -1){
	// Can't close original archive.
	snprintf(errDesc, ERR_STATUS_LENGTH, "closing archive file %s", tmpName);
	return -1;
    }
    if(arkiv != STDIN_FILENO && unlink(tmpBackupName) == -1){
	snprintf(errDesc, ERR_STATUS_LENGTH, 
		"unlinking original archive, renamed %s", tmpBackupName);
    }

    return 0;
}

int archiveFileOrDirectoryAndLog(char * fileName, struct stack * log,
	int arkivFileDis){
#ifdef DEBUG
    printf("archiveFileOrDirectoryAndLog(%s)\n", fileName);
#endif
    // Did we already add 'fileName' to archive? If so, don't add it
    if(stackContains(log, fileName)) return 0;


    // Find some information about fileName
    struct stat statBuf;
    if (lstat(fileName, &statBuf) == -1){
	// File does not exist, but not a fatal error. Log as to not repeat.
	snprintf(errDesc, ERR_STATUS_LENGTH, "running lstat on %s", fileName);
	return -1;
    }

    if(S_ISREG(statBuf.st_mode)){
	// 'fileName' refers to a regular file
	archiveFileAndLog(fileName, log, arkivFileDis);
    }else if (S_ISDIR(statBuf.st_mode)){
	// Add directory as a normal file, make sure it ends in '/'
	char dirName[PATH_MAX];
	strcpy(dirName, fileName);
	if(dirName[strlen(fileName) - 1] != '/') strcat(dirName, "/");

	// File of folder is zero
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
		    return -1;
		}
	    } 
	}
    }
    
    return 0;
}

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

int archiveFileAndLog (char * fileName, struct stack * log, int arkiv){	
    
    // Did we already add 'fileName' to archive? If so, don't add it
    if(stackContains(log, fileName)) return 0;

#ifdef DEBUG
    printf("archiveFileAndLog(%s)\n", fileName);
#endif
    struct stat statBuf;

    if (lstat(fileName, &statBuf) == -1){
	snprintf(errDesc, ERR_STATUS_LENGTH, "running lstat on %s", fileName);
	return -1;
    }
    
    if(archiveFileSizeAndName(fileName, arkiv, statBuf.st_size) == -1){
	return -1;
    }

    // Now write the rest of the file
    char block[WRITE_BUFFER_SIZE];
    // File descriptor for input file
    int in;
    // Number of bytes read in
    int nread;

    // Open file
    if ((in = open(fileName, O_RDONLY)) == -1){
	snprintf(errDesc, ERR_STATUS_LENGTH, "opening %s", fileName);
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

int moveArchiveAndLog(char * fileName, struct stack * log, int fromArchive, int toArchive, off_t nbytes){
    
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

int tcommand (int argc, char *argv[]){
#ifdef DEBUG
    printf("tcommand\n");
#endif
    char * arkName = argv[2];

    int arkiv;

    if(!strcmp(arkName, "-")){
	// If archive name is '-' then read from std_in
	arkiv = STDIN_FILENO;
    } else if ((arkiv = open(arkName, O_RDONLY, S_IRUSR | S_IWUSR)) == -1){
	snprintf(errDesc, ERR_STATUS_LENGTH, "opening initial archive %s", arkName);
	return -1;
    }
    
    // Variable to store filesize of current file in archive
    off_t fileSize;

    // Read through archive 'arkiv'
    // For each file, filesize is stored, then file's name, terminated by null char
#ifdef DEBUG
    printf("    scanning through archive\n");
#endif
    while(read(arkiv, &fileSize, sizeof(off_t)) > 0){
	// File's name
	char nameBuffer[PATH_MAX];
	// Read into nameBuffer until null char
	int posInNameBuff = 0;
	while(read(arkiv, nameBuffer + posInNameBuff, 1) == 1 
		&& nameBuffer[posInNameBuff++] != '\0');

	// Do we have a valid file?
	if(strlen(nameBuffer) < 1){
	    // Abort mission. Roll back.
	    snprintf(errDesc, ERR_STATUS_LENGTH, 
		    "reading file '%s' with name less than one char from archive %s", nameBuffer, arkName);
	    return -1;
	}
	
	// Print file
	printf("%8d %s\n", (int) fileSize, nameBuffer);

	// Skip ahead to next file
	if(lseek(arkiv, fileSize, SEEK_CUR) == -1){
	    // Couldn't skip ahead in file. Rollback.
	    return -1;
	}
    }
    
    if(close(arkiv) == -1){
	// Can't close original archive.
	snprintf(errDesc, ERR_STATUS_LENGTH, "closing old archive file %s", arkName);
	return -1;
    }
    return 0;
}

int xcommand (int argc, char *argv[]){
#ifdef DEBUG
    printf("xcommand\n");
#endif
    // Get archive name from command line
    char * arkName = argv[2];

    // File descriptor for existing archive
    int arkiv;

    // Make sure 'open' is working
    if(!strcmp(arkName, "-")){
	// If archive name is '-' then read from std_in
	arkiv = STDIN_FILENO;
    } else if ((arkiv = open(arkName, O_RDONLY, S_IRUSR | S_IWUSR)) == -1){
	snprintf(errDesc, ERR_STATUS_LENGTH, "opening initial archive %s", arkName);
	return -1;
    }
    
    // Use a log so we can check for leftover names
    Stack filesAdded = stackCreate();

    // Variable to store filesize of current archive
    off_t fileSize;

    // Read through archive 'arkiv'
    // For each file, filesize is stored, then file's name, terminated by null char
#ifdef DEBUG
    printf("    scanning through archive\n");
#endif
xreadloop:
    while(read(arkiv, &fileSize, sizeof(off_t)) > 0){
	// File's name
	char nameBuffer[PATH_MAX];
	// Read into nameBuffer until null char
	int posInNameBuff = 0;
	while(read(arkiv, nameBuffer + posInNameBuff, 1) == 1 
		&& nameBuffer[posInNameBuff++] != '\0');

	// Do we have a valid file?
	if(strlen(nameBuffer) < 1){
	    // Abort mission. Roll back.
	    snprintf(errDesc, ERR_STATUS_LENGTH, 
		    "reading file '%s' with name less than one char from archive %s", nameBuffer, arkName);
	    return -1;
	}


	// If this file is in command line args, or there are no command line args, extract
	int shouldExtract = 0;
	if (argc == 3) {
#ifdef DEBUG
	    printf("    three command line args, so extract it\n");
#endif
	    shouldExtract = 1;
	}else{
#ifdef DEBUG
	    printf("    scanning file %s, checking if it should be extracted\n", nameBuffer);
#endif
	    for(int i = 3; i < argc; i++){
		// If 'zoo' is in command line, also check for 'zoo/'
		char asDirectoryName[PATH_MAX];
		strcpy(asDirectoryName, argv[i]);
		if(asDirectoryName[strlen(asDirectoryName) - 1] != '/') strcat(asDirectoryName, "/");
		if(!strcmp(nameBuffer, argv[i]) 
			|| !strcmp(nameBuffer, asDirectoryName)
			|| checkIfParentDirectory(asDirectoryName, nameBuffer)){
#ifdef DEBUG
		    printf("        it is listed in command line args, so extract it\n");
#endif
		    shouldExtract = 1;
		}
	    }
	}
	
	if(shouldExtract){
#ifdef DEBUG
	    printf("    extract %s\n", nameBuffer);
#endif
	    if(extractFileFromArchive(nameBuffer, arkiv, fileSize) == -1){
		// Couldn't extract. Rollback.
		stackAddFile(filesAdded, nameBuffer);
		fprintf(stderr, "Far: cannot extract %s\n", nameBuffer);
	    }else{
		stackAddFile(filesAdded, nameBuffer);
		goto xreadloop;
	    }
	}

	// If we've gotten here, we don't have to extract it... skip to next file
	if(lseek(arkiv, fileSize, SEEK_CUR) == -1){
	    snprintf(errDesc, ERR_STATUS_LENGTH, "skipping past file %s of size %lld", nameBuffer, (long long) fileSize);
	    return -1;
	}
    }

    // Make sure that all the arg commands were extracted
    for (int i = 3; i < argc; i++){
	if(!stackContainsChild(filesAdded, argv[i])){
	    fprintf(stderr, "Far: cannot find %s\n", argv[i]);
	}
    }

    // Done extracting
    stackDestroy(filesAdded);
    if(close(arkiv) == -1){
	// Can't close archive.
	snprintf(errDesc, ERR_STATUS_LENGTH, "closing archive file %s", arkName);
	return -1;
    }
    
    return 0;
}

int dcommand(int argc, char * argv[]){

    struct stack * log = stackCreate();
#ifdef DEBUG
    printf("dcommand\n");
#endif
    // Get archive name from command line
    char * arkName = argv[2];

    // File descriptor for existing archive
    int arkiv;

    // Make sure 'open' is working
    if(!strcmp(arkName, "-")){
	// If archive name is '-' then read from std_in
	arkiv = STDIN_FILENO;
    } else if ((arkiv = open(arkName, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1){
	snprintf(errDesc, ERR_STATUS_LENGTH, "opening initial archive %s", arkName);
	return -1;
    }
    
    /* Make new archive (temporary) to copy to
     * ... if it succeeds, we will keep temp archive
     * else we will unlink it. */
    int newArkiv;
    char tmpName [PATH_MAX];
    if(!strcmp(arkName, "-")){
	newArkiv = STDOUT_FILENO;
    }else{
	snprintf(tmpName, PATH_MAX, "%sXXXXXX", arkName);
	if ((newArkiv = mkstemp(tmpName)) == -1){
	    snprintf(errDesc, ERR_STATUS_LENGTH, 
		    "making new tmp archive with name %s", tmpName);
	    return -1;
	}
    }
    
    // Variable to store filesize of current archive
    off_t fileSize;

    // Read through archive 'arkiv'
    // For each file, filesize is stored, then file's name, terminated by null char
#ifdef DEBUG
    printf("    scanning through archive\n");
#endif
dreadloop:
    while(read(arkiv, &fileSize, sizeof(off_t)) > 0){
	// File's name
	char nameBuffer[PATH_MAX];
	// Read into nameBuffer until null char
	int posInNameBuff = 0;
	while(read(arkiv, nameBuffer + posInNameBuff, 1) == 1 
		&& nameBuffer[posInNameBuff++] != '\0');

	// Do we have a valid file?
	if(strlen(nameBuffer) < 1){
	    // Abort mission. Roll back.
	    snprintf(errDesc, ERR_STATUS_LENGTH, 
		    "reading file '%s' with name less than one char from archive %s", nameBuffer, arkName);
	    return -1;
	}
	
	// Check if this file is in command line
	for(int i = 3; i < argc; i++){
	    // If 'zoo' is in command line, also check for 'zoo/'
	    char asDirectoryName[PATH_MAX];
	    strcpy(asDirectoryName, argv[i]);
	    if(asDirectoryName[strlen(asDirectoryName) - 1] != '/') strcat(asDirectoryName, "/");
	    if(!strcmp(nameBuffer, argv[i]) 
		    || !strcmp(nameBuffer, asDirectoryName)
		    || checkIfParentDirectory(asDirectoryName, nameBuffer)){
#ifdef DEBUG
		printf("        it is listed in command line args, so skip it\n");
#endif
		// Skip ahead to next file
		if(lseek(arkiv, fileSize, SEEK_CUR) == -1){
		    // Couldn't skip ahead in file. Rollback.
		    return -1;
		}
		stackAddFile(log, nameBuffer);
		goto dreadloop;
	    }
	}

	// Not listed - so copy to new file

	// Add file size and name to archive
	if(archiveFileSizeAndName(nameBuffer, newArkiv, fileSize) == -1){
	    return -1;
	}
       
	// Add rest of file
	if(copyBytes(nameBuffer, arkiv, newArkiv, fileSize) == -1){
	    return -1;
	}
	    
    }
    
    // Make sure that all the arg commands were deleted
    for (int i = 3; i < argc; i++){
	if(!stackContainsChild(log, argv[i])){
	    fprintf(stderr, "Far: cannot find %s\n", argv[i]);
	}
    }

    // Done! Destory stack and switch files
    stackDestroy(log);
    char tmpBackupName[PATH_MAX];
    snprintf(tmpBackupName, PATH_MAX, "%s.bak", arkName);
    if(arkiv != STDIN_FILENO && rename(arkName, tmpBackupName) == -1){
	// Can't rename first file with .bak. Rollback.
	snprintf(errDesc, ERR_STATUS_LENGTH, 
		"first step switching archives, renaming %s to %s", arkName, tmpBackupName);
	unlink(tmpName);
	return -1;
    }
    if(arkiv != STDIN_FILENO && rename(tmpName, arkName) == -1){
	// Can't make tmpfile permanent. Rollback.
	snprintf(errDesc, ERR_STATUS_LENGTH, 
		"second step switching archives, renaming %s to %s", tmpName, arkName);
	rename(tmpBackupName, arkName);
	if(arkiv != STDIN_FILENO) unlink(tmpName);
	return -1;
    }
    if(arkiv != STDIN_FILENO && close(arkiv) == -1){
	// Can't close original archive.
	snprintf(errDesc, ERR_STATUS_LENGTH, "closing old archive file %s", arkName);
	return -1;
    }
    if(arkiv != STDIN_FILENO && close(newArkiv) == -1){
	// Can't close original archive.
	snprintf(errDesc, ERR_STATUS_LENGTH, "closing archive file %s", tmpName);
	return -1;
    }
    if(arkiv != STDIN_FILENO && unlink(tmpBackupName) == -1){
	snprintf(errDesc, ERR_STATUS_LENGTH, 
		"unlinking original archive, renamed %s", tmpBackupName);
    }

    return 0;
}

int copyBytes(char *toFileName, int fromFile, int toFile, off_t nbytes){
    // toFileName is just for debugging purposes
#ifdef DEBUG
    printf("copyBytes(%s)\n", toFileName);
#endif

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

int extractFileFromArchive(char * fileName, int arkiv, off_t nbytes){
#ifdef DEBUG
    printf("extractFileFromArchive(%s)\n", fileName);
#endif
    // Set up required paths for fileName
    if(setupPath(fileName) != 0){
	return -1;
    }
    
    if(isDir(fileName) == 1){
	// setupPath already took care of it for us
#ifdef DEBUG
	printf("     %s is already a directory\n", fileName);
#endif
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

int setupPath(char * path){
    // This file creates a directory for every instance of '/' in path (if doesn't exist)
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

// Check if file exists
int fileOrDirExists(char * fileName){
#ifdef DEBUG
    printf("fileOrDirExists(%s)\n", fileName);
#endif
    struct stat statBuf;
    if(lstat(fileName, &statBuf) == -1){
	// Doesn't exist
#ifdef DEBUG
    printf("    %s doesn't exist\n", fileName);
#endif
	errno = 0;
	return 0;
    }else if (S_ISREG(statBuf.st_mode) || S_ISDIR(statBuf.st_mode)){
#ifdef DEBUG
    printf("    %s DOES exist\n", fileName);
#endif
	// It is a regular file
	return 1;
    }else {
#ifdef DEBUG
    printf("    %s is not a regular file or dir\n", fileName);
#endif
	// Not a regular file
	return 0;
    }
	    
}

// Check if 'path' is a directory
int isDir(char * path){
    // Return -1 for error, 0 for no, 1 for yes
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

// Check if parentDir is a parent directory of filename
int checkIfParentDirectory(char * parentDir, char * fileName){
#ifdef DEBUG
    printf("checkIfParentDirectory(%s, %s)\n", parentDir, fileName);
#endif
    int currentCharIndex = 0;
    
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
#ifdef DEBUG
	printf("    Yes, %s is a parent of %s\n", parentDir, fileName);
#endif
	return 1;
    }else {
#ifdef DEBUG
	printf("    No, %s not a parent of %s\n", parentDir, fileName);
#endif
	return 0;
    }
}
