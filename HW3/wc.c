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
#define WRITE_FLAGS (O_WRONLY | O_CREAT)
#define WRITE_PERMS (S_IRUSR | S_IWUSR )
#define EXIT_SUCCESSFUL 0
#define EXIT_UNSUCCESSFUL 1

/**** PROTOTYPES ****/
int readline(char* fileName);
ssize_t r_write(int fd, void *buf, size_t size);
ssize_t r_read(int fd, void *buf, size_t size);
static void signalHandlerSIGINT(int sigNo);


int main(int argc, char** argv){

	int lineCount = 0;
	int fileDescriptor = 0;
	struct sigaction signalActionSIGINT;

	//Signal handler
    signalActionSIGINT.sa_handler = signalHandlerSIGINT;
    signalActionSIGINT.sa_flags = 0;
    if ((sigemptyset(&signalActionSIGINT.sa_mask) == -1) || (sigaction(SIGINT, &signalActionSIGINT, NULL) == -1)) {
        fprintf(stderr, "Failed to set SIGINT handler for Process A");
        exit(1);
    }

/*
	if(argc != 2  || argc != 4){
		fprintf(stderr,"Usage : %s FileName, Operator*, OperatorFile*\n *These fields are not necessary\n", argv[0]);
		return EXIT_SUCCESSFUL;
	}*/

	if(argc == 2 ){ //simple wc call. Output to screen
		lineCount = readline(argv[1]);
		printf("%d\n", lineCount);
		return EXIT_SUCCESSFUL;
	}else if(argc == 4){ //wc call with > or < or | 
		lineCount = readline(argv[1]);

		if(argv[2][0]== '>'){
			if((fileDescriptor = open(argv[3],WRITE_FLAGS,WRITE_PERMS)) == -1){
				fprintf(stderr, "Failed to open file %s\n",argv[3]);
				exit(EXIT_UNSUCCESSFUL);
			}
			r_write(fileDescriptor,&lineCount, sizeof(int));
		}
	}



	return 0;
}

int readline(char* fileName){
	int lineCount = 0;
	int fileDescriptor = 0;
	char bufp = 'a';
	int retval = 1;
	int numread = 0;

	if((fileDescriptor = open(fileName,READ_FLAGS)) == -1){
		fprintf(stderr,"Failed to open file %s\n",fileName);
		exit(EXIT_UNSUCCESSFUL);
	}

	while(retval != 0){
		retval = r_read(fileDescriptor,&bufp,sizeof(char));
		if(retval == -1){
			return -1;
		}
		if(retval == 0){//end of file
			return ++lineCount;
		}
		numread++;
		if(bufp == '\n'){
			++lineCount;
		}
	
	}

	return lineCount;
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

ssize_t r_read(int fd, void *buf, size_t size) {
   ssize_t retval;
   while (retval = read(fd, buf, size), retval == -1 && errno == EINTR) ;
   return retval;
}

//Signal handler for wc
static void signalHandlerSIGINT(int sigNo) {
    puts("\nSIGINT signal caught, exiting wc\n");    
    exit(1);
}