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
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <pthread.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

/***** MACROS *****/
#define RNGLIMIT 10 //Generation limit
#define EXIT_SUCC 0
#define EXIT_ERR 1
#define true 1
#define false 0
#define MAX_SIZE 256
#define MAX_QUEUE 100
#define READ_FLAGS O_RDONLY
#define WRITE_FLAGS (O_WRONLY | O_APPEND | O_CREAT | O_TRUNC )
#define WRITE_PERMS (S_IRUSR | S_IWUSR )

/***** STRUCTS *****/
typedef struct {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} provider_mutex_t;

typedef struct {
	char name[MAX_SIZE];
	int performance;
	int price;
	int duration;
	pthread_t id;
	int inQueue;		//How many clients are in the queue
	int tracker;		//tracker index for next client	
	int logOut;			//Duration run out
	int clientfd[2];	//socket fd's for clients that are in the queue
	int calc[2];		//Tasks that are assigned by the client
	provider_mutex_t mutex_t; 	//Mutex & cond for  provider_t struct variable protection.
	provider_mutex_t done_mutex[2];	//  Mutex & cond variables for signaling that provider's job is done with the current client.
	int clientsServed; // num of clients served.
} provider_t;

typedef struct{
	int clientSocketFd;  	//Client Socket id
	int closed;				//Is socket closed ? 
} client_socket_t;

/**** PROTOTYPES ****/
void readFile(char* fileName);
int readline(int fd, char *buf, int nbytes);
int createLogFile(char* fileName);
static void* handleClient(void *arg);
static void* provider(void *arg);
int getConnection(int s);
int establish ( unsigned short portnum);
static void signalHandlerServer(int sigNo);
void serverPrint(char* buf);