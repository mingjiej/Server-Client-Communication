
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/uio.h>

#define SVRPORT "55000" // the server port

#define MAXDATASIZE 1000

#define MAX_CLIENTS 1000     // how many pending connections queue will hold

void *server_func(void*);

int main(int argc, char **argv)
{
  int sockfd, new_fd,yes=1;  
  struct sockaddr_in server,client;
  int sockaddr_len=sizeof(struct sockaddr_in); 
  
  extern char *optarg;
  extern int optind;
  int c;
  char *pname=SVRPORT;
  static char usage[] = "usage: %s [-p port]\n";
  
  server.sin_family=AF_INET;
  
  while ((c = getopt(argc, argv, "p:")) != -1)
    switch (c) {
    case 'p':
      pname = optarg;
      break;
    case '?':
      fprintf(stderr, usage, argv[0]);
      exit(1);
      break;
    }

  server.sin_family = AF_INET;
  server.sin_port = htons(atoi(pname));
  server.sin_addr.s_addr = INADDR_ANY;
  bzero(&server.sin_zero,8);
  
  if ((sockfd = socket(AF_INET, SOCK_STREAM,0)) == -1) {
    perror("Server: socket");
  }
  
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) { 
    perror("Server: setsockopt");
    exit(1);
  }
  if (bind(sockfd, (struct sockaddr *)&server, sockaddr_len) == -1) {
    close(sockfd);
    perror("Server: bind");
  }
  
  if (listen(sockfd, MAX_CLIENTS) == -1) {
    perror("Server: listen");
    exit(1);
  }
  
  printf("Server at %s:%s is ready.\n", inet_ntoa((server.sin_addr)), pname);
  
  while(1) {  
    
    new_fd = accept(sockfd, (struct sockaddr *)&client,(socklen_t *) &sockaddr_len);
    if (new_fd == -1) {
      perror("Server: accept");
      continue;
      
    }
    printf("Server connected to %s.\n", inet_ntoa((client.sin_addr)));
    
    //multithreaded for different client connections
    pthread_t server_thread;
    int *fdarg = (int *)malloc(sizeof(int));
    *fdarg = new_fd;

    if( pthread_create( &server_thread , NULL , server_func, fdarg) < 0)
      {
	perror("Server: could not create thread");
	return 1;
      }
  }
  
  return 0;
}

//////////////////////////////////////
// send a string ending in \n
//////////////////////////////////////
void mealysend(int sendsock, char *b, int bsize)
{
  char sendbuf[MAXDATASIZE + 3] = "";

  strncpy(sendbuf, b, MAXDATASIZE);
  strncat(sendbuf, "\n", MAXDATASIZE+2);

  if (send(sendsock,sendbuf,strlen(sendbuf), 0) == -1){
    perror("Server: send");
    exit(1);
  }
  printf("Server sent: %s",b);
  printf("  -- and it was %d bytes.\n",bsize);
  fflush(stdout);
}

//////////////////////////////////////
// receive up to the first \n
// (but omit the \n)
//	return 0 on EOF (socket closed)
//	return -1 on timeout
//////////////////////////////////////
int mealyrecvto(int recvsock, char *b, int bsize, int to)
{
  int num;
  int selectresult;
  fd_set myset;
  struct timeval tv;  // Time out

  int count = 0;
  char c = '\127';

  memset(b,0,MAXDATASIZE);
  
  while ((count < (bsize-2)) && (c != '\n') && (c != '\0')) {
    FD_ZERO(&myset);
    FD_SET(recvsock, &myset);
    tv.tv_sec = to;
    tv.tv_usec = 0;
    if ((to > 0) &&
	((selectresult = select(recvsock+1, &myset, NULL, NULL, &tv)) <= 0)) {
      // timeout happened (drop what you were waiting for -
      // if it was delimited by \n, it didn't come!
      return -1;
    }
    // got here because select returned >0, i.e., no timeout
    if ((num = recv(recvsock, &c, 1, 0)) == -1) {
      perror("Server: recv failed");
      exit(1);
    }
    if (num == 0) {
      // nothing left to read (socket closed)
      // no need to wait for a timeout; you're already done by now
      return 0;
    }
    b[count] = c;
    count++;
  }
  // at this point, either c is \n or bsize has been reached
  // so just add a string terminator
  if (count < bsize-1) {
    b[count-1] = '\0';
  } else {
    b[bsize-1] = '\0';
  }
  printf("Server received :%s",b);
  printf(": and it was %d bytes.\n", (int)strlen(b));
  return strlen(b);
}

void quitthread(char *msg, int fd) {
  printf(msg);
  fflush(stdout);
  close(fd);
  pthread_exit(NULL);
}

void *server_func(void* new)
{
  char buf1[MAXDATASIZE] = "\0";
  char buf2[MAXDATASIZE] = "SERVER GOT AND RETURNED: ";
  int *fd = (int *)new;
  int new_fd = *fd;

  free(fd);

  // wait 20 seconds or die
  if (mealyrecvto(new_fd, buf1, MAXDATASIZE, 20) < 1) {
    quitthread("Server: recv timed out and quit.\n", new_fd);
  }

  strncat(buf2, buf1, MAXDATASIZE);

  mealysend(new_fd, buf2, strlen(buf2));

  printf("Server waiting (to force client timeout)\n");
  fflush(stdout);

  sleep(10);

  quitthread("Server done.\n", new_fd);
  pthread_exit(NULL);
}



