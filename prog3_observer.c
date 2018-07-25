//
// prog3_observer.c
// David Nachmanson
// CSCI 367 Winter 2017
// February, 2017
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

/*------------------------------------------------------------------------
* Program: prog3_observer.c
*
* Purpose: allocate a socket, connect to a server, connect to a chatters
*          print messages.
* Syntax: ./prog3_observer server_address server_port
*
* server_address - name of a computer on which server is executing
* server_port    - protocol port number server is using
*
*------------------------------------------------------------------------
*/
void bindToUserName(char *username, int sd){
  uint8_t size;
  scanf("%s",username);
  size = strlen(username);
  while(size > 10){
    printf("enter a username less than 11 characters: ");
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
  int bytesRecieved;
	char buf[1000];

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


  bytesRecieved = recv(sd,buf,1,0);
  if(bytesRecieved == 0){
    printf("server dropped you,not your fault\n");
    close(sd);
    exit(EXIT_SUCCESS);
  }
  int negotiating = 0;
	if(buf[0] == 'Y'){
	   negotiating = 1;
     printf("enter a username: ");
	   bindToUserName(buf,sd);
	   bytesRecieved = recv(sd, buf, 1, 0);
	   while(negotiating == 1) {
       if(bytesRecieved == 0){
         printf("server dropped connection,took to long to enter username\n");
         close(sd);
         exit(EXIT_FAILURE);
       } else if(buf[0] == 'T'){
	         printf("Username taken..try another one: ");
	         bindToUserName(buf,sd);
	     } else if(buf[0] == 'I'){
	         printf("invalid username, try again: ");
	         bindToUserName(buf,sd);
	     } else if(buf[0] == 'Y'){
	         negotiating = 0;
	     } else if(buf[0] == 'N'){
          printf("that participant doesnt exist.\n");
          close(sd);
      	  exit(EXIT_FAILURE);
       }
       if(negotiating != 0){
	        bytesRecieved == recv(sd,buf,1,0);
	        if(bytesRecieved == 0){
	           printf("took to long to enter valid username..better luck next time\n");
		         close(sd);
		         exit(EXIT_FAILURE);
          }
      }
    }
  } else {
    printf("server cant host any more observers\n");
	  close(sd);
	  exit(EXIT_SUCCESS);
  }
  uint16_t size;
  while(1){
	      bytesRecieved = recv(sd, &size, sizeof(uint16_t),MSG_WAITALL);
		    if(bytesRecieved == 0){ // server has closed
		        close(sd);
		        exit(EXIT_SUCCESS);
		    }
        size = ntohs(size);
        if(size > 0){
          bytesRecieved = recv(sd,buf,size,MSG_WAITALL);
          if(bytesRecieved == 0){ // server has closed
		        close(sd);
		        exit(EXIT_SUCCESS);
		        }
            if(buf[size - 1] != '\n'){
              buf[size] = '\n';
              size = size + 1;
            }
            write(1,buf,size);
        }

	  }

}
