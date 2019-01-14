/***** INCLUDES *****/
#include <stdio.h>
#include <semaphore.h>
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
#include <sys/mman.h>



/***** MACROS *****/
#define WRITE_FLAGS (O_WRONLY | O_APPEND | O_CREAT)
#define WRITE_PERMS (S_IRUSR | S_IWUSR )
#define RNGLIMIT 50 //Generation limit
#define EXIT_SUCC 0
#define EXIT_ERR 1
#define true 1
#define false 0

/**** PROTOTYPES ****/
void chef(int ing1, int ing2, sem_t *sema, char* market, sem_t* dessert);
void wholesaler(sem_t * sema, char* market, sem_t *dessert);
static void sigINTHandler(int sigNo);


/**** GLOBALS ****/
int childCount = 0;
int signalCaught = 0;




typedef enum ingredients {EGG = 0, FLOUR, BUTTER, SUGAR} ing_t;
char ingredients[4][7] = {"egg","flour","butter", "sugar"};

int main(int argc, char** argv){

	pid_t childpid = 0;
	pid_t childArr[50];
	int rngIngridient1  = 0 ;
	int rngIngridient2 = 0;
	size_t sharedSize1;


	if(argc < 2){
		fprintf(stderr, "Usage : %s NumberOfChefs\n", argv[0] );
		return EXIT_ERR;
	}
	if(atoi(argv[1]) <6 || atoi(argv[1]) > 50){
		fprintf(stderr, "NumberOfChefs must be greater than 5 and smaller than 51\n" );
	}

	srand(time(NULL));

	//Signal handle
    struct sigaction sigInt; 
    sigInt.sa_handler= &sigINTHandler;
    sigInt.sa_flags = 0;

    if( (sigemptyset(&sigInt.sa_mask)== -1) || (sigaction(SIGINT, &sigInt, NULL) == -1 ) || (sigaction(SIGTERM, &sigInt, NULL) == -1)) {
        perror("Failed to install Signal handlers");
        exit(-1);
    }



	sem_t *testSem = mmap(NULL, sizeof(*testSem), PROT_READ |PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1, 0);
	if(testSem == MAP_FAILED){
		fprintf(stderr, "failed to mmap\n" );
		return EXIT_ERR;
	}

	char *market = mmap(NULL, sizeof(char)*2, PROT_READ |PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1, 0);
	if(market == MAP_FAILED){
		fprintf(stderr, "failed to mmap\n" );
		return EXIT_ERR;
	}

	sem_t *dessertReady = mmap(NULL, sizeof(*dessertReady), PROT_READ |PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1, 0);
	if(dessertReady == MAP_FAILED){
		fprintf(stderr, "failed to mmap\n" );
		return EXIT_ERR;
	}


	if(sem_init(testSem,1,0) == -1){
		fprintf(stderr, "sem_init failed\n");
		return EXIT_ERR;
	}

	if(sem_init(dessertReady,1,0) == -1){
		fprintf(stderr, "sem_init failed\n");
		return EXIT_ERR;
	}


	

	while(childCount < atoi(argv[1])){
		childpid = fork();
		if(childpid == -1){
			fprintf(stderr, "Failed to Fork\n" );
			return EXIT_ERR;
		}else if (childpid == 0){ // Child
				if(childCount == 0){
					rngIngridient1 = 0;
					rngIngridient2 = 1 ;
				}else if (childCount == 1){
					rngIngridient1 = 0;
					rngIngridient2 = 2;
				}else if(childCount == 2){
					rngIngridient1 = 0;
					rngIngridient2 = 3;
				}else if(childCount == 3){
					rngIngridient1 = 1;
					rngIngridient2 = 2;
				}else if(childCount == 4){
					rngIngridient1 = 1;
					rngIngridient2 = 3;
				}else if(childCount == 5){
					rngIngridient1 = 2;
					rngIngridient2 = 3;
				}else{
					rngIngridient1 = rand()%4;
					while(rngIngridient2 == rngIngridient1){
						rngIngridient2 = rand()%4;
					}
				}

				chef(rngIngridient1,rngIngridient2, testSem, market, dessertReady);

				exit(EXIT_SUCC);
		} else{ // Parent
			
			++childCount;
			childArr[childCount] = childpid;
		}
	}

	wholesaler(testSem, market,dessertReady);



    return EXIT_SUCC;
}



void chef(int ing1, int ing2, sem_t* sema, char* market, sem_t * dessert){


	int reqIng1 = 0 , reqIng2 = 0;
	int i = 0;


	for(i=0; i<4; ++i){
		if(i != ing1 && i != ing2 ){
			reqIng1 = i;
		}
	}

	for(i = 0; i< 4; ++i){
		if(i != ing1 && i!= ing2 && i != reqIng1){
			reqIng2 = i;
		}
	}

	printf("Chef%d's current ingredients %s and %s. Needed ingredients to make Sekerpare -> %s and %s\n",
		childCount+1,ingredients[ing1], ingredients[ing2], ingredients[reqIng1], ingredients[reqIng2] );


	while(!signalCaught){

		if(sem_wait(sema) == -1 ){
			fprintf(stderr, "sem_wait failed on Chef%d\n", childCount );
			_exit(EXIT_ERR);
		}

		//Check for ingredients 
		if( (int)market[0] == reqIng1 || (int)market[0] == reqIng2){
			if((int)market[1] == reqIng1 || (int)market[1] == reqIng2){
				//Required Ingredients
				printf("Chef%d has taken %s and %s\n", childCount+1 ,ingredients[reqIng1], ingredients[reqIng2]);
				printf("Chef%d has delivered the dessert to the wholesaler\n", childCount+1);
				if(sem_post(dessert) == -1 ){
					fprintf(stderr, "sem_post failed on Chef%d\n",childCount+1 );
					_exit(EXIT_ERR);
				}
				printf("Chef%d is waiting for %s and %s\n", childCount+1 ,ingredients[reqIng1], ingredients[reqIng2]);

			}else{//Wrong ingredients. Let another chef check the ingredients
				if(sem_post(sema) == -1 ){
					fprintf(stderr, "sem_post failed on Chef%d\n",childCount+1 );
					_exit(EXIT_ERR);
				}
			}
		}else{//Wrong ingredients. Let another chef check the ingredients
			if(sem_post(sema) == -1 ){
				fprintf(stderr, "sem_post failed on Chef%d\n",childCount+1 );
				_exit(EXIT_ERR);
			}
		}
	}

	fprintf(stderr,"\nSIGINT signal caught, exiting Chef%d\n",childCount+1);
    	
	if(sem_destroy(sema) < 0) {
        perror("sem_destroy failed");
        exit(EXIT_FAILURE);
    }
    if(munmap(sema, sizeof(sema)) < 0) {
        perror("munmap failed");
        exit(EXIT_FAILURE);
    }
	if(sem_destroy(dessert) < 0) {
        perror("sem_destroy failed");
        exit(EXIT_FAILURE);
    }
    if(munmap(dessert, sizeof(dessert)) < 0) {
        perror("munmap failed");
        exit(EXIT_FAILURE);
    }
    if(munmap(market, sizeof(char)*2) < 0) {
        perror("munmap failed");
        exit(EXIT_FAILURE);
    }

	_exit(EXIT_ERR);

}


void wholesaler(sem_t* sema, char* market, sem_t *dessert){

	int newIng1 = 0;
	int newIng2 = 0;

	while(!signalCaught){

		//Generate random ingredients

		newIng1 = rand()%4;
		while(newIng1 == newIng2){
			newIng2 = rand()%4;
		}

		market[0] = newIng1;
		market[1] = newIng2;

		printf("Wholesaler came to market with ingredients %s - %s\n",ingredients[(int)market[0]], ingredients[(int)market[1]] );
		printf("Wholesaler is waiting for Sekerpare\n");

		if(sem_post(sema)== -1){
			fprintf(stderr, "sem_post failed on Wholesaler\n");
			exit(EXIT_ERR);
		}


		if(sem_wait(dessert) == -1) {
			fprintf(stderr, "sem_wait failed on Wholesaler\n");
		}

		printf("Wholesaler has obtained the Sekerpare and left to sell it\n");

	}

	fprintf(stderr, "\nSIGINT signal caught, exiting Wholesaler\n");
    	while (waitpid(-1, NULL, 0))
            if (errno == ECHILD)
                      break;
    
	if(sem_destroy(sema) < 0) {
        perror("sem_destroy failed");
        exit(EXIT_FAILURE);
    }
    if(munmap(sema, sizeof(sema)) < 0) {
        perror("munmap failed");
        exit(EXIT_FAILURE);
    }
	if(sem_destroy(dessert) < 0) {
        perror("sem_destroy failed");
        exit(EXIT_FAILURE);
    }
    if(munmap(dessert, sizeof(dessert)) < 0) {
        perror("munmap failed");
        exit(EXIT_FAILURE);
    }
    if(munmap(market, sizeof(char)*2) < 0) {
        perror("munmap failed");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_ERR);

}


static void sigINTHandler(int sigNo) {
    signalCaught = 1;
}