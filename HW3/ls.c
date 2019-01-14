/***** INCLUDES *****/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include <string.h>
#include <dirent.h>

/**** MACROS ****/
#define MAX_PATH 255
#define READ_FLAGS O_RDONLY
#define WRITE_FLAGS (O_WRONLY | O_APPEND | O_CREAT)
#define WRITE_PERMS (S_IRUSR | S_IWUSR )
#define BLKSIZE 256
#define EXIT_SUCCESSFUL 0
#define EXIT_UNSUCCESSFUL 1



/**** PROTOTYPES ****/
ssize_t r_write(int fd, void *buf, size_t size);
static void signalHandlerSIGINT(int sigNo);


int main(int argc, char** argv){

	char currentWorkingPath[MAX_PATH];
	DIR *currentDir;
	struct dirent *direntp;
	struct stat fileStat;
	char fileDoc[MAX_PATH];
	char fileName[MAX_PATH];
	char fileType[MAX_PATH];
	char fileRights[MAX_PATH];
	char fileSize[MAX_PATH];
	int fileDescriptor = 0;
	struct sigaction signalActionSIGINT;

	//Signal handler
    signalActionSIGINT.sa_handler = signalHandlerSIGINT;
    signalActionSIGINT.sa_flags = 0;
    if ((sigemptyset(&signalActionSIGINT.sa_mask) == -1) || (sigaction(SIGINT, &signalActionSIGINT, NULL) == -1)) {
        fprintf(stderr, "Failed to set SIGINT handler for Process A");
        exit(1);
    }

	// Bulunan directory açılır.
	getcwd(currentWorkingPath, MAX_PATH);
	if((currentDir = opendir(currentWorkingPath)) == NULL){
		fprintf(stderr,"Failed to open directory : %s\n", currentWorkingPath);
		exit(1);
	}

	// Directory içerisindeki dosyalar teker okunarak bir arraye kaydedilir 
	while((direntp = readdir(currentDir)) != NULL){

		/* Klasor icinde gizli dosyalara takilmadan atlanir */
		if (strcmp(direntp->d_name, "..") == 0 || strcmp(direntp->d_name, ".") == 0 || direntp->d_name[strlen(direntp->d_name) - 1] == '~')
			continue;


		if(stat(direntp->d_name, &fileStat) < 0){
	        fprintf(stderr,"Failed to get stat for the file\n");
			continue;
		}


		/*Get FileName*/
		sprintf(fileName,"%s\0",direntp->d_name);

		/* Get File Type */

		if(S_ISREG(fileStat.st_mode)){
			sprintf(fileType,"Regular File\0");
		}else if(S_ISDIR(fileStat.st_mode)){
			sprintf(fileType,"Directory\0");
		}else if(S_ISCHR(fileStat.st_mode)){
			sprintf(fileType,"Character Device\0");
		}else if(S_ISBLK(fileStat.st_mode)){
			sprintf(fileType,"Block Device\0");
		}else if(S_ISFIFO(fileStat.st_mode)){
			sprintf(fileType,"FIFO\0");
		}else if(S_ISLNK(fileStat.st_mode)){
			sprintf(fileType,"Symbolic Link\0");
		}else if(S_ISSOCK(fileStat.st_mode)){
			sprintf(fileType,"Socket\0");
		}else{
			sprintf(fileType,"Unknown File Type\0");
		}

		/* Get Access rights */
		sprintf(fileRights,"%c%c%c%c%c%c%c%c%c\0",
			(fileStat.st_mode & S_IRUSR) ? 'r' : '-',
			(fileStat.st_mode & S_IWUSR) ? 'w' : '-',
			(fileStat.st_mode & S_IWUSR) ? 'x' : '-',
			(fileStat.st_mode & S_IRGRP) ? 'r' : '-',
			(fileStat.st_mode & S_IWGRP) ? 'w' : '-',
			(fileStat.st_mode & S_IXGRP) ? 'x' : '-',
			(fileStat.st_mode & S_IROTH) ? 'r' : '-',
			(fileStat.st_mode & S_IWOTH) ? 'w' : '-',
			(fileStat.st_mode & S_IXOTH) ? 'x' : '-');


		/* Get Filze Size */ 
		sprintf(fileSize,"%zu\0",fileStat.st_size);

		/* Create a string for this specific file */
		sprintf(fileDoc,"%s %s %s %s\n",fileName, fileType, fileRights, fileSize);

		if(argc == 1){
			printf("%s",fileDoc);
		}else if(argc == 3){
			if(argv[1][0] == '>'){
				if((fileDescriptor = open(argv[2],WRITE_FLAGS,WRITE_PERMS)) == -1){
					fprintf(stderr, "Failed to open file -> %s\n",argv[2] );
					return EXIT_UNSUCCESSFUL;
				}
				r_write(fileDescriptor,fileDoc,strlen(fileDoc));
			}
			else{
				fprintf(stderr, "Command not supported %s %s %s\n", argv[0], argv[1], argv[2] );
			}
		}
	}

	closedir(currentDir);

	return 1;
}

ssize_t r_write(int fd, void *buf, size_t size) {
    char *bufp;
    size_t bytestowrite;
    ssize_t byteswritten;
    size_t totalbytes;

    for (bufp = buf, bytestowrite = size, totalbytes = 0;
         bytestowrite > 0;
         bufp += byteswritten, bytestowrite -= byteswritten) {
        byteswritten = write(fd, bufp, bytestowrite);
        if ((byteswritten) == -1 && (errno != EINTR))
            return -1;

        if (byteswritten == -1)
            byteswritten = 0;
        totalbytes += byteswritten;
    }
    return totalbytes;
}

//Signal handler for ls
static void signalHandlerSIGINT(int sigNo) {
    puts("\nSIGINT signal caught, exiting ls\n");    
    exit(1);
}