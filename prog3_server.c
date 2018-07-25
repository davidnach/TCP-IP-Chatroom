//
// prog3_server
// David Nachmanson
// CSCI 367 Winter 2017
// February 25,2017
//
// Sourced from demo_server.c provided from CSCI 367 Winter 2017

/** demo_server.c - code for example server program that uses TCP **/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h> /* keep track of round timer */
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h> /* for checking username validity */

#define QLEN 6 /* size of request queue */
#define MAX_CLIENTS 255 /* max number of concurrent observers and participants*/
#define WAIT_TIME 60 /*time allocated to secure connection*/
#define MAX_BUF 1000 /*max size of message allowed to be sent
/*------------------------------------------------------------------------
* Program: prog3_server
*
* Purpose: establish two ports, two listening sockets, accept and handle
*          connections, host a chatroom.
*
* Syntax: ./prog3_server participant_port observer_port
*
* participant_port - port on which chatters connect
* observer_port    - port on which observers connect
*
*------------------------------------------------------------------------
*/


typedef struct chatter{
  int active;
  int sd;
  int observer_sd;
  char name[11];
  struct timeval tv;
}chatter;

typedef struct observer{
  int active;
  int sd;
  struct timeval tv;
}observer;

// Globals
chatter chatter_set[MAX_CLIENTS];
observer observer_set[MAX_CLIENTS];
int active_Observers = 0;
int active_Chatters = 0;

void initializeChatterSet(){
  for(int i = 0; i < MAX_CLIENTS;i++){
    chatter_set[i].sd = -1;
    chatter_set[i].observer_sd = -1;
    chatter_set[i].active = 0;
  }
}

void intitializeObserverSet(){
  for(int i = 0; i < MAX_CLIENTS;i++){
    observer_set[i].sd = -1;
    observer_set[i].active = 0;
  }
}

//closes connection with either a participant or an observer. If a participant is
//removed and has an associated observer, that observer is removed.
void removeClient(int sd,fd_set *master,int *connectedWithNoInput){
  uint16_t networkByteOrder;
  int errorCheck;
  for(int i = 0; i < MAX_CLIENTS; i++){
    if(chatter_set[i].sd == sd){
      chatter_set[i].sd = -1;
      FD_CLR(sd,master);
      close(sd);
      if(chatter_set[i].active == 0){
        *connectedWithNoInput--;
      }else {
        int nameLength = strlen(chatter_set[i].name);
        int msgLength;
        char msg[25];
        sprintf(msg,"%s","User ");
        sprintf(&(msg[5]),"%s ",chatter_set[i].name);
        sprintf(&(msg[5 + nameLength + 1]),"%s","has left");
        msgLength = strlen(msg);
        networkByteOrder = htons(msgLength);
        for(int j = 0; j < MAX_CLIENTS; j++){
          if(chatter_set[j].observer_sd != -1){
            errorCheck = send(chatter_set[j].observer_sd,&networkByteOrder,sizeof(uint16_t),MSG_NOSIGNAL|MSG_DONTWAIT);
            errorCheck = send(chatter_set[j].observer_sd,msg,msgLength,MSG_NOSIGNAL|MSG_DONTWAIT);
            if(errorCheck == -1 && i != j){
              removeClient(chatter_set[j].observer_sd,master,connectedWithNoInput);
            }
          }
        }
        chatter_set[i].active = 0;
        active_Chatters--;

        if(chatter_set[i].observer_sd != -1){
          removeClient(chatter_set[i].observer_sd,master,connectedWithNoInput);
          chatter_set[i].observer_sd = -1;
        }
      }
      break;
    }
    if(observer_set[i].sd == sd){
      if(observer_set[i].active == 0){
        (*connectedWithNoInput)--;
      }else {
        active_Observers--;
        observer_set[i].active = 0;
        for(int j = 0; j < MAX_CLIENTS;j++){
          if(chatter_set[j].observer_sd == observer_set[i].sd){
            chatter_set[j].observer_sd = -1;
            break;
          }
        }
      }
      FD_CLR(sd,master);
      close(sd);
      observer_set[i].sd = -1;
      break;
    }
  }
}

void sendPublicMsg(char *buf,int senderIndex,uint16_t msgSize,int *connectedWithNoInput,
fd_set *master){
  uint16_t netByteOrder;
  int errorCheck;
  char msg[MAX_BUF + 14];
  int recipient_Observer;
  int senderNameLength = strlen(chatter_set[senderIndex].name);
  msg[0] = '>';
  msg[12] = ':';
  msg[13] = ' ';
  int variableSpaces;
  variableSpaces = 11 - senderNameLength;
  for(int i = 0; i < variableSpaces; i++){
    msg[i + 1] = ' ';
  }
  strncpy(&(msg[variableSpaces + 1]),chatter_set[senderIndex].name,senderNameLength);
  strncpy(&(msg[14]),buf,msgSize);
  netByteOrder = htons(msgSize + 14);
  for(int i = 0; i < MAX_CLIENTS; i++){
    if(chatter_set[i].active == 1 && chatter_set[i].observer_sd != -1){
      recipient_Observer = chatter_set[i].observer_sd;
      errorCheck = send(recipient_Observer,&netByteOrder,sizeof(uint16_t),MSG_NOSIGNAL|MSG_DONTWAIT);
      errorCheck = send(recipient_Observer,msg,msgSize + 14,MSG_NOSIGNAL|MSG_DONTWAIT);
      if(errorCheck == -1){
        removeClient(recipient_Observer,master,connectedWithNoInput);
      }
    }
  }
}

void sendPrivateMsg(char *name,char *buf, int senderIndex,uint16_t msgSize,int *connectedWithNoInput,
fd_set *master){
  uint16_t netByteOrder;
  int errorCheck;
  int warning = 1;
  int sender_Observer;
  int recipient_Observer;
  char msg[MAX_BUF + 14];
  int senderNameLength = strlen(chatter_set[senderIndex].name);
  msg[0] = '*';
  msg[12] = ':';
  msg[13] = ' ';
  int variableSpaces;
  variableSpaces = 11 - senderNameLength;
  for(int i = 0; i < variableSpaces; i++){
    msg[i + 1] = ' ';
  }
  strncpy(&(msg[variableSpaces + 1]),chatter_set[senderIndex].name,senderNameLength);
  strncpy(&(msg[14]),buf,msgSize);
  netByteOrder = htons(msgSize + 14);
  for(int i = 0; i < MAX_CLIENTS; i++){
    if(strcmp(chatter_set[i].name,name) == 0){
      recipient_Observer = chatter_set[i].observer_sd;
      if(recipient_Observer != -1){
        errorCheck = send(recipient_Observer,&netByteOrder,sizeof(uint16_t),MSG_NOSIGNAL|MSG_DONTWAIT);
        errorCheck = send(recipient_Observer,msg,msgSize + 14,MSG_NOSIGNAL|MSG_DONTWAIT);
        if(errorCheck == -1){
          removeClient(recipient_Observer,master,connectedWithNoInput);
        }
        sender_Observer = chatter_set[senderIndex].observer_sd;
        if(sender_Observer!= -1){
          errorCheck = send(sender_Observer,&netByteOrder,sizeof(uint16_t),MSG_NOSIGNAL|MSG_DONTWAIT);
          errorCheck = send(sender_Observer,msg,msgSize + 14,MSG_NOSIGNAL|MSG_DONTWAIT);
          if(errorCheck == -1){
            removeClient(sender_Observer,master,connectedWithNoInput);
          }
        }
      }
      warning = 0;
    }
 }
 if (warning == 1){
 char warning[41];
 sprintf(warning,"Warning: user ");
 sprintf(&(warning[14]),"%s",name);
 int i = 14;
 while(warning[i] != '\0'){
   i++;
 }
 warning[i] = ' ';
 i++;
 sprintf(&(warning[i]),"doesn't exist...\n");
 netByteOrder = htons(strlen(warning));
 errorCheck = send(chatter_set[senderIndex].observer_sd,&netByteOrder,sizeof(uint16_t),MSG_NOSIGNAL|MSG_DONTWAIT);
 errorCheck = send(chatter_set[senderIndex].observer_sd,warning,strlen(warning),MSG_NOSIGNAL|MSG_DONTWAIT);
 if(errorCheck == -1){
   removeClient(chatter_set[senderIndex].observer_sd,master,connectedWithNoInput);
 }
 }
 }

//returns index of observer
int findObserver(int sd){
  for(int i = 0; i < MAX_CLIENTS;i++){
    if(observer_set[i].sd == sd){
      return i;
    }
  }
}
int findChatter(int sd){
  for(int i = 0; i < MAX_CLIENTS;i++){
    if(chatter_set[i].sd == sd){
      return i;
    }
  }
}


//returns 1 for chatter, 0 for observer
int observerOrChatter(int sd){
  for(int i = 0; i < MAX_CLIENTS;i++){
    if(chatter_set[i].sd == sd){
      return 1;
    }else if(observer_set[i].sd == sd){
      return 0;
    }
  }
}


//returns 1 if valid username, 0 otherwise
int validateUsername( char *username,int length){
  for(int i = 0; i < length; i++){
    if(isalpha(username[i]) == 0 && username[i] != 95 && isdigit(username[i]) == 0){
      return 0;
    }
  }
  return 1;
}

int bindToChatter(char *username,int observer_sd,int *taken,int *connectedWithNoInput,
fd_set *master){
  int errorCheck;
  for(int i = 0; i < MAX_CLIENTS; i++){
    if(strcmp(chatter_set[i].name,username) == 0){
      if(chatter_set[i].observer_sd != -1){
        (*taken)++;
        return 0;
      }else {
        char newObserver[] = "A new observer has joined";
        uint16_t msgSize = htons(25);
        for(int j = 0; j < MAX_CLIENTS; j++){
          if(chatter_set[j].active == 1 && chatter_set[j].observer_sd != -1){
            errorCheck = send(chatter_set[j].observer_sd,&msgSize,sizeof(uint16_t),MSG_NOSIGNAL|MSG_DONTWAIT);
            errorCheck = send(chatter_set[j].observer_sd,newObserver,25,MSG_NOSIGNAL|MSG_DONTWAIT);
            if(errorCheck == -1){
              removeClient(chatter_set[j].observer_sd,master,connectedWithNoInput);
            }
          }
        }
        chatter_set[i].observer_sd = observer_sd;
        active_Observers++;
        return 1;
      }
    }
  }
  return 0;
}


/*return values : returns 0 if the name a user inserts is
already taken by somebody. Returns 1 otherwise.*/

int checkUsernames(char *name){
  for(int i = 0; i < MAX_CLIENTS;i++){
    if(chatter_set[i].active == 1){
      if(strcmp(chatter_set[i].name,name) == 0){
        return 0;
      }
    }
  }
  return 1;
}


void minTimeoutFinder(struct timeval *select){
    for(int i = 0; i < MAX_CLIENTS; i++){
      if(chatter_set[i].sd != -1 && chatter_set[i].active == 0){
        if((chatter_set[i].tv.tv_sec) < select->tv_sec){
            (*select).tv_sec = chatter_set[i].tv.tv_sec;
            (*select).tv_usec = chatter_set[i].tv.tv_usec;
          }else if(chatter_set[i].tv.tv_sec == select->tv_sec){
            if(chatter_set[i].tv.tv_usec < select->tv_usec){
              (*select).tv_usec = chatter_set[i].tv.tv_usec;
           }
         }
      }
      if(observer_set[i].sd != -1 && observer_set[i].active == 0){
        if(observer_set[i].tv.tv_sec < select->tv_sec){
          (*select).tv_sec = observer_set[i].tv.tv_sec;
          (*select).tv_usec = observer_set[i].tv.tv_usec;
        }else if(observer_set[i].tv.tv_sec == select->tv_sec){
           if(observer_set[i].tv.tv_usec < select->tv_usec){
             (*select).tv_usec = observer_set[i].tv.tv_usec;
           }
        }
    }
  }
}

void addObserverConnection(int sd){
  for(int i = 0; i < MAX_CLIENTS;i++){
    if(observer_set[i].sd == -1){
      observer_set[i].sd = sd;
      observer_set[i].tv.tv_sec = WAIT_TIME;
      observer_set[i].tv.tv_usec = 0;
      break;
    }
  }
}

void addChatterConnection(int sd){
  for(int i = 0; i < MAX_CLIENTS;i++){
    if(chatter_set[i].sd == -1){
      chatter_set[i].sd = sd;
      chatter_set[i].tv.tv_sec = WAIT_TIME;
      chatter_set[i].tv.tv_usec = 0;
      break;
    }
  }
}

void timeDifference(long int *seconds, long int *microSeconds,struct timeval *select){
  int long tempSeconds = *seconds;
  int long tempMseconds = *microSeconds;
  if (select->tv_usec > tempMseconds){
    *seconds = (tempSeconds - select->tv_sec) - 1;
    *microSeconds = 1000000 - (select->tv_usec - tempMseconds);
  }else if(select->tv_usec < tempMseconds){
    *seconds = (tempSeconds - select->tv_sec) + 1;
    *microSeconds = tempMseconds - select->tv_usec;
  }else {
    *seconds = tempMseconds - select->tv_sec;
    *microSeconds = 0;
  }
}

void insertUsername(char *name, int username_size,int sd,int index,char *buf,int *connectedWithNoInput){
  if(validateUsername(name,username_size)){
    if(checkUsernames(name)){
      printf("username [%s] accepted\n",name);
      strcpy(chatter_set[index].name,name);
      printf("new chatter name : %s \n",chatter_set[index].name);
      chatter_set[index].active = 1;
      active_Chatters++;
      (*connectedWithNoInput)--;
      buf[0] = 'Y';
      send(sd,buf,1,MSG_NOSIGNAL|MSG_DONTWAIT);
      uint16_t networkByteOrder;
      int nameLength = strlen(chatter_set[index].name);
      int msgLength;
      char msg[25];
      sprintf(msg,"%s","User ");
      sprintf(&(msg[5]),"%s ",chatter_set[index].name);
      sprintf(&(msg[5 + nameLength + 1]),"%s","has joined");
      msgLength = strlen(msg);
      networkByteOrder = htons(msgLength);
      for(int i = 0; i < MAX_CLIENTS; i++){
        if(chatter_set[i].observer_sd != -1){
          send(chatter_set[i].observer_sd,&networkByteOrder,sizeof(uint16_t),MSG_NOSIGNAL|MSG_DONTWAIT);
          send(chatter_set[i].observer_sd,msg,msgLength,MSG_NOSIGNAL|MSG_DONTWAIT);
        }
      }
      //send msg to observers
    }else {
      buf[0] = 'T';
      printf("username taken");
      send(sd,buf,1,MSG_DONTWAIT|MSG_NOSIGNAL);
    }
  }else {
    printf("username invalid");
    buf[0] = 'I';
    send(sd,buf,1,MSG_DONTWAIT|MSG_NOSIGNAL);
  }
}


void updateTimeouts(int long seconds,int long microSeconds
  ,int *connectedWithNoInput, fd_set *master){
   long int updatedSeconds;
   long int updatedMseconds;
  for(int i = 0; i < MAX_CLIENTS; i++){
    if(chatter_set[i].active == 0 && chatter_set[i].sd != -1){
      if(chatter_set[i].tv.tv_usec < microSeconds){
        updatedSeconds = chatter_set[i].tv.tv_sec - seconds - 1;
        updatedMseconds = 1000000 - (microSeconds - chatter_set[i].tv.tv_usec);
      }else if(chatter_set[i].tv.tv_usec > microSeconds){
        updatedSeconds = chatter_set[i].tv.tv_sec - seconds ;
        updatedMseconds = (chatter_set[i].tv.tv_usec - microSeconds);
      }else {
        updatedSeconds = chatter_set[i].tv.tv_sec - seconds;
        updatedMseconds = 0;
      }
      if(updatedSeconds <= 0){
          if(updatedSeconds < 0){
            close(chatter_set[i].sd);
            FD_CLR(chatter_set[i].sd,master);
            chatter_set[i].sd = -1;
            (*connectedWithNoInput)--;
          }else if(updatedMseconds == 0){
            if(updatedMseconds == 0){
              close(chatter_set[i].sd);
              FD_CLR(chatter_set[i].sd,master);
              chatter_set[i].sd = -1;
              (*connectedWithNoInput)--;
            }else {
              chatter_set[i].tv.tv_usec = updatedMseconds;
            }
          }
      }else {
        chatter_set[i].tv.tv_sec = updatedSeconds;
        chatter_set[i].tv.tv_usec = updatedMseconds;
      }
     }
    if(observer_set[i].active == 0 && observer_set[i].sd != -1){
      if(observer_set[i].tv.tv_usec < microSeconds){
        updatedSeconds = observer_set[i].tv.tv_sec - seconds - 1;
        updatedMseconds = 1000000 - (microSeconds - observer_set[i].tv.tv_usec);
      }else if(observer_set[i].tv.tv_usec > microSeconds){
        updatedSeconds = observer_set[i].tv.tv_sec - seconds ;
        updatedMseconds = observer_set[i].tv.tv_usec - microSeconds;
      }else {
        updatedSeconds = observer_set[i].tv.tv_sec - seconds;
        updatedMseconds = observer_set[i].tv.tv_usec;
      }
      if(updatedSeconds <= 0){
          if(updatedSeconds < 0){
          close(observer_set[i].sd);
          FD_CLR(observer_set[i].sd,master);
          observer_set[i].sd = -1;
          (*connectedWithNoInput)--;
        }else if(updatedMseconds <= 0){
          close(observer_set[i].sd);
          FD_CLR(observer_set[i].sd,master);
          observer_set[i].sd = -1;
          (*connectedWithNoInput)--;
        }else {
          observer_set[i].tv.tv_usec = updatedMseconds;
        }
      }else {
        observer_set[i].tv.tv_sec = updatedSeconds;
        observer_set[i].tv.tv_usec = updatedMseconds;
      }
    }
  }
}



// MAIN
int main(int argc, char **argv) {
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in S_PAD; /* structure to hold server's participant address */
	struct sockaddr_in S_OAD; /* structure to hold server's observer address */
	struct sockaddr_in pad; /* structure to hold client's address */
	struct sockaddr_in oad; /* structure to hold observer's address */
	int p_listen, o_listen, participantPort, observerPort; /* observer post number */
  fd_set master; /* master set of file descriptors */
	fd_set readfds; /* set of active file descriptors */
  int fdMax = 0; /* maximum file descriptor */
  int currentfdMax = 0;//fd max before select call
  int newP, newO, alen, blen; /* variables for accepting new connections */
	int optval = 1; /* boolean value when we set socket option */
	char buf[MAX_BUF]; /* buffer for string the server sends */
  int minTimeout = WAIT_TIME; //determine which sd has smallest amount of time left
  int timing = 0; //indicate whether selected used timeout
  long int seconds;//used to get timeout seconds value before select
  long int microSeconds;//used to get timeout microseconds value before select
  uint8_t username_size;
  uint16_t msgSize;
  int index; //location of client within array;


  /*keep track of time*/
  struct timeval selectTimeout;
  struct timeval *timeoutPointer;
  timeoutPointer = &selectTimeout;

  /* Error handle command line arguments */
	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./server participant_port observer_port\n");
		exit(EXIT_FAILURE);
	}

  FD_ZERO(&master); /* Clear master set */
  FD_ZERO(&readfds); /* clear temp set */

	memset((char *)&S_PAD,0,sizeof(S_PAD)); /* clear sockaddr structure */
	S_PAD.sin_family = AF_INET; /* set family to Internet */
	S_PAD.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	memset((char *)&S_OAD,0,sizeof(S_OAD)); /* clear sockaddr structure */
	S_OAD.sin_family = AF_INET; /* set family to Internet */
	S_OAD.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	/* check for valid participant and observer ports */
	participantPort = atoi(argv[1]); /* convert argument to binary */
	if(participantPort > 0) { /* test for illegal value */
		S_PAD.sin_port = htons((u_short)participantPort);
	} else { /* print error message and exit */
		fprintf(stderr,"Error: Bad port number %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}
	observerPort = atoi(argv[2]); /* convert argument to binary */
	if(observerPort > 0){ /* test for illegal value */
		S_OAD.sin_port = htons((u_short)observerPort);
	}else{ /* print error message and exit */
		fprintf(stderr,"Error: Bad port number %s\n",argv[2]);
		exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
	if(((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create participant listening socket */
	p_listen = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if(p_listen < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}
	/* Create observer listening socket */
	o_listen = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if(o_listen < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Allow reuse of port - avoid "Bind failed" issues */
	if(setsockopt(p_listen, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}
	/* Allow reuse of port - avoid "Bind failed" issues */
	if(setsockopt(o_listen, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}

	/* Bind a local address to the socket */
	if(bind(p_listen, (struct sockaddr *)&S_PAD, sizeof(S_PAD)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}
	/* Bind a local address to the socket */
	if(bind(o_listen, (struct sockaddr *)&S_OAD, sizeof(S_OAD)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	/* Specify size of request queue */
	if(listen(p_listen, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}
	/* Specify size of request queue */
	if(listen(o_listen, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}

  initializeChatterSet();
  intitializeObserverSet();


  int bytesRead = 0; /* number of bytes being received */
  int connectedWithNoInput = 0;//tracking clients that havent secured connection
  int selectReturn;

  /* Add listening sockets to fd_set and keep track of highest socket */
  FD_SET(p_listen, &master);
  FD_SET(o_listen, &master);
  if(p_listen > fdMax){
    fdMax = p_listen;
  }
  if(o_listen > fdMax){
    fdMax = o_listen;
  }

	while (1) {
    readfds = master;
    selectTimeout.tv_sec = WAIT_TIME;
    selectTimeout.tv_usec = 1000000;
    currentfdMax = fdMax;
    printf("connected with no input: %d\n",connectedWithNoInput);
    if(connectedWithNoInput > 0){
      timing = 1;
      minTimeoutFinder(&selectTimeout);
      seconds = selectTimeout.tv_sec;
      microSeconds = selectTimeout.tv_usec;
      printf("\nSelect monitoring...with timeout\n");
      selectReturn = select(currentfdMax + 1, &readfds, NULL, NULL, &selectTimeout);
    }else {
    printf("\nSelect monitoring...\n");
    selectReturn = select(currentfdMax + 1, &readfds, NULL, NULL, NULL);
    }
    if(selectReturn == -1){
      printf("Select Error\n");
      printf("Exiting\n");
      exit(EXIT_FAILURE);
    }else if(selectReturn == 0){ // Timeout has triggered
      updateTimeouts(seconds,microSeconds, &connectedWithNoInput,&master);
      minTimeout = WAIT_TIME;
    }else{
      currentfdMax = fdMax;
      for(int i = 0; i < currentfdMax + 1; i++){
        if(FD_ISSET(i, &readfds)){
          int errorCheck;
          if(i == p_listen){
            alen = sizeof(pad);
            if((newP = accept(p_listen, (struct sockaddr *)&pad, &alen)) < 0) {
              fprintf(stderr, "Error: Participant accept failed\n");
              exit(EXIT_FAILURE);
            }
            printf("currently there is %d active chatters\n",active_Chatters);
            if(active_Chatters < MAX_CLIENTS && connectedWithNoInput < MAX_CLIENTS) {
              buf[0] = 'Y';
              errorCheck = send(newP,buf,1,MSG_DONTWAIT|MSG_NOSIGNAL);
              if(timing){
                timeDifference(&seconds,&microSeconds,&selectTimeout);
                updateTimeouts(seconds,microSeconds, &connectedWithNoInput,&master);
                timing = 0;
               }
              if(errorCheck == -1){
                close(newP);
              }else {
                addChatterConnection(newP);
                FD_SET(newP, &master);
                connectedWithNoInput++;
                if(newP > fdMax){
                  fdMax = newP;
                }
              }
            }else{
                buf[0] = 'N';
                send(newP,buf,1,MSG_DONTWAIT|MSG_NOSIGNAL);
                close(newP);
            }
          }else if(i == o_listen){ //Activity on the observer listening socket
            blen = sizeof(oad);
      		  if((newO = accept(o_listen, (struct sockaddr *)&oad, &blen)) < 0) {
      			  fprintf(stderr, "Error: Observer accept failed\n");
      			  exit(EXIT_FAILURE);
      		  }
            printf("currently there is %d active observers\n",active_Observers);
            if(active_Observers < MAX_CLIENTS && connectedWithNoInput < MAX_CLIENTS){
               printf("connectedWithNoInput : %d",connectedWithNoInput);
               buf[0] = 'Y';
               printf("new observer accepted\n");
               if(timing){
                timeDifference(&seconds,&microSeconds,&selectTimeout);
                updateTimeouts(seconds,microSeconds ,&connectedWithNoInput,&master);
                timing = 0;
                }
               errorCheck = send(newO,buf,1,MSG_NOSIGNAL|MSG_DONTWAIT);
               if(errorCheck == -1){
                 close(newP);
               }else {
                  addObserverConnection(newO);
                  FD_SET(newO, &master);
                  connectedWithNoInput++;
                  if(newO > fdMax){
                  fdMax = newO;
                 }
               }
             }else{
               printf("observer rejected");
               buf[0] = 'N';
               send(newO,buf,1,0);
               close(newO);
             }
         } else {
                char name[25];
                if(observerOrChatter(i)){
                  //chatter
                  index = findChatter(i);
                  if(chatter_set[index].active == 0 ){
                      bytesRead = recv(i,buf,1,0);
                      printf("bytes read = %d\n",bytesRead);
                      if(bytesRead < 0){
                        printf("recv error\n");
                        printf("Exiting...\n");
                        exit(EXIT_FAILURE);
                      }else if(bytesRead == 0){ // Socket has closed
                          removeClient(i,&master,&connectedWithNoInput);
                      }
                      username_size = buf[0];
                      bytesRead = recv(i,buf,username_size,MSG_WAITALL);
                      printf("bytes read = %d\n",bytesRead);
                      snprintf(name,username_size + 1,"%s",&(buf[0]));
                      insertUsername(name,username_size,i,index,buf,&connectedWithNoInput);
                  } else {
                      uint16_t msgSize;
                      bytesRead = recv(i,&msgSize,2,0);
                      printf("bytes read = %d\n",bytesRead);
                      if(bytesRead < 0){
                        printf("recv error\n");
                        printf("Exiting...\n");
                        exit(EXIT_FAILURE);
                      }else if(bytesRead == 0){ // Socket has closed
                          removeClient(i,&master,&connectedWithNoInput);
                      }
                      msgSize = ntohs(msgSize);
                      if(msgSize > 1000 || msgSize == 0){
                        removeClient(i,&master,&connectedWithNoInput);
                      } else {
                        bytesRead = recv(i, buf, msgSize, MSG_WAITALL);
                        printf("bytes read = %d",bytesRead);
                        if(buf[0] == '@'){
                          index = findChatter(i);//sender info
                          int i = 1;
                          int j = 0;
                          while(buf[i] != ' ' && i < 11){
                            name[j] = buf[i];
                            i++;
                            j++;
                          }
                          name[j] = '\0';
                          if(buf[i] == ' '){
                          sendPrivateMsg(name,buf,index,msgSize,&connectedWithNoInput,&master);
                         }
                       }else {
                        sendPublicMsg(buf,index,msgSize,&connectedWithNoInput,&master);
                       }
                     }
                  }
                  if(timing){
                    timeDifference(&seconds,&microSeconds,&selectTimeout);
                    updateTimeouts(seconds,microSeconds ,&connectedWithNoInput,&master);
                    if(buf[0] == 'T'){
                      chatter_set[index].tv.tv_sec = WAIT_TIME;
                      chatter_set[index].tv.tv_usec = 0;
                    }
                    timing = 0;
                   }
                   printf("time left : %d\n",(int)chatter_set[index].tv.tv_sec);
              } else {
                  int bindInfo;
                  int taken = 0;
                  char name[11];
                  index = findObserver(i);
                  bytesRead = recv(i,buf,1,0);
                  username_size = buf[0];
                  if(bytesRead < 0){
                    printf("recv error\n");
                    printf("Exiting...\n");
                    exit(EXIT_FAILURE);
                  }else if(bytesRead == 0){ // Socket has closed
                      removeClient(i,&master,&connectedWithNoInput);
                  }else {
                    bytesRead = recv(i,buf,username_size,0);
                    printf("bytes read = %d\n",bytesRead);
                    snprintf(name,username_size + 1,"%s",buf);
                    bindInfo = bindToChatter(name,i,&taken,&connectedWithNoInput,&master);
                    if(bindInfo == 0){
                      if(taken != 0){
                        buf[0] = 'T';
                        printf("username taken\n");
                        errorCheck = send(i,buf,1,0);
                        if(errorCheck == -1){
                          removeClient(i,&master,&connectedWithNoInput);
                        }
                      }else {
                        printf("username rejected\n");
                        buf[0] = 'N';
                        send(i,buf,1,MSG_NOSIGNAL|MSG_DONTWAIT);
                        removeClient(i,&master,&connectedWithNoInput);
                      }
                    }else {
                        connectedWithNoInput--;
                        buf[0] = 'Y';
                        errorCheck = send(i,buf,1,MSG_NOSIGNAL|MSG_DONTWAIT);
                        if(errorCheck == -1){
                          removeClient(i,&master,&connectedWithNoInput);
                        }
                        printf("fd : [%d] connected to username [%s]\n",i,name);
                        observer_set[index].active = 1;
                     }
                     if(timing){
                        timeDifference(&seconds,&microSeconds,&selectTimeout);
                        updateTimeouts(seconds,microSeconds ,&connectedWithNoInput,&master);
                        if(buf[0] == 'T' && errorCheck != -1){
                          observer_set[index].tv.tv_sec = WAIT_TIME;
                          observer_set[index].tv.tv_usec = 0;
                        }
                        timing = 0;
                      }
                }
             }
          }
        }
      }
    }
  }
}
