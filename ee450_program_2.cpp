//
//  main.cpp
//  EE450 pro2
//
//  Created by User on 3/17/15.
//  Copyright (c) 2015 User. All rights reserved.
//

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

#define SVRPORT "60450,"
#define SVRNAME "DNS.POSTEL.ORG"

#define MAXDATASIZE 500

void paddr(unsigned char *a)
{
    printf("The server address is:%d.%d.%d.%d\n", a[0], a[1], a[2], a[3]);
    
}
int main(int argc,  char *argv[]) {
    int sockfd;
    struct sockaddr_in myaddr;
    struct hostent *hp;
    struct sockaddr_in serveraddr;
    int myport=50000;
    int serverport;
    char server[] = "128.9.112.1";
    char register_message[] = "FM:00 TO:01 NO:000 NP:3 DATA:register me";
    char data[500];
    fd_set myset;
    struct timeval tv;
    FD_ZERO(&myset);
    FD_SET(sockfd, &myset);
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    char ipmap[500];
    int recvlen;
    unsigned int alen;	/* length of address (for getsockname) */
    char buf[MAXDATASIZE];
    //get port number and DNS address from argument.
    extern char *optarg;
    extern int optind;
    int c;
    char *sname=SVRNAME, *pname = SVRPORT;
    static char usage[] = "usage: %s [-s sname] [- p pname]\n";
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
    // get a socket
    if ((sockfd = socket(AF_INET,SOCK_DGRAM,0)) == -1) {
        perror("Client: could not create socket");
        exit(-1);
    }
    // get information for myaddr struct
    memset((void *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(myport);
    //bind socket
    if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return 0;
    }
    //get socket name and print out the port number
    alen = sizeof(myaddr);
    if (getsockname(sockfd, (struct sockaddr *)&myaddr, &alen) < 0) {
        perror("getsockname failed");
        return 0;
    }
    printf("bind complete. Port number = %d\n", ntohs(myaddr.sin_port));
    /* fill in the server's address and data */
    memset((char*)&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(atoi(pname));
    /* look up the address of the server given its name, and print that ip address */
    hp = gethostbyname(sname);
    if (!hp) {
        fprintf(stderr, "could not obtain address of %s\n", sname);
        return 0;
    }
    else{
        printf("The host name is: %s\n",sname);
    }
    for (int i=0; hp->h_addr_list[i] != 0; i++)
    {
        paddr((unsigned char*) hp->h_addr_list[i]);
    }
    /* put the host's address into the server address structure */
    memcpy((void *)&serveraddr.sin_addr, hp->h_addr_list[0], hp->h_length);
    inet_aton(server, &serveraddr.sin_addr);
    /* send a message to the server */
    if (sendto(sockfd, register_message, strlen(register_message), 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("sendto failed");
        return 0;
    }
    else{
        printf("Client has sent: %s\n",register_message);
    }
    /* receive the register number */
    for (;;) {
        printf("waiting on port %s\n", pname);
        recvlen = recvfrom(sockfd, buf, MAXDATASIZE, 0, (struct sockaddr *)&serveraddr, &alen);
        printf("received %d bytes\n", recvlen);
        if (recvlen > 0) {
            buf[recvlen] = 0;
            printf("received message: \"%s\"\n", buf);
            break;
        }
        else{
            perror("receive message error.");
        }
    }
    // store the register number for making message to send
    char reg_number[3];
    reg_number[0]=buf[57];
    reg_number[1]=buf[58];
    reg_number[2]='\0';
    //ask user to operate.
    printf("What do you want to do?\n");
    printf("Type s to send message, type m to get map, type w to wait for any message, type type n to end.\n");
    char answer = getchar();
    /* get map */
    if (answer=='m') {
        //make a message for get map
        char map_message[500];
        sprintf(map_message, "FM:%s TO:01 NO:000 NP:4 DATA:GET MAP",reg_number);
        recvlen=0;
        //receive and print message
        for(;recvlen==0;){
            if (sendto(sockfd, map_message, strlen(map_message), 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
                perror("sendto failed");
                return 0;
            }
            else{
                printf("Client has sent: %s\n", map_message);
            }
            recvlen = recvfrom(sockfd, buf, MAXDATASIZE, 0, (struct sockaddr *)&serveraddr, &alen);
        }
        buf[recvlen] = 0;
        printf("received message: \"%s\"\n", buf);
        printf("Do you want to boardcast?\n");
        /*ask user if they want to boardcast, if they want to, pick out useful ip address and port number, and send message. Then check received message to see if they need to be relayed.*/
        if (getchar()=='y') {
            /*ready to boardcast*/
            char address[20];
            int count = 0;
            char temp[1];
            char port[5];
            printf("please type in you data.\n");
            scanf("%s",&data);
            char boardcast_message[500];
            //make boardcast message.
            sprintf(boardcast_message, "FM:%s TO:99 NO:000 NP:9 HC:05 VL:%s DATA:%s",reg_number,reg_number,data);
            strcpy(ipmap, buf);
            memset(buf, 0, sizeof(buf));
            recvlen=0;
            //catch ip address and port number for message, and send message
            for(int i=30;i<500;i++)
            {
                if (ipmap[i]=='=') {
                    for(int j=i+1;j<500;j++,i++)
                    {
                        if(ipmap[j]!='@')
                        {
                            sprintf(temp,"%c",ipmap[j]);
                            strcat(address,temp);
                            
                        }
                        else
                        {
                            printf("the address is: %s\n",address);
                            for(int k=j+1;k<j+6;k++)
                            {
                                sprintf(temp,"%c",ipmap[k]);
                                strcat(port,temp);
                            }
                            i=j+6;
                            printf("the port is: %s\n",port);
                            break;
                        }
                        
                    }
                }
                //if address and port is ready, use them to send message.
                if(strlen(address)!=0&&strlen(port)==5)
                {
                    hp = gethostbyname(address);
                    serveraddr.sin_port = htons(atoi(port));
                    if (sendto(sockfd, boardcast_message, strlen(boardcast_message), 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
                        perror("sendto failed");
                        return 0;
                    }
                    else{
                        printf("Client has sent: %s\n",boardcast_message);
                        //reset address and port.
                        memset(address, 0, sizeof(address));
                        memset(port, 0, sizeof(port));
                    }
                    select(sockfd+1, &myset, NULL, NULL, &tv);
                    //                if (FD_ISSET(sockfd,&myset)) {
                    recvlen = recvfrom(sockfd, buf, MAXDATASIZE, 0, (struct sockaddr *)&serveraddr, &alen);
                    printf("received %d bytes\n", recvlen);
                    if (recvlen > 0) {
                        buf[recvlen] = 0;
                        printf("received message: \"%s\"\n", buf);
                        //check for relay
                        if (buf[22]=='9') {
                            if (buf[9]!=reg_number[0]||buf[10]!=reg_number[1]) {
                                printf("Received message not for you, check if it can be relayed\n");
                                if(buf[28]!=0)
                                {
                                    printf("It can, start relay.\n");
                                    int i= atoi(&buf[28]);
                                    buf[28]='0'+i;
                                    strcat(buf, reg_number);
                                    //send it back to everyone in the list from GET MAP
                                    for(int i=30;i<500;i++)
                                    {
                                        if (ipmap[i]=='=') {
                                            for(int j=i+1;j<500;j++,i++)
                                            {
                                                if(ipmap[j]!='@')
                                                {
                                                    sprintf(temp,"%c",ipmap[j]);
                                                    strcat(address,temp);
                                                    
                                                }
                                                else
                                                {
                                                    printf("the address is: %s\n",address);
                                                    for(int k=j+1;k<j+6;k++)
                                                    {
                                                        sprintf(temp,"%c",ipmap[k]);
                                                        strcat(port,temp);
                                                    }
                                                    i=j+6;
                                                    printf("the port is: %s\n",port);
                                                    break;
                                                }
                                                
                                            }
                                        }
                                        //if address and port is ready, use them to send message.
                                        if(strlen(address)!=0&&strlen(port)==5)
                                        {
                                            hp = gethostbyname(address);
                                            serveraddr.sin_port = htons(atoi(port));
                                            if (sendto(sockfd, boardcast_message, strlen(boardcast_message), 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
                                                perror("sendto failed");
                                                return 0;
                                            }
                                            else{
                                                printf("Client has sent: %s\n",boardcast_message);
                                                //reset address and port.
                                                memset(address, 0, sizeof(address));
                                                memset(port, 0, sizeof(port));
                                            }
                                            
                                        }
                                    }
                                }
                                else
                                {
                                    printf("dropped-hopcount exceeded.\n");
                                }
                            }
                            else
                            {
                                printf("No need for relay, it is for you.\n");
                            }
                        }
                        break;
                    }
                    else{
                        perror("receive message error.");
                    }
                    buf[recvlen] = 0;
                    //                }
                }
            }

        }
    }
    ///////////////wait for a message
    if (answer=='w') {
        do{
             printf("Keep waiting....");
            //set a time limit for receive message, and keep looping.
             if (FD_ISSET(sockfd, &myset)) {
                if(recvfrom(sockfd, buf, MAXDATASIZE, 0, (struct sockaddr *)&serveraddr, &alen)<0){
                     perror("sendto failed");
                     return 0;
                }
             }
            }
        while(recvfrom(sockfd, buf, MAXDATASIZE, 0, (struct sockaddr *)&serveraddr, &alen)<=0);
        char to[3];
        char np_code[2];
        to[0]=buf[9];
        to[1]=buf[10];
        to[2]='\0';
        np_code[0]=buf[22];
        np_code[1]='\0';
        if(strcmp(to, reg_number)==0&&strcmp(np_code, "7")==0)
        {
            printf("received message: \"%s\"\n", buf);
        }
        
    }
    //ask user to in put a destination code and data, then send it
    if (answer=='s') {
        printf("please type in your destination code.\n");
        char dest[2];
        scanf("%s",&dest);
        char send_message[500];
        memset(send_message, 0, sizeof(send_message));
        printf("please type in your data.\n");
        scanf("%s",&data);
        sprintf(send_message, "FM:%s TO:%s NO:000 NP:7 DATA:%s.",reg_number,dest,data);
        if(sendto(sockfd, send_message, strlen(send_message), 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
            perror("sendto failed");
            return 0;
        }
        else{
            printf("Client has sent: %s\n",send_message);
        }
    }
    if (answer=='n') {
        exit(0);
    }
}

