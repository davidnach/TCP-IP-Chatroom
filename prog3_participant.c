//
// prog3_participant.c
// David Nachmanson
// CSCI 367 Winter 2017
// January 29, 2017
//
// Sourced from demo_server.c provided from CSCI 367 Winter 2017

/** demo_client.c - code for example client program that uses TCP **/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define MAX_BUF 1000

void getUsername( char *username,int sd){
  uint8_t size;
  scanf("%s",username);
  size = strlen(username);
  while(size > 10){
    printf("enter a username of less than 11 characters: ");
    scanf("%s",username);
    size = strlen(username);
  }
  send(sd,&size,sizeof(uint8_t),0);
  send(sd,username,size,0);

}

int main( int argc, char **argv) {
	struct hostent *ptrh; /* pointer to a host table entry */
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold an IP address */
	int sd; /* socket descriptor */
	int port; /* protocol port number */
	char *host; /* pointer to host name */
	char buf[MAX_BUF]; /* buffer for data from the server */

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */

	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./client server_address server_port\n");
		exit(EXIT_FAILURE);
	}

	port = atoi(argv[2]); /* convert to binary */
	if (port > 0) /* test for legal value */
		sad.sin_port = htons((u_short)port);
	else {
		fprintf(stderr,"Error: bad port number %s\n",argv[2]);
		exit(EXIT_FAILURE);
	}

	host = argv[1]; /* if host argument specified */

	/* Convert host name to equivalent IP address and copy to sad. */
	ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		fprintf(stderr,"Error: Invalid host: %s\n", host);
		exit(EXIT_FAILURE);
	}

	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	/* Map TCP transport protocol name to protocol number. */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket. */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Connect the socket to the specified server. */
	if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"connect failed\n");
		exit(EXIT_FAILURE);
	}

	/* Establish username and proper connection with server */
	int negotiating = 0;
	int bytesRecieved = 0;;
  bytesRecieved = recv(sd,buf,1,0);
  if(bytesRecieved == 0){
    printf("server dropped you,not your fault\n");
    close(sd);
    exit(EXIT_SUCCESS);
  }
  fprintf(stderr,"%c\n",buf[0]);
	if(buf[0] == 'Y'){
	  negotiating = 1;
    printf("input username: ");
	   getUsername(buf,sd);
	   bytesRecieved = recv(sd, buf, 1, 0);
	   while(negotiating == 1) {
       if(bytesRecieved == 0){
         printf("server dropped connection,took to long to enter username\n");
         close(sd);
         exit(EXIT_FAILURE);
       } else if(buf[0] == 'T'){
	         printf("Username taken..try another one: ");
	         getUsername(buf,sd);
	     } else if(buf[0] == 'I'){
	         printf("invalid username, try again : ");
	         getUsername(buf,sd);
	     } else if(buf[0] == 'Y'){
	         negotiating = 0;
	     }
       if(negotiating != 0){
	        bytesRecieved == recv(sd,buf,1,0);
	        if(bytesRecieved == 0){
	           printf("took to long to enter valid username..better luck next time");
		         close(sd);
		         exit(EXIT_FAILURE);
	       }
	    }
    }
	  uint16_t msgSize;
    uint16_t nsbyteOrder;
    int i = 0;
    while((i = getchar()) != '\n' && i != EOF);
    printf("enter a message:");
	  while(1){
       fgets(buf,MAX_BUF + 1,stdin);
	     msgSize = strlen(buf) ;
       nsbyteOrder = htons(msgSize);
	     send(sd,&nsbyteOrder,sizeof(uint16_t),0);
	     send(sd,buf,msgSize,0);
       if(msgSize < MAX_BUF || buf[msgSize - 1] == '\n'){
         printf("enter a message:");
       }
	  }
	} else {
	   printf("server cannot handle anymore participants,try to reconnect soon\n");
	   close(sd);
	   exit(EXIT_SUCCESS);
	}
}
