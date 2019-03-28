#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#define SERVERPORT "4950"    // the port users will be connecting to


#define MAXBUFLEN 100

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct timeval beforeSend;
    struct timeval afterSend;
    struct timeval afterReceive;

    char buf[MAXBUFLEN];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;

    if (argc != 3) {
        fprintf(stderr,"usage: talker hostname message\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
   }


    gettimeofday(&beforeSend, NULL);
    // long beforeSendto = (long)TIMEER.tv_usec;
    // printf("Sendto Before: %ld\n", beforeSendto);

    numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0, p->ai_addr, p->ai_addrlen);

    gettimeofday(&afterSend, NULL);
    // long afterSendto = (long)afterSend.tv_usec;
    // printf("Sendto After: %ld\n", afterSendto);

    if (numbytes == -1) {
        perror("talker: sendto");
        exit(1);
    }
    printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);


    addr_len = sizeof their_addr;
     if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
        }

    gettimeofday(&afterReceive, NULL);
    // long afterRecvfrom = (long)TIMEER.tv_usec;
    // printf("After Recvfrom: %ld\n\n", afterRecvfrom);


    

    freeaddrinfo(servinfo);

    long TransmissionTime = (afterReceive.tv_sec - beforeSend.tv_sec)*1000000L +afterReceive.tv_usec - beforeSend.tv_usec;

    printf("\n");
    printf("Entire Transmission Time: %ld\n", TransmissionTime);
    
    // long FrameDelay = (afterSend.tv_sec - beforeSend.tv_sec)*1000000L +afterSend.tv_usec - beforeSend.tv_usec;
    // long PropagationDelay = (TransmissionTime - 2 * FrameDelay) / 2;
    // printf("Frame Delay: %ld\n", FrameDelay);
    // printf("Propagation Delay: %ld\n", PropagationDelay);

    printf("\n");

    close(sockfd);

    return 0;
}
