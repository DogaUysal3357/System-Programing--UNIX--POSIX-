#include "server.h"

/**** GLOBALS ****/
int numOfProviders;
int logFileFD;	// File descriptor for logFile
pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;	// Mutex for printing log file.
pthread_mutex_t handle_mutex = PTHREAD_MUTEX_INITIALIZER;	// Mutex for provider search.
provider_t* providers;	//Dynamic provider array.
int serversocket;	// Main server's socket fd
client_socket_t* clientFDs;	//Dynamic client socket fd array
int totalClients = 0;	//

/**** MAIN ****/
int main(int argc, char** argv){

	unsigned short portNum;
	int i = 0;
	int s,t;
	pthread_t threadId;
	struct sigaction signalActionServer;
	char printBuf[MAX_SIZE];

	//Usage Control
	if(argc != 4){
		fprintf(stderr, "Usage : %s ConnectionPort ProviderFile LogFileName\n", argv[0] );
		return EXIT_ERR;
	}

	//Signal handler
    signalActionServer.sa_handler = signalHandlerServer;
    signalActionServer.sa_flags = 0;
    if ((sigemptyset(&signalActionServer.sa_mask) == -1) || (sigaction(SIGINT, &signalActionServer, NULL) == -1)) {
        fprintf(stderr, "Failed to set SIGINT handler for Process A");
        exit(1);
    }

    //Rng init.
	srand(time(NULL));

	logFileFD =	createLogFile(argv[3]);

	clientFDs = (client_socket_t *)malloc(sizeof(client_socket_t));

	//Read File and Create threads
	readFile(argv[2]);

	sprintf(printBuf, "%d provider threads created\n",numOfProviders );
	serverPrint(printBuf);


	sprintf(printBuf,"Name \t Performance \t Price \t Duration\n");
	serverPrint(printBuf);

	for(i=0; i<numOfProviders; ++i){
		sprintf(printBuf, "%s \t %d \t\t %d \t %d\n",providers[i].name, providers[i].performance, providers[i].price, providers[i].duration);
		serverPrint(printBuf);
	}

	//Create providers
	for(i=0; i<numOfProviders; ++i){
		if( pthread_create(&providers[i].id, NULL, provider, (void*)i) !=  0){
	    		fprintf(stderr, "Failed to pthread_create\n");
	    		raise(SIGINT);
		}	
	}

	//Server connection establish
	if( (serversocket= establish(atoi(argv[1])) ) < 0){
		fprintf(stderr, "Establish Error \n");
		raise(SIGINT);
	}

	sprintf(printBuf, "Server is waiting for client connections at port 5555\n");
	serverPrint(printBuf);

	for(;;){
		//Wait for clients to connect
		if ((t= getConnection(serversocket)) < 0) { 
			if (errno == EINTR) 
				continue; 
			fprintf(stderr,"getConnection failed\n"); 
			raise(SIGINT);
		}

		//Someone is connected, 
		totalClients ++;
		clientFDs = (client_socket_t *) realloc(clientFDs,sizeof(client_socket_t) * totalClients);
		clientFDs[totalClients-1].clientSocketFd = t;
		clientFDs[totalClients-1].closed = 0;

		//Create thread to handleClient
 		if( pthread_create(&threadId, NULL, handleClient, (void*)totalClients) !=  0){
    		fprintf(stderr, "Failed to pthread_create\n");
	    	raise(SIGINT);	
		}
	}

	return EXIT_SUCC;
}

//Takes index for clientFDs array as a parameter. Searches providers array for available provider.
// Waits for providers jobs tobe done. 
void* handleClient(void* arg){
	int clientIndex = ((int)arg)-1;
	int socketfd = clientFDs[clientIndex].clientSocketFd;
	char buff[MAX_SIZE];
	int providerID = 0;
	int i;
	int task;
	char desire;
	char name[MAX_SIZE];
	char printBuf[MAX_SIZE];
	char returnBuff[MAX_SIZE];
	int err;
	
	//Take message
	while((err = read(socketfd,buff, MAX_SIZE)) <= 0 ){
		if(err < 0){
			perror("err-> ");
			raise(SIGINT);
		}
	}

	sscanf(buff,"%s %c %d", name, &desire, &task);

	//lock mutex for searching provider
	if(pthread_mutex_lock(&handle_mutex)){
		fprintf(stderr, "Mutex failed to lock\n" );
		raise(SIGINT);
	}	

	//search for provider
	if(desire == 'Q'){
		for(i = 0; i < numOfProviders; ++i){
			if(providers[i].performance > providers[providerID].performance &&
				providers[i].inQueue != 2 && providers[i].logOut == 0){
				providerID = i;
			}
		}
	}else if(desire == 'T'){
		for(i = 0; i < numOfProviders; ++i){
			if(providers[i].inQueue < providers[providerID].inQueue &&
				providers[i].inQueue != 2 && providers[i].logOut == 0){
				providerID = i;
			}
		}
	}else if(desire == 'C'){
		for(i = 0; i < numOfProviders; ++i){
			if(providers[i].price < providers[providerID].price &&
				providers[i].inQueue != 2 && providers[i].logOut == 0){
				providerID = i;
			}
		}
	}

	if(providers[providerID].inQueue == 2 || providers[providerID].logOut == 1){
		//Send message to client, there's no Provider available

		sprintf(returnBuff, "NO PROVIDER AVAILABLE\0\n");
		if( (i = write(socketfd, returnBuff, strlen(returnBuff)+1)) <0 ){
			fprintf(stderr, "Write Failed. \n");
			raise(SIGINT);
		}

		if(pthread_mutex_unlock(&handle_mutex)){
			fprintf(stderr, "Mutex failed to unlock\n" );
			raise(SIGINT);		
		}

		clientFDs[clientIndex].closed = 1;
		close(socketfd);
		pthread_exit(EXIT_SUCC);
	}

	//update provider information
	if(pthread_mutex_lock(&providers[providerID].mutex_t.mutex)){
		fprintf(stderr, "Mutex failed to lock\n" );
		raise(SIGINT);
	}	

	providers[providerID].inQueue ++;
	providers[providerID].tracker = (providers[providerID].tracker +1 ) %2;
	providers[providerID].clientfd[providers[providerID].tracker] = socketfd;
	providers[providerID].calc[providers[providerID].tracker] = task;

	sprintf(printBuf,"Client %s (%c %d) coonnected, forwarded to provider %s\n",name,desire,task, providers[providerID].name);
	serverPrint(printBuf);

	if(pthread_mutex_unlock(&providers[providerID].mutex_t.mutex)){
		fprintf(stderr, "Mutex failed to unlock\n" );
		raise(SIGINT);
	}

	//Send signal to provider, that a client is in the queue
	if(pthread_cond_signal(&providers[providerID].mutex_t.cond)){
		fprintf(stderr, "Condition Signal Failed\n");
		raise(SIGINT);
	}

	if(pthread_mutex_unlock(&handle_mutex)){
		fprintf(stderr, "Mutex failed to unlock\n" );
		raise(SIGINT);
	}

	
	//Wait for provider to finish
	if(pthread_mutex_lock(&providers[providerID].done_mutex[providers[providerID].tracker].mutex)){
		fprintf(stderr, "Mutex failed to lock\n" );
		raise(SIGINT);
	}

	pthread_cond_wait(&providers[providerID].done_mutex[providers[providerID].tracker].cond,
		 &providers[providerID].done_mutex[providers[providerID].tracker].mutex);

	if(pthread_mutex_unlock(&providers[providerID].done_mutex[providers[providerID].tracker].mutex)){
		fprintf(stderr, "Mutex failed to unlock\n" );
		raise(SIGINT);
	}


	//Close socket
	clientFDs[clientIndex].closed = 1;
	close(socketfd); 

	pthread_exit(EXIT_SUCC);
}


void* provider(void* arg){
	int providerNum = (int)arg;
	int totalCompletedTasks = 0;
	int i;
	double result;
	char printBuf[MAX_SIZE];
	int tracker;
	struct timeval start_time, end_time, creation_time;
	double workTime;
	char returnBuff[MAX_SIZE];

	gettimeofday(&creation_time, NULL);

	while(true){

		//Duration timer
		gettimeofday(&end_time,NULL);
		if(((double)end_time.tv_sec - (double)creation_time.tv_sec ) > providers[providerNum].duration){
			providers[providerNum].logOut = 1;
			pthread_exit(&totalCompletedTasks);
		}

		if(pthread_mutex_lock(&providers[providerNum].mutex_t.mutex)){
			fprintf(stderr, "Mutex failed to lock\n" );
			raise(SIGINT);
			
		}	

		//Wait for Client
		while(providers[providerNum].inQueue == 0){
			sprintf(printBuf, "Provider %s waiting for tasks\n", providers[providerNum].name );
			serverPrint(printBuf);
			pthread_cond_wait(&providers[providerNum].mutex_t.cond, &providers[providerNum].mutex_t.mutex);
		}

		tracker = providers[providerNum].tracker;
		if(providers[providerNum].inQueue == 2){
			tracker = (tracker +1)%2;
		}

		//Client found, do the job
		sprintf(printBuf, "Provider %s is processing task number %d : %d\n", providers[providerNum].name, totalCompletedTasks+1, providers[providerNum].calc[tracker] );
		serverPrint(printBuf);

		//Get timer
		gettimeofday(&start_time, NULL);

		result = cos(providers[providerNum].calc[tracker] * M_PI / 180.0);

		sleep(rand()%10+5);

		gettimeofday(&end_time,NULL);

		workTime = (double)(end_time.tv_usec - start_time.tv_usec) / 1000000 + (double)(end_time.tv_sec - start_time.tv_sec);

		sprintf(printBuf, "Provider %s completed task number %d: cos(%d) = %.2f in %.2f seconds.\n", providers[providerNum].name, totalCompletedTasks+1, providers[providerNum].calc[tracker], result, workTime );
		serverPrint(printBuf);

		totalCompletedTasks++;
		providers[providerNum].clientsServed++;
		providers[providerNum].inQueue--;

		//Give results to client 
		sprintf(returnBuff,"%s %.2f %.2f %d\0", providers[providerNum].name, workTime, result, providers[providerNum].price);

		if( (i = write(providers[providerNum].clientfd[tracker], returnBuff, strlen(returnBuff)+1)) <0 ){
			fprintf(stderr, "Write Failed.\n");
			perror("Error Cause : ");
			raise(SIGINT);
		}

		//Send signal to handler, clients job is done
		if(pthread_cond_signal(&providers[providerNum].done_mutex[tracker].cond)){
			fprintf(stderr, "Condition Signal Failed\n");
			raise(SIGINT);
		}

		if(pthread_mutex_unlock(&providers[providerNum].mutex_t.mutex)){
			fprintf(stderr, "Mutex failed to unlock\n" );
			raise(SIGINT);
		}

	}
}

//Codes taken from Lecture 13 Slides
int getConnection(int s){
	int t; /* socket of connection */
	if ((t = accept(s,NULL,NULL)) < 0) /* accept connection if there is one */
		return(-1);
	return(t);
}

//Codes taken from Lecture 13 Slides
int establish ( unsigned short portnum){
	char myname[MAX_SIZE+1];
	int s;
	struct sockaddr_in sa;
	struct hostent *hp;
	
	memset(&sa, 0, sizeof(struct sockaddr_in)); /* clear our address */
	gethostname(myname, MAX_SIZE); /* who are we? */
	hp= gethostbyname(myname); /* get our address info */
	

	if (hp == NULL) {
		fprintf(stderr,"HP null\n");
		return(-1);
	}
	sa.sin_family= hp->h_addrtype; /* this is our host address */
	sa.sin_port= htons(portnum); /* this is our port number */
	if ((s= socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Socket error\n" );
		return(-1);
	}
	if (bind(s,(struct sockaddr *)&sa,sizeof(struct sockaddr_in)) < 0) {
		perror("bind error " );
		close(s);
		return(-1); /* bind address to socket */
	}
	listen(s, MAX_QUEUE); /* max # of queued connects */
	return(s);
}

//Read given data file and creates providers accordingly and dynamically.
void readFile(char* fileName){
	char buff[MAX_SIZE-1];
	int i = 0 ;
	int fileDescriptor;

	if ((fileDescriptor = open(fileName, READ_FLAGS)) == -1) {
        fprintf(stderr, "Failed to open %s\n", fileName);
        exit(1);
    }
    
    providers = (provider_t*)malloc(sizeof(provider_t));

    while(readline(fileDescriptor, buff, MAX_SIZE) != 0){
    	if(i==0){
			++i;
    		continue;
    	} 
    	if(strlen(buff) > 5){
    		providers = (provider_t *) realloc(providers,sizeof(provider_t) * i);

    		sscanf(buff,"%s %d %d %d", &providers[i-1].name, &providers[i-1].performance, &providers[i-1].price, &providers[i-1].duration);

    		providers[i-1].tracker = 0;
    		providers[i-1].inQueue = 0;
    		providers[i-1].logOut = 0;
    		providers[i-1].clientsServed = 0;

    		if(pthread_mutex_init(&providers[i-1].mutex_t.mutex, NULL) != 0){
    			fprintf(stderr, "Failed to mutex init\n" );
    		}
    		if(pthread_cond_init( &providers[i-1].mutex_t.cond,NULL) != 0){
    			fprintf(stderr, "Failed to cond init\n" );
    		} 
			if(pthread_mutex_init(&providers[i-1].done_mutex[0].mutex, NULL) != 0){
    			fprintf(stderr, "Failed to mutex init\n" );
    		}
    		if(pthread_cond_init( &providers[i-1].done_mutex[0].cond,NULL) != 0){
    			fprintf(stderr, "Failed to cond init\n" );
    		} 
			if(pthread_mutex_init(&providers[i-1].done_mutex[1].mutex, NULL) != 0){
    			fprintf(stderr, "Failed to mutex init\n" );
    		}
    		if(pthread_cond_init( &providers[i-1].done_mutex[1].cond,NULL) != 0){
    			fprintf(stderr, "Failed to cond init\n" );
    		} 

    	}
	++i;    
    }

	numOfProviders = i-1;    
}

int createLogFile(char* fileName){
	int logDescriptor;
	char tempBuf[MAX_SIZE];

	if((logDescriptor = open(fileName,WRITE_FLAGS,WRITE_PERMS)) == -1){
        fprintf(stderr,"Failed to create log file %s\n", fileName);
        exit(1);
    }

    fprintf(stderr, "Logs kept at %s\n", fileName );

    return logDescriptor;
}

/* Code Taken From UNIX SYSTEMS PRGRAMMING Book */
int readline(int fd, char *buf, int nbytes) {
   int numread = 0;
   int returnval;

   while (numread < nbytes - 1) {
      returnval = read(fd, buf + numread, 1);
      if ((returnval == -1) && (errno == EINTR))
         continue;
      if ((returnval == 0) && (numread == 0))
         return 0;
      if (returnval == 0)
         break;
      if (returnval == -1)
         return -1;
      numread++;
      if (buf[numread-1] == '\n') {
         buf[numread] = '\0';
         return numread;
      }  
   }   
   errno = EINVAL;
   return -1;
}

//Signal handler for Shell
static void signalHandlerServer(int sigNo) {
    int i = 0;

    char buff[MAX_SIZE] = "Termination signal received\n";
    puts("\nSERVER SHUTTING DOWN\n");

    write(logFileFD, buff, strlen(buff));

    sprintf(buff, "Terminating all clients\0");
    puts(buff);
    write(logFileFD,buff, strlen(buff));

    sprintf(buff,"SERVER SHUTTING DOWN\n\0");
    for(i=0; i<totalClients; ++i){
    	if(clientFDs[i].closed == 0){
    		write(clientFDs[i].clientSocketFd, buff, strlen(buff));
    		close(clientFDs[i].clientSocketFd);
    	}
    }

    sprintf(buff, "Terminating all providers\n");
    puts(buff);

    for(i=0; i<numOfProviders; ++i){
    	 pthread_mutex_destroy(&providers[i].mutex_t.mutex);
         pthread_cond_destroy(&providers[i].mutex_t.cond);
    	 pthread_mutex_destroy(&providers[i].done_mutex[0].mutex);
         pthread_cond_destroy(&providers[i].done_mutex[0].cond);
     	 pthread_mutex_destroy(&providers[i].done_mutex[1].mutex);
         pthread_cond_destroy(&providers[i].done_mutex[1].cond);
    }

    //Print all provider info
    sprintf(buff,"Statics\nName \t Number of clients served\n\0" );
    puts(buff);
    write(logFileFD,buff,strlen(buff));

    for(i=0; i<numOfProviders; ++i){
    	sprintf(buff,"%s \t %d\n\0", providers[i].name, providers[i].clientsServed);
    	puts(buff);
    	write(logFileFD,buff, strlen(buff));
    }

    //Close all provider mutex cond
    close(serversocket);
    free(providers);
    free(clientFDs);
    sprintf(buff,"Goodbye\n");
    puts(buff);
    write(logFileFD,buff,strlen(buff));

    close(logFileFD);

    exit(1);
}

// Simple print function. Prints given buffer both to screen and log File
void serverPrint(char* buf){
	if(pthread_mutex_lock(&logMutex)){
		fprintf(stderr, "Mutex failed to lock\n" );
		raise(SIGINT);
	}	

	fprintf(stderr,"%s", buf);

	write(logFileFD, buf, strlen(buf));


	if(pthread_mutex_unlock(&logMutex)){
		fprintf(stderr, "Mutex failed to lock\n" );
		raise(SIGINT);
	}	
}


