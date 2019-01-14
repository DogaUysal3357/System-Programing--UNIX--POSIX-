/*
Missing Parts ->  "|" and "<".

This shell can not use link more than one time. 
In other words you can't use something like this -> " ls > temp.txt | wc temp.txt"

*/



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

/**** MACROS ****/
#define MAX_PATH 255
#define READ_FLAGS O_RDONLY
#define WRITE_FLAGS (O_WRONLY | O_CREAT)
#define WRITE_PERMS (S_IRUSR | S_IWUSR )
#define BLKSIZE 256
#define EXIT_SUCCESSFUL 0
#define EXIT_UNSUCCESSFUL 1


/**** PROTOTYPES ****/
int takeInput(char* entry);
void addToHistory(char* buff);
void history(int fd);
void help(int fd);
void processInput(char* entry, char** parsedSection);
ssize_t r_write(int fd, void *buf, size_t size);
static void signalHandlerParent(int sigNo); 
void freeHistory();

/**** GLOBALS ****/
char** prevEntries;
int currPrevEntrySize = 256;
int historyNum;


int main(int argc, char** argv){

	
	char currEntry[BLKSIZE];
	char tempEntry[BLKSIZE];
	char currentPath[MAX_PATH];
	char rootPath[MAX_PATH];
	char execPathLS[MAX_PATH];
	char execPathCAT[MAX_PATH];
	char execPathWC[MAX_PATH];
	int i = 0;
	int entryCheck;
	char* parsedSection[5];
	int fileDescriptor;
	char commHelp[] = "help";
	char commLs[] = "ls";
	pid_t pid = 0 ;
	int status = 0;
	struct sigaction signalActionParent;

	if(argc > 1){
		fprintf(stderr, "Usage : %s\n",argv[0]);
		return EXIT_UNSUCCESSFUL;
	}

 	//Signal handler
    signalActionParent.sa_handler = signalHandlerParent;
    signalActionParent.sa_flags = 0;
    if ((sigemptyset(&signalActionParent.sa_mask) == -1) || (sigaction(SIGINT, &signalActionParent, NULL) == -1)) {
        fprintf(stderr, "Failed to set SIGINT handler for Process A");
        exit(1);
    }

	//init history
	prevEntries = (char**) malloc(BLKSIZE * sizeof(char*));
	for(i=0; i< BLKSIZE; ++i){
		prevEntries[i] = (char*)malloc(BLKSIZE* sizeof(char));
	}


	//Working Directory
    getcwd(currentPath, sizeof(currentPath));
    strcpy(rootPath,currentPath);


    sprintf(execPathLS,"%s/ls", rootPath);
	sprintf(execPathCAT,"%s/cat", rootPath);
    sprintf(execPathWC,"%s/wc", rootPath);


	/*Shell Loop*/
	while(1){

		//Take Input from the User
		entryCheck = takeInput(currEntry);
		strcpy(tempEntry, currEntry);

		if(entryCheck == 1){

			for(i = 0; i<5; ++i){
				parsedSection[i] = NULL;
			}

			processInput(currEntry, parsedSection);

			// cd command
			if(strcmp(parsedSection[0],"cd") == 0){
				if(chdir(parsedSection[1]) == -1){
					fprintf(stderr, "No Such file or directory -> %s\n",parsedSection[1] );
				}
				getcwd(currentPath, sizeof(currentPath));
			}

			// help command
			else if(strcmp(parsedSection[0],"help") == 0){
				if(parsedSection[1] == NULL){
					help(STDOUT_FILENO);
				}else if (strcmp(parsedSection[1],">") == 0){
					if((fileDescriptor = open(parsedSection[2],WRITE_FLAGS,WRITE_PERMS)) == -1){
						fprintf(stderr, "Failed to open file %s\n", parsedSection[2] );
						continue;
					}
					help(fileDescriptor);
					close(fileDescriptor);

				}else{
					fprintf(stderr,"Given command is not supperted %s \n", tempEntry );
				}
			}


			//pwd command
			else if(strcmp(parsedSection[0], "pwd") == 0){
				if(parsedSection[1]==NULL){
					printf("%s\n",currentPath);
				}
				else if(strcmp(parsedSection[1], ">") == 0){
					if((fileDescriptor = open(parsedSection[2],WRITE_FLAGS,WRITE_PERMS)) == -1){
						fprintf(stderr, "Failed to open file %s\n", parsedSection[2] );
						continue;
					}
					r_write(fileDescriptor, currentPath, strlen(currentPath));
					close(fileDescriptor);
				}else{
					fprintf(stderr,"Given command is not supperted %s \n", tempEntry );
				}
			}

			// exit command
			else if(strcmp(parsedSection[0],"exit") == 0){
				for(i=0; i<currPrevEntrySize; ++i){
					free(prevEntries[i]);
				}
				return EXIT_SUCCESSFUL;
			}

			//ls command
			else if(strcmp(parsedSection[0],"ls") == 0){				
				if((pid = fork()) == -1){
					fprintf(stderr, "Failed to fork\n" );
				}else if (pid == 0){
					freeHistory();
					if(parsedSection[1] == NULL){
						execv(execPathLS,(char* []){parsedSection[0], NULL});
						fprintf(stderr, "Failed to execv for ls.\n");
					}
					else if(strcmp(parsedSection[1],">") == 0){
						execv(execPathLS, (char*[]){parsedSection[0],parsedSection[1],parsedSection[2],NULL});
						fprintf(stderr, "Failed to execv for ls.\n");
					}else{
					fprintf(stderr,"Given command is not supperted for ls %s \n", tempEntry );
					}
				}else{
					continue;
				}
			}

			//cat command
			else if(strcmp(parsedSection[0],"cat") == 0){
				if((pid = fork()) == -1){
					fprintf(stderr, "Failed to fork\n" );
				}else if (pid == 0){
					freeHistory();
					if(parsedSection[2] == NULL){
						execv(execPathCAT,(char*[]){parsedSection[0],parsedSection[1],NULL});
						fprintf(stderr, "Failed to execv for cat.\n");
					}
					else if(strcmp(parsedSection[2],">") == 0){
						execv(execPathCAT,(char*[]){parsedSection[0],parsedSection[1],parsedSection[2],parsedSection[3],NULL});
						fprintf(stderr, "Failed to execv for cat.\n");
					}else{
					fprintf(stderr,"Given command is not supperted %s \n", tempEntry );
					}
				}else{
					continue;
				}
			}

			//wc command
			else if(strcmp(parsedSection[0],"wc") == 0){
				if((pid = fork()) == -1){
					fprintf(stderr, "Failed to fork\n" );
				}else if (pid == 0){
					freeHistory();
					if(parsedSection[2] == NULL){
						execv(execPathWC,(char*[]){parsedSection[0],parsedSection[1],NULL});
						fprintf(stderr, "Failed to execv for wc.\n");
					}
					else if(strcmp(parsedSection[2],">") == 0){
						execv(execPathWC,(char*[]){parsedSection[0],parsedSection[1],parsedSection[2],parsedSection[3],NULL});
						fprintf(stderr, "Failed to execv for wc.\n");
					}else{
					fprintf(stderr,"Given command is not supperted %s \n", tempEntry );
					}
				}else{
					continue;
				}
			}

			else if (strcmp(parsedSection[0],"history") == 0){
				if(parsedSection[1] == NULL){
					history(STDOUT_FILENO);
				}else if (strcmp(parsedSection[1],">") == 0){
					if((fileDescriptor = open(parsedSection[2],WRITE_FLAGS,WRITE_PERMS)) == -1){
						fprintf(stderr, "Failed to open file %s\n", parsedSection[2] );
						continue;
					}
					history(fileDescriptor);
					close(fileDescriptor);
				}else{
					fprintf(stderr,"Given command is not supperted %s \n", tempEntry );
				}

			}




			else{
				fprintf(stderr, "Unknown command -> %s\n", tempEntry );
			}

		}

		waitpid(-1, &status, WNOHANG);		

	}




}

int takeInput(char* entry){

	char buf[BLKSIZE];
	int historyTries = 0;
	
	fprintf(stderr,"\n~~ ");
	fgets(buf,BLKSIZE, stdin);


	if(strlen(buf) != 0){
		addToHistory(buf);
		strcpy(entry,buf);

		return 1;
	}
	return 0;


}

void addToHistory(char* buff){
	int i = 0;

	strcpy(prevEntries[++historyNum], buff);

	//No more room, increase history size
	if(historyNum == currPrevEntrySize -1){
		currPrevEntrySize += currPrevEntrySize;
		prevEntries = (char**) realloc(prevEntries,currPrevEntrySize);
		for(i=historyNum; i<currPrevEntrySize; ++i){
			prevEntries[i] = (char*) malloc(BLKSIZE * sizeof(char));
		}
	}
}

void history(int fd){
	int i = 0;
	for(i=0; i<historyNum ; ++i){
		r_write(fd, prevEntries[i], strlen(prevEntries[i]));
	}
}

void help(int fd){
	char helper[BLKSIZE];

	sprintf(helper,"\n****HELP****\n"
		"List Of Commands Supperted\n"
		"~ls\n"
		"~pwd\n"
		"~cat\n"
		"~wc\n"
		"~history\n"
		"~cd\n"
		"~exit\n\0");

	r_write(fd,helper,strlen(helper));
}


void processInput(char* entry, char** parsedSection){
	int i = 0;
	int flag = 1;
	char temp[BLKSIZE];

	parsedSection[0] = strtok(entry, " \n");
	for(i = 1; i < 5 ; ++i){

		parsedSection[i] = strtok(NULL, " \n");

		if(parsedSection[i] == NULL){
			break;
		}
		if(strlen(parsedSection[i]) == 0){
			i--;
		}
	}


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

//Signal handler for Shell
static void signalHandlerParent(int sigNo) {
    puts("\nSIGINT signal caught, exiting shell\n");
    freeHistory();
    
    exit(1);
}

void freeHistory(){
	int i;

	for(i=0; i<currPrevEntrySize+1; ++i){
		free(prevEntries[i]);
	}
	free(prevEntries);

}
