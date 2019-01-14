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
#define BLKSIZE PIPE_BUF
#define EXIT_SUCCESSFUL 0
#define EXIT_UNSUCCESSFUL 1

/**** PROTOTYPES ****/
ssize_t r_write(int fd, void *buf, size_t size);
ssize_t r_read(int fd, void *buf, size_t size);
int copyfile(int fromfd, int tofd);
int readwrite(int fromfd, int tofd);
static void signalHandlerSIGINT(int sigNo);

int main(int argc, char** argv){

	int lineCount = 0;
	int fileDescriptor1 = 0, fileDescriptor2 = 0;
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
	}
*/
	if((fileDescriptor1 = open(argv[1],READ_FLAGS)) == -1){
		fprintf(stderr, "Failed to open file %s\n",argv[1]);
		exit(EXIT_UNSUCCESSFUL);
	}

	if(argc == 2 ){ //simple cat call. Output to screen
		copyfile(fileDescriptor1,STDOUT_FILENO);
		return EXIT_SUCCESSFUL;
	}else if(argc == 4){ //cat call with > or < or | 
	
		if(argv[2][0]== '>'){
			if((fileDescriptor2 = open(argv[3],WRITE_FLAGS,WRITE_PERMS)) == -1){
				fprintf(stderr, "Failed to open file %s\n",argv[3]);
				exit(EXIT_UNSUCCESSFUL);
			}
			copyfile(fileDescriptor1,fileDescriptor2);
		}

	}

	return 0;
}

/* Start of functions that are in the book */
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

int copyfile(int fromfd, int tofd) {
   int bytesread;
   int totalbytes = 0;

   while ((bytesread = readwrite(fromfd, tofd)) > 0)
      totalbytes += bytesread;
   return totalbytes;
}

int readwrite(int fromfd, int tofd) {
   char buf[BLKSIZE];
   int bytesread;

   if ((bytesread = r_read(fromfd, buf, BLKSIZE)) < 0)
      return -1;
   if (bytesread == 0)
      return 0;
   if (r_write(tofd, buf, bytesread) < 0)
      return -1;
   return bytesread;
}
/* End of functions that are in the book */

//Signal handler for cat
static void signalHandlerSIGINT(int sigNo) {
    puts("\nSIGINT signal caught, exiting cat\n");    
    exit(1);
}