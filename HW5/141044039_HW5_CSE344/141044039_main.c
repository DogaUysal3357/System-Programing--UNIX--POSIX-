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
#include <pthread.h>
#include <math.h>

/***** MACROS *****/
#define RNGLIMIT 40 //Generation limit
#define EXIT_SUCC 0
#define EXIT_ERR 1
#define true 1
#define false 0
#define READ_FLAGS O_RDONLY
#define MAX_SIZE 256
#define MAX_QUEUE 100

typedef enum {
	orchid = 0, 
	rose = 1,
	violet = 2,
	clove = 3,
	daffodil = 4
} Flowers;


typedef struct {
	char name[MAX_SIZE];
	int x;
	int y;
	double tickRate;
	Flowers first;
	Flowers second;
	Flowers third;
} Florist_t;


typedef struct{
	char name[MAX_SIZE];
	int x;
	int y;
	Flowers product;
} Consumer_t;


typedef struct{
	int sale;
	int totalTime;
} SaleReport_t;

/**** PROTOTYPES ****/
double Distance(int x1, int y1, int x2, int y2);
int CalculateTimeToTravel(double distance, double tickRate);
int readline(int fd, char *buf, int nbytes);
void ProcessFlorist(char* buf, Florist_t* florist);
static void* firstFlorist(void *arg);
static void* secondFlorist(void *arg);
static void* thirdFlorist(void *arg);
void ProcessCustomer(char* buff, Consumer_t *customer);
int HasRequiredFlower(Flowers flower, Florist_t florist);
int SelectFlorist(Consumer_t customer, Florist_t f1, Florist_t f2, Florist_t f3);


/**** GLOBALS ****/
Consumer_t listForFirst[MAX_QUEUE];
Consumer_t listForSecond[MAX_QUEUE];
Consumer_t listForThird[MAX_QUEUE];
const char FLOWERS[][9] = {"orchid","rose","violet","clove","daffodil"};
int FF = 0;
int SF = 0;
int TF = 0;
static pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lock3 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond3 = PTHREAD_COND_INITIALIZER;

SaleReport_t sale1,sale2,sale3;
int done = 0;


int main(int argc, char** argv){

	int fileDescriptor;
	char buff[MAX_SIZE] = "";
	int i=0;
	Florist_t firstFlor, secondFlor, thirdFlor ;
	Consumer_t currCustomer;
	pthread_t t1;
	pthread_t t2;
	pthread_t t3;
	int targetFlorist;
	int i1 = 0, i2 = 0, i3 = 0; 

	if(argc != 2){
		fprintf(stderr, "Usage : %s FileName\n",argv[0]);
		exit(EXIT_ERR);
	}


	if ((fileDescriptor = open(argv[1], READ_FLAGS)) == -1) {
            fprintf(stderr, "Central Thread failed to open %s\n", argv[1]);
            exit(1);
    }
	
	printf("\nFlorist application initializing from file: %s\n\n", argv[1]);

	srand(time(NULL));

    while(readline(fileDescriptor,buff, MAX_SIZE) != 0){
    	if(i == 0){	//First Florist
	    	ProcessFlorist(buff, &firstFlor);
	    	if( pthread_create(&t1, NULL, firstFlorist, &firstFlor) !=  0){
	    		fprintf(stderr, "Failed to pthread_create\n");
	    		exit(EXIT_ERR);
	    	}
	    	printf("Florist %s created. Coordinates X:%d Y:%d. Delivery Speed %.2f. Flowers in the shop %s, %s, %s\n",
	    		firstFlor.name, firstFlor.x, firstFlor.y, firstFlor.tickRate, FLOWERS[firstFlor.first], FLOWERS[firstFlor.second], FLOWERS[firstFlor.third]);
    	}else if(i == 1){	// Second Florist
    		ProcessFlorist(buff, &secondFlor);
    		if( pthread_create(&t2, NULL, secondFlorist, &secondFlor) !=  0){
	    		fprintf(stderr, "Failed to pthread_create\n");
	    		exit(EXIT_ERR);
	    	}
	    	printf("Florist %s created. Coordinates X:%d Y:%d. Delivery Speed %.2f. Flowers in the shop %s, %s, %s\n",
	    		secondFlor.name, secondFlor.x, secondFlor.y, secondFlor.tickRate, FLOWERS[secondFlor.first], FLOWERS[secondFlor.second], FLOWERS[secondFlor.third]);
    	}else if(i == 2){	//Third Florist
    		ProcessFlorist(buff, &thirdFlor);
    		if( pthread_create(&t3, NULL, thirdFlorist, &thirdFlor) !=  0){
	    		fprintf(stderr, "Failed to pthread_create\n");
	    		exit(EXIT_ERR);
	    	}
	    	printf("Florist %s created. Coordinates X:%d Y:%d. Delivery Speed %.2f. Flowers in the shop %s, %s, %s\n\n",
	    		thirdFlor.name, thirdFlor.x, thirdFlor.y, thirdFlor.tickRate, FLOWERS[thirdFlor.first], FLOWERS[thirdFlor.second], FLOWERS[thirdFlor.third]);
    	}else if(strlen(buff) > 8){
    		//Customers
    	
    		ProcessCustomer(buff, &currCustomer);
    		targetFlorist = SelectFlorist(currCustomer,firstFlor,secondFlor, thirdFlor);

    		if(targetFlorist == 1){ // Florist 1

    			if(pthread_mutex_lock(&lock1)){
    				fprintf(stderr, "Mutex failed to lock\n" );
    				exit(EXIT_ERR);
    			}

    			++FF;
    			listForFirst[i1] = currCustomer;
    			++i1;

    			if(pthread_mutex_unlock(&lock1)){
    				fprintf(stderr, "Mutex failed to unlock\n" );
    				exit(EXIT_ERR);
    			}

    			if(pthread_cond_signal(&cond1)){
    				fprintf(stderr, "Condition Signal Failed\n");
    				exit(EXIT_ERR);
    			}

    		}else if( targetFlorist == 2){ // Florist 2

    			if(pthread_mutex_lock(&lock2)){
    				fprintf(stderr, "Mutex failed to lock\n" );
    				exit(EXIT_ERR);
    			}

    			++SF;
    			listForSecond[i2] = currCustomer;
    			++i2;

    			if(pthread_mutex_unlock(&lock2)){
    				fprintf(stderr, "Mutex failed to unlock\n" );
    				exit(EXIT_ERR);
    			}

    			if(pthread_cond_signal(&cond2)){
    				fprintf(stderr, "Condition Signal Failed\n");
    				exit(EXIT_ERR);
    			}


    		}else {	// Florist 3

    			if(pthread_mutex_lock(&lock3)){
    				fprintf(stderr, "Mutex failed to lock\n" );
    				exit(EXIT_ERR);
    			}

    			++TF;
    			listForThird[i3] = currCustomer;
    			++i3;

    			if(pthread_mutex_unlock(&lock3)){
    				fprintf(stderr, "Mutex failed to unlock\n" );
    				exit(EXIT_ERR);
    			}

    			if(pthread_cond_signal(&cond3)){
    				fprintf(stderr, "Condition Signal Failed\n");
    				exit(EXIT_ERR);
    			}

    		}    	

    	}

    	++i;
    }
	
    printf("-> All requests processed. Waiting for Florists.\n");

    done = 1;

    if(pthread_cond_signal(&cond1)){
		fprintf(stderr, "Condition Signal Failed\n");
		exit(EXIT_ERR);
	}	

	if(pthread_cond_signal(&cond2)){
		fprintf(stderr, "Condition Signal Failed\n");
		exit(EXIT_ERR);
	}

	if(pthread_cond_signal(&cond3)){
		fprintf(stderr, "Condition Signal Failed\n");
		exit(EXIT_ERR);
	}	

    if(pthread_join(t1,NULL) != 0){
    	fprintf(stderr,"Failed to Thread1 Join\n");
    	exit(EXIT_ERR);
    }

	if(pthread_join(t2,NULL) != 0 ){
		fprintf(stderr, "Failed to Thread2 Join\n");
		exit(EXIT_ERR);
	}

    if(pthread_join(t3,NULL) != 0){
    	fprintf(stderr, "Failed to Thread3 Join\n");
    	exit(EXIT_ERR);
    }

    pthread_mutex_destroy(&lock1);
    pthread_mutex_destroy(&lock2);
    pthread_mutex_destroy(&lock3);

    printf("\n\n--------------------------------------------------------\n");
    printf("Florsit \t # of sales \t Total Time\n");
    printf("--------------------------------------------------------\n");

    printf("%s \t\t %d \t\t %d\n",firstFlor.name,sale1.sale, sale1.totalTime);
	printf("%s \t\t %d \t\t %d\n",secondFlor.name,sale2.sale, sale2.totalTime);
 	printf("%s \t\t %d \t\t %d\n",thirdFlor.name,sale3.sale, sale3.totalTime);
    printf("--------------------------------------------------------\n");




	return EXIT_SUCC;
}


static void* firstFlorist(void *arg){
	Florist_t florist = *(Florist_t *)arg;
	int numOfSales = 0;
	int totalTime = 0;
	int travelTime = 0;
	int rng = 0;
	SaleReport_t report;
	char temp[MAX_SIZE];

	while(done == 0 || FF > 0){

		if(pthread_mutex_lock(&lock1)){
			fprintf(stderr, "Mutex failed to lock\n" );
			exit(EXIT_ERR);
		}

		while(FF == 0 && done == 0){
			pthread_cond_wait(&cond1,&lock1);
		}

		--FF;

		if(pthread_mutex_unlock(&lock1)){
			fprintf(stderr, "Mutex failed to unlock\n");
			exit(EXIT_ERR);
		}

		rng = rand()%40 +10;

		travelTime = CalculateTimeToTravel(Distance(florist.x,florist.y, listForFirst[numOfSales].x, listForFirst[numOfSales].y), florist.tickRate);
		travelTime += rng + travelTime;

		usleep(travelTime);

		printf("Florist %s has delivered a %s to %s, and returned to shop in %dms\n", florist.name, FLOWERS[listForFirst[numOfSales].product], listForFirst[numOfSales].name, travelTime );

		++numOfSales;

		totalTime += travelTime;			
		
	}

	printf("-> %s closing shop.\n", florist.name);
	sale1.sale = numOfSales;
	sale1.totalTime = totalTime;

	pthread_exit(EXIT_SUCC);
}




static void* secondFlorist(void *arg){
	Florist_t florist = *(Florist_t *)arg;
	int numOfSales = 0;
	int totalTime = 0;
	int travelTime = 0;
	int rng = 0;
	SaleReport_t report;
	char temp[MAX_SIZE];

	while(done == 0 || SF > 0){

		if(pthread_mutex_lock(&lock2)){
			fprintf(stderr, "Mutex failed to lock\n" );
			exit(EXIT_ERR);
		}

		while(SF == 0 && done == 0){
			pthread_cond_wait(&cond2,&lock2);
		}

		--SF;

		if(pthread_mutex_unlock(&lock2)){
			fprintf(stderr, "Mutex failed to unlock\n");
			exit(EXIT_ERR);
		}
	
		rng = rand()%40 +10;

		travelTime = CalculateTimeToTravel(Distance(florist.x,florist.y, listForSecond[numOfSales].x, listForSecond[numOfSales].y), florist.tickRate);
		travelTime += rng + travelTime;

		usleep(travelTime);

		printf("Florist %s has delivered a %s to %s, and returned to shop in %dms\n", florist.name, FLOWERS[listForSecond[numOfSales].product], listForSecond[numOfSales].name, travelTime );

		++numOfSales;

		totalTime += travelTime;			
		
	}

	printf("-> %s closing shop.\n", florist.name);
	sale2.sale = numOfSales;
	sale2.totalTime = totalTime;
	pthread_exit(EXIT_SUCC);
}

static void* thirdFlorist(void *arg){
	Florist_t florist = *(Florist_t *)arg;
	int numOfSales = 0;
	int totalTime = 0;
	int travelTime = 0;
	int rng = 0;
	SaleReport_t report;
	char temp[MAX_SIZE];

	while(done == 0 || TF > 0){

		if(pthread_mutex_lock(&lock3)){
			fprintf(stderr, "Mutex failed to lock\n" );
			exit(EXIT_ERR);
		}

		while(TF == 0 && done == 0){
			pthread_cond_wait(&cond3,&lock3);
		}

		--TF;

		if(pthread_mutex_unlock(&lock3)){
			fprintf(stderr, "Mutex failed to unlock\n");
			exit(EXIT_ERR);
		}

		rng = rand()%40 +10;

		travelTime = CalculateTimeToTravel(Distance(florist.x,florist.y, listForThird[numOfSales].x, listForThird[numOfSales].y), florist.tickRate);
		travelTime += rng + travelTime;

		usleep(travelTime);

		printf("Florist %s has delivered a %s to %s, and returned to shop in %dms\n", florist.name, FLOWERS[listForThird[numOfSales].product], listForThird[numOfSales].name, travelTime );

		++numOfSales;

		totalTime += travelTime;
		
	}

	printf("-> %s closing shop.\n", florist.name);

	sale3.sale = numOfSales;
	sale3.totalTime = totalTime;

	pthread_exit(EXIT_SUCC);
}


void ProcessCustomer(char* buff, Consumer_t *customer){
	char flowerName[MAX_SIZE];
	int i = 0;

	sscanf(buff, "%s (%d,%d): %s", customer->name, &(customer->x), &(customer->y),flowerName );

	while(strcmp(flowerName,FLOWERS[i])){
		++i;
	}
	customer->product = i;
}


double Distance(int x1, int y1, int x2, int y2){
	double x = pow( (x1-x2), 2.0);
	double y = pow( (y1-y2), 2.0);
	return  sqrt(x+y);
}

int CalculateTimeToTravel(double distance, double tickRate){
	return distance/tickRate;
}

void ProcessFlorist(char* buff, Florist_t* florist){
	char firstFlow[MAX_SIZE];
	char secondFlow[MAX_SIZE];
	char thirdFlow[MAX_SIZE];
	int first = 0, second = 0, i=0;


	sscanf(buff,"%s (%d,%d; %d.%d) : %s %s %s", florist->name, &(florist->x), &(florist->y), &first , &second, firstFlow, secondFlow, thirdFlow  );
	florist->tickRate =  first + second/pow(10, floor(log10(second) + 1));

	firstFlow[strlen(firstFlow)-1] = '\0';
	secondFlow[strlen(secondFlow)-1] = '\0';

	while(strcmp(firstFlow,FLOWERS[i])){
		++i;
	}
	florist->first = i;

	i=0;
	while(strcmp(secondFlow,FLOWERS[i])){
		++i;
	}
	florist->second = i;

	i=0;
	while(strcmp(thirdFlow,FLOWERS[i])){
		++i;
	}
	florist->third = i;
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


int HasRequiredFlower(Flowers flower, Florist_t florist){
	if(flower == florist.first || flower == florist.second || flower == florist.third)
		return 1;
	return 0;
}

int SelectFlorist(Consumer_t customer, Florist_t f1, Florist_t f2, Florist_t f3){
	double distTo1, distTo2, distTo3, temp;
	double sort[3];
	int arr[3] = {1,2,3};
	int tempInt;
	Florist_t florArr[4];
	florArr[1] = f1;
	florArr[2] = f2;
	florArr[3] = f3;

	distTo1 = Distance(customer.x,customer.y,f1.x,f1.y);
	distTo2 = Distance(customer.x,customer.y,f2.x,f2.y);
	distTo3 = Distance(customer.x,customer.y,f3.x,f3.y);

	sort[0] = distTo1;
	sort[1] = distTo2;
	sort[2] = distTo3;

	if(sort[0]>sort[1]){
		temp = sort[0];
		sort[0] = sort[1];
		sort[1] = temp;
		tempInt = arr[0];
		arr[0] = arr[1];
		arr[1] = tempInt;
	}
	if(sort[1] > sort[2]){
		temp = sort[1];
		sort[1] = sort[2];
		sort[2] = temp;
		tempInt = arr[1];
		arr[1] = arr[2];
		arr[2] = tempInt;
	}
	if(sort[0]>sort[1]){
		temp = sort[0];
		sort[0] = sort[1];
		sort[1] = temp;
		tempInt = arr[0];
		arr[0] = arr[1];
		arr[1] = tempInt;
	}

	if(HasRequiredFlower(customer.product,florArr[arr[0]])){
		return arr[0];
	}else if (HasRequiredFlower(customer.product,florArr[arr[1]])){
		return arr[1];
	}else{
		return arr[2];
	}

}