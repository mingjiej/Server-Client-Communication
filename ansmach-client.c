
/*
 ** client.c -- a stream socket client demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <fcntl.h>

#define SVRPORT "55002"
#define SVRNAME "localhost"

#define MAXDATASIZE 1000

//////////////////////////////////////
// send a string ending in \n
//////////////////////////////////////
void mealysend(int sendsock, char *b, int bsize)
{
    char sendbuf[MAXDATASIZE + 3] = "";
    
    strncpy(sendbuf, b, MAXDATASIZE);
    strncat(sendbuf, "\n", MAXDATASIZE+2);
    
    if (send(sendsock,sendbuf,strlen(sendbuf), 0) == -1){
        perror("Client: send");
        exit(1);
    }
    printf("Client sent: %s",b);
    printf("  -- and it was %d bytes.\n",bsize);
    fflush(stdout);
}

//////////////////////////////////////
// receive up to the first \n
// (but omit the \n)
//  return 0 on EOF (socket closed)
//  return -1 on timeout
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
            perror("Client: recv failed");
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
    printf("Client received :%s",b);
    printf(": and it was %d bytes.\n", (int)strlen(b));
    return strlen(b);
}

void mealy(int msock)
{
    char buf[MAXDATASIZE] = "";
    char data[MAXDATASIZE];
    //string to store user input data.
    int numbytes;
    char copy[2];
    //a new string which is used to copy number client mechine received at the and of "CODE "
    char ch;
    //an int to get number user input.
    char buffrecv[MAXDATASIZE] = "";
    // make a new char string to store recivied content.
    printf("Do you want to make a call? Type y to call.\n");
    ch = getchar();
    if(ch=='y')
    {
        printf("Now you are in Call state.\n");
        //keep sending ring until server send message.
        strcpy(buf,"RING");
        do{
            mealysend(msock,buf,strlen(buf));
        }
        while (mealyrecvto(msock, buffrecv, MAXDATASIZE, 5)==-1);
        strcpy(buf,"HELLO");
        mealysend(msock,buf,strlen(buf));
        printf("Now you are in Talk state.\n");
        printf("Type in your data, type n to cancel.\n");
        while(scanf("%s", &data)==1)
        {
            if(data[0]>='0'&&data[0]<='9'&&data[1]=='\0')
            {
                //if user only input one number, copy it into copy[] and put them at the end of "CODE "
                copy[0]=data[0];
                copy[1]='\0';
                strcpy(buf,"GET");
                mealysend(msock,buf,strlen(buf));
                printf("Now you are in Check state.\n");
                strcpy(buf,"CODE ");
                strcat(buf, copy);
                mealysend(msock,buf,strlen(buf));
                printf("Now you are in Try state.\n");
                numbytes=mealyrecvto(msock, buffrecv, MAXDATASIZE, 5);
                //Based on what client receive, decide to do what next
                while (!strcmp(buffrecv,"NO"))
                {
                    //if "NO" is received, ask user try another number.
                    printf("Return to Check state.\n");
                    printf("Please try another number.\n");
                    scanf("%s",&copy);
                    printf("Client input: %s.\n", copy);
                    strcpy(buf,"CODE ");
                    strcat(buf, copy);
                    mealysend(msock,buf,strlen(buf));
                    numbytes=mealyrecvto(msock, buffrecv, MAXDATASIZE, 5);
                }
                if(numbytes>4)
                {
                    //if the length of buffrecv is longer than 4, is must be a "MSG XXXXXXXXX", so send "NOTED".
                    strcpy(buf,"NOTED");
                    mealysend(msock,buf,strlen(buf));
                    strcpy(buf,"FINISHED");
                    mealysend(msock,buf,strlen(buf));
                    break;
                    
                }
                if(!strcmp(buffrecv, "FAIL"))
                {
                    //if "FAIL" is received, send "QUIT".
                    strcpy(buf,"QUIT");
                    mealysend(msock,buf,strlen(buf));
                    break;
                }
            }
            else if(!strcmp(data,"n"))
            {
                //if client send "n", means they don't want to send any data, return to idle.
                strcpy(buf,"FINISHED");
                mealysend(msock,buf,strlen(buf));
                break;
            }
            else
            {
                do{
                    // if user don't send "n" or number which means they send regular data, keep let them send until they say "n".
                    if (strcmp(data,"n")) {
                        strcpy(buf,"DATA ");
                        strcat(buf, data);
                        printf("User input: %s\n", data);
                        mealysend(msock,buf,strlen(buf));
                        printf("Any more data?");
                    }
                    else
                    {
                        strcpy(buf,"FINISHED");
                        mealysend(msock,buf,strlen(buf));
                        break;
                    }
                }
                while(scanf("%s",&data)==1);
                break;
            }
        }
        printf("Now you are in Hangup state.\n");
        numbytes = mealyrecvto(msock, buffrecv, MAXDATASIZE, 5);
        strcpy(buf,"BYE");
        mealysend(msock,buf,strlen(buf));
        // everything is done, ending three ways handshake.
    }
    printf("You are back to Idle state.\n");
    ////// Receive - either one works
    numbytes = mealyrecvto(msock, buf, MAXDATASIZE, 5);
    // numbytes = mealyrecvto(msock, buf, MAXDATASIZE, 0);
    ////// Receive with timeout (returns 0 on timeout)
    numbytes = mealyrecvto(msock, buf, MAXDATASIZE, 5);
    if (numbytes == -1) {
        printf("Client timeout.\n");
    } else if (numbytes == 0) {
        printf("Client stop; server socket closed.\n");
    }
}

int main(int argc, char *argv[])
{
    int sockfd, sen, valopt;
    
    struct hostent *hp;
    struct sockaddr_in server;
    socklen_t lon;
    
    extern char *optarg;
    extern int optind;
    int c;
    char *sname=SVRNAME, *pname = SVRPORT;
    static char usage[] = "usage: %s [-s sname] [- p pname]\n";
    
    long arg;
    fd_set myset;
    struct timeval tv;  // Time out
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    
    // parse the command line arguments
    while ((c = getopt(argc, argv, "s:p:")) != -1)
        switch (c) {
            case 'p':
                pname = optarg;
                break;
            case 's':
                sname = optarg;
                break;
            case '?':
                fprintf(stderr, usage, argv[0]);
                exit(1);
                break;
        }
    
    // convert the port string into a network port number
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(pname));
    
    // convert the hostname to a network IP address
    hp = gethostbyname(sname);
    if ((hp = gethostbyname(sname)) == NULL) {  // get the host info
        herror("Client: gethostbyname failed");
        return 2;
    }
    bcopy(hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
    bzero(&server.sin_zero, 8);
    
    // get a socket to play with
    if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
        perror("Client: could not create socket");
        exit(-1);
    }
    
    //////////////////////////////////////
    // open the connection
    
    // set the connection to non-blocking
    arg = fcntl(sockfd, F_GETFL, NULL);
    arg |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, arg);
    //
    if ((sen = connect(sockfd,(struct sockaddr *)&server,sizeof(server))) == -1) {
        if (errno == EINPROGRESS) {
            FD_ZERO(&myset);
            FD_SET(sockfd, &myset);
            if (select(sockfd + 1, NULL, &myset, NULL, &tv) > 0){
                lon = sizeof(int);
                getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon);
                if (valopt) {
                    fprintf(stderr, "Client error in connection() %d - %s\n", valopt, strerror(valopt));
                    exit(-1);
                }
            }
            else {
                perror("Client: connection time out");
                exit(-1);
            }
        }
        else {
            fprintf(stderr, "Client error connecting %d - %s\n", errno, strerror(errno));
            exit(0);
        }
    }
    //
    ///////////////////////////////
    
    // Set to blocking mode again
    arg = fcntl(sockfd, F_GETFL, NULL);
    arg &= (~O_NONBLOCK);
    fcntl(sockfd, F_SETFL, arg);
    
    //////////////////////////////
    
    mealy(sockfd);
    // and now it's done
    //////////////////////////////
    
    close(sockfd);
    return 0;
}