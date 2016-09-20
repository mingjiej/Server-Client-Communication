
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <time.h>

#define BUFLEN 8192
#define SERVICE_PORT	21234
int main(void)
{
    time_t start,end;
    start = time(NULL);
    //after the user defined function does its work
    struct sockaddr_in myaddr, remaddr;
    int fd, i, slen=sizeof(remaddr);
    char buf[BUFLEN*2];	/* message buffer */
    int recvlen;		/* # bytes in acknowledgement message */
    char *server = "127.0.0.1";	/* change this to use a different server */
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 200000;
    fd_set fds;
    FD_ZERO(&fds);
    /* create a socket */
    if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
        printf("socket created\n");
    /* bind it to all local addresses and pick any port number */
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(0);
    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return 0;
    }
    FD_SET(fd,&fds);
    memset((char *) &remaddr, 0, sizeof(remaddr));
    remaddr.sin_family = AF_INET;
    remaddr.sin_port = htons(SERVICE_PORT);
    if (inet_aton(server, &remaddr.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    int nSendBufLen = 64*1024;
    /* set buffer length */
    if (setsockopt( fd, SOL_SOCKET, SO_SNDBUF, ( const char* )&nSendBufLen, sizeof( int ) )<0) {
        printf("Buffer setting(SENDING) failed!");
    }
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, ( const char* )&nSendBufLen, sizeof( int ) )<0) {
        printf("Buffer setting(RECEIVING) failed!");
    }
    int sequenceBase = 0;
    int windowsize =256;
    int rightboundary = sequenceBase+windowsize-1;
    int index = 0;
    int acknumber =0;
    int lastone =-1;
    int totalsize=10*1024*1024;
    int sendout =0;
    int received =0;
    char data[BUFLEN];
    for (;;) {
        for (; index<=rightboundary; index++) {
            memset(buf, 0, BUFLEN*sizeof(char));
            sprintf(buf, "%d", index);
            memset(data, 0, BUFLEN*sizeof(char));
            memset(data, 'a', (BUFLEN-strlen(buf))*sizeof(char));
            printf("Sending packet %d to %s port %d\n", index, server, SERVICE_PORT);
            strcat(buf,data);
            if (sendto(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, slen)==-1) {
                perror("sendto");
                exit(1);
            } else{
                sendout++;
            }
        }
        do{
            memset(buf, 0, sizeof(buf));
            printf("Receiving\n");
            /* the receive will have a time out, if time out, mean packets lose, resend for the last ack number*/
            recvlen = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, &slen);
            if (recvlen >= 0) {
                buf[recvlen] = 0;
                acknumber = atoi(buf);
                printf("received message: \"%d\"\n", acknumber);
                received++;
                totalsize=totalsize-BUFLEN;
                printf("Data: %d remained\n", totalsize);
            } //if received the message, means send complete
            else{
                index=acknumber;
                break;
            } //if not need sent the missing part
            if (totalsize<=0) {
                printf("Total packet: %d\n", sendout);
                printf("Receiced packet: %d\n", received);
                printf("number of lost packet is %d\n", sendout-received);
                end = time(NULL);
                printf("time=%f\n",difftime(end,start));
                return 0;
            } //exit the program.
        }while (acknumber<rightboundary);
        if (acknumber>=rightboundary) {
            acknumber=0;
            index=0;
            lastone=-1;
        }
    }
    return 0;
}
