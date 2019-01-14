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
#include <complex.h>
#include <math.h>
#include <string.h>

/***** MACROS *****/
#define WRITE_FLAGS (O_WRONLY | O_APPEND | O_CREAT)
#define WRITE_PERMS (S_IRUSR | S_IWUSR )
#define RNGLIMIT 50
#define PROCESSA_FILENAME "processA.txt"
#define PROCESSB_FILENAME "processB.txt"

/**** PROTOTYPES ****/
void parentWork(int seqLength, char* fileName, int maxSize);
void childWork(int seqLength, char* fileName, int maxSize);
static void signalHandlerParent(int sigNo);
static void signalHandlerChild(int sigNo);
ssize_t r_write(int fd, void *buf, size_t size);
ssize_t r_read(int fd, void *buf, size_t size);


/**** GLOBALS ****/
int fileDescriptor;
int logDescriptor;
int* rngArray;

int main(int argc, char** argv){

    pid_t childpid;

    //Usage control
    if (argc != 7) {
        fprintf(stderr, "Usage : %s -N sequenceAmount -X filename -M maxSizeForFile \n", argv[0]);
        exit(1);
    }

    if ((childpid = fork()) == -1) {
        fprintf(stderr, "Error occourred while using fork\n");
        exit(1);
    }
    else if (childpid == 0) {	//Child process
        childWork(atoi(argv[2]), argv[4], atoi(argv[6]));
    }
    else {	//Parent process
        parentWork(atoi(argv[2]), argv[4], atoi(argv[6]));
    }
    return 0;
}


void parentWork(int seqLength, char* fileName, int maxSize) {
    struct sigaction signalActionParent;
    struct stat fileStat;
    int lineCount = 0;
    int i = 0;
    char tempBuff[256];

    //Signal handler
    signalActionParent.sa_handler = signalHandlerParent;
    signalActionParent.sa_flags = 0;
    if ((sigemptyset(&signalActionParent.sa_mask) == -1) || (sigaction(SIGINT, &signalActionParent, NULL) == -1)) {
        fprintf(stderr, "Failed to set SIGINT handler for Process A");
        exit(1);
    }

    rngArray = (int*)malloc(sizeof(int)* seqLength);

    //Random generation seed
    srand(getpid());


    if((logDescriptor = open(PROCESSA_FILENAME,WRITE_FLAGS,WRITE_PERMS)) == -1){
        fprintf(stderr,"Process A failed to open its log file\n");
        exit(1);
    }

    //Work loop
    for (;;) {

        if ((fileDescriptor = open(fileName, WRITE_FLAGS, WRITE_PERMS)) == -1) {
            fprintf(stderr, "Process A failed to open %s\n", fileName);
            exit(1);
        }

        //File Lock
        flock(fileDescriptor, LOCK_EX);

        //Get File line Size
        fstat(fileDescriptor,&fileStat);
        if (fileStat.st_size != 0) {
            lineCount = fileStat.st_size / (sizeof(int)*seqLength);
        }
        else {
            lineCount = 0;
        }

        if (lineCount+1 <= maxSize) {
            //Generate Random numbers in to the array
            printf("Process A: Producing Random Sequence For Line %d -> ", lineCount + 1);

            while (i < seqLength) {
                rngArray[i] = rand() % RNGLIMIT;
                printf("%d ", rngArray[i]);
                ++i;
            }
            i = 0;
            printf("\n");

            //Writes to ProcessA log
            sprintf(tempBuff,"Line %d : ",lineCount+1);
            r_write(logDescriptor,tempBuff, strlen(tempBuff));
            while(i<seqLength){
                sprintf(tempBuff,"%d ",rngArray[i]);
                r_write(logDescriptor,tempBuff, strlen(tempBuff));
                ++i;
            }
            i=0;
            sprintf(tempBuff,"\n");
            r_write(logDescriptor,tempBuff, strlen(tempBuff));


            r_write(fileDescriptor, rngArray, sizeof(int)*seqLength);

        }

        //Unlock file
        flock(fileDescriptor, LOCK_UN);
        close(fileDescriptor);

    }


}

void childWork(int seqLength, char* fileName, int maxSize) {
    struct sigaction signalActionChild;
    struct stat fileStat;
    int lineCount = 0;
    int i = 0;
    char tempBuff[256];

    //Signal handler
    signalActionChild.sa_handler = signalHandlerChild;
    signalActionChild.sa_flags = 0;
    if ((sigemptyset(&signalActionChild.sa_mask) == -1) || (sigaction(SIGINT, &signalActionChild, NULL) == -1)) {
        fprintf(stderr, "Failed to set SIGINT handler for Process B");
        exit(1);
    }

    rngArray = (int*)malloc(sizeof(int)* seqLength);

    if((logDescriptor = open(PROCESSB_FILENAME,WRITE_FLAGS,WRITE_PERMS)) == -1){
        fprintf(stderr,"Process B failed to open its log file\n");
        exit(1);
    }

    //Work Loop
    for (;;) {

        if ((fileDescriptor = open(fileName, O_RDWR, WRITE_PERMS)) == -1) {
            fprintf(stderr, "Process B failed to open %s\n", fileName);
            exit(1);
        }

        //File Lock
        flock(fileDescriptor, LOCK_EX);

        //Get file line size
        fstat(fileDescriptor,&fileStat);
        if (fileStat.st_size != 0) {
            lineCount = fileStat.st_size / (sizeof(int)*seqLength);
        }
        else {
            lineCount = 0;
        }

        if (lineCount != 0) {

            //Get to last input line
            lseek(fileDescriptor, sizeof(int)*seqLength*(lineCount - 1), SEEK_SET);

            //Read from file
            r_read(fileDescriptor, rngArray, sizeof(int)*seqLength);

            //Print to screen
            printf("Process B: Read sequence for line %d -> ", lineCount);
            while (i < seqLength) {
                printf("%d ", rngArray[i]);
                ++i;
            }
            i = 0;
            printf("\n");

            //Print to ProcessB log file
            sprintf(tempBuff,"Line %d : ",lineCount);
            r_write(logDescriptor,tempBuff, strlen(tempBuff));
            while(i<seqLength){
                sprintf(tempBuff,"%d ",rngArray[i]);
                r_write(logDescriptor,tempBuff, strlen(tempBuff));
                ++i;
            }
            i=0;
            sprintf(tempBuff,"\n");
            r_write(logDescriptor,tempBuff, strlen(tempBuff));

            //Shorten file, in other words delete last line
            if((ftruncate(fileDescriptor, sizeof(int)*seqLength*(lineCount - 1))) == -1 ){
                fprintf(stderr," Error occurred during ftruncate \n");
                exit(1);
            }

        }

        //Unlock file
        flock(fileDescriptor, LOCK_UN);
        close(fileDescriptor);

    }


}

//Signal handler for Process A (aka parent process)
static void signalHandlerParent(int sigNo) {
    int status = 0;
    puts("\nSIGINT signal caught, exiting Process A\n");

    free(rngArray);
    wait(&status);
    close(fileDescriptor);
    close(logDescriptor);
    exit(1);
}

//Signal handler for Process B (aka child process)
static void signalHandlerChild(int sigNo) {
    puts("\nSIGINT signal caught, exiting Process B\n");
    free(rngArray);
    close(fileDescriptor);
    close(logDescriptor);
    exit(1);
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
    while (retval = read(fd, buf, size), retval == -1 && errno == EINTR);
    return retval;
}

















