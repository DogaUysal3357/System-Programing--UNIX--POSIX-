/***** INCLUDES *****/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/file.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_SIZE 256

int callSocket(char *hostname, unsigned short portnum);

/**** MAIN ****/
int main(int argc, char** argv){

	int portAddress;
	int socketfd ;
	char buf[256];
	struct timeval start_time, end_time;
	double waitTime;
	double calculationTime, result;
	int price;
	char providerName[256];

	if(argc != 6){
		fprintf(stderr, "Usage %d : %s Name Priority HomeWork ServerAdress PortAdress\n",argc, argv[0] );
		return 1;
	}
	
	gettimeofday(&start_time, NULL);

	if(	(socketfd = callSocket(argv[4], atoi(argv[5]))) == -1 ){
		fprintf(stderr, "Failed to connect server.\n" );
		return 1;
	}

	fprintf(stderr,"Client %s is requesting %s %s from server %s:%s\n", argv[1], argv[2], argv[3], argv[4], argv[5]);
	sprintf(buf,"%s %s %s", argv[1],argv[2],argv[3]);

	if(( portAddress = write(socketfd,buf, strlen(buf))) <1){
		fprintf(stderr,"Failed to write. %d",portAddress);
	}

	while((portAddress = read(socketfd,buf, MAX_SIZE)) <1){
		if(portAddress == -1){
			fprintf(stderr, "Failed to read.\n" );
			return 1;
		}
	}

	sscanf(buf,"%s", providerName);

	//No Provider was available
	if(strcmp(providerName,"NO") == 0){
		fprintf(stderr, "No Provider Available\n");
		close(socketfd);
		return 1;
	}else if(strcmp(providerName, "SERVER") == 0){
		fprintf(stderr, "SERVER SHUTTIN DOWN\n" );
		close(socketfd);
		return 1;
	}

	gettimeofday(&end_time,NULL);
	waitTime = (double)(end_time.tv_usec - start_time.tv_usec) / 1000000 + (double)(end_time.tv_sec - start_time.tv_sec);

	sscanf(buf,"%s %lf %lf %d", providerName, &calculationTime, &result, &price);
	fprintf(stderr,"%s's task completed by %s in %.2f seconds, cos(%d) = %.2f, cost is %dTL, total time spent %.2f\n",argv[1],providerName,calculationTime, atoi(argv[3]), result, price, waitTime );

	close(socketfd);
	return 0;
}


int callSocket(char *hostname, unsigned short portnum){
    struct sockaddr_in sa;
    struct hostent *hp;
    int a, s;
    if ((hp= gethostbyname(hostname)) == NULL) { /* do we know the host's */
        errno= ECONNREFUSED; /* address? */
        return(-1); /* no */
    } /* if gethostname */
    memset(&sa,0,sizeof(sa));
    memcpy((char *)&sa.sin_addr,hp->h_addr,hp->h_length); /* set address */
    sa.sin_family= hp->h_addrtype;
    sa.sin_port= htons((u_short)portnum);
    if ((s= socket(hp->h_addrtype,SOCK_STREAM,0)) < 0) /* get socket */
        return(-1);
    if (connect(s,(struct sockaddr *)&sa,sizeof sa) < 0) { /* connect */
        close(s);
        return(-1);
    } /* if connect */
    return(s);
} 