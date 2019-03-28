/*
 ** client.c -- a stream socket client demo
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h> 
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 
#define STDIN 0  // file descriptor for standard input

struct send_packet {
    char cmd[MAXDATASIZE];
    char message[MAXDATASIZE];
    char user_name[MAXDATASIZE];
    bool initial;
};


bool EXIT = false;


pthread_t tid;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void * thread_stub(void * sockfd) {
     
     while (!EXIT) {
        char received[MAXDATASIZE + 1];
        int size = recv(*((int *) sockfd), received, 100, 0);
        received[size] = '\0';

        printf("You received a message: %s \n", received);

        if(strcmp(received,"Duplicate") == 0){
            EXIT = true;
            close(*((int *)sockfd));
            printf("The user has existed. Please try again\n");
            exit(0);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];


    if (argc != 4) {
        fprintf(stderr, "usage: chatclient hostname server_port user_name\n");
        exit(1);
    }


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); 


    struct send_packet packet;

    packet.initial = true;
    strcpy(packet.user_name, argv[3]);
    strcpy(packet.cmd, "broadcast");
    strcpy(packet.message, argv[3]);
    if (send(sockfd, (void *) &packet, sizeof (packet), 0) == -1) {
        perror("send");
    } else {
        printf("finish sending first messege: %s\n", argv[3]);
         packet.initial = false;
    }


    printf("Please enter you message below:\n");
    pthread_create(&tid, NULL, thread_stub, (void *) &sockfd);
    
    // fd_set readfds;
    // FD_ZERO(&readfds);
    // FD_SET(STDIN, &readfds);
    // don't care about writefds and exceptfds:
    // select(STDIN+1, &readfds, NULL, NULL, NULL);

    while (!EXIT) {
        
        char input[MAXDATASIZE];
        int input_char;
        int i;
        packet.initial = false;

        for (i = 0; (input_char = getchar()) != '\n' && input_char != EOF; i++) input[i] = input_char;
        input[i] = '\0';


        char* temp = malloc(sizeof (char) * strlen(input));
        strcpy(temp, input);

        
        char* cmd = strtok(temp, " ");
        int input_length = strlen(input);
        int cmd_length = strlen(cmd);
        
        if (cmd != NULL) {
            
            if (cmd_length == input_length) {
                strcpy(packet.cmd, input);
                packet.message[0] = '\0';
                // free(temp);
            }
            else{
                strcpy(packet.cmd, cmd);
                for (int i = cmd_length + 1; i < input_length; i++) packet.message[i - cmd_length - 1] = input[i];
                packet.message[input_length - cmd_length - 1] = '\0';
            }
        }
        // if (temp != NULL) free(temp);

        if (strcmp(packet.cmd, "exit") == 0) EXIT = true;

        strcpy(packet.user_name, argv[3]);

        if (send(sockfd, (void *) &packet, sizeof (packet), 0) == -1) perror("send");
        else printf("%s Sent Successfully\n", argv[3]);
    }

    pthread_join(tid, NULL);

    close(sockfd);

    return 0;
}

