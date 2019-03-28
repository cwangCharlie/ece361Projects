/*
** selectserver.c -- a cheezy multiperson chat server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <signal.h>

#define PORT "9034"   // port we're listening on
#define MAXUSER 100
#define MAXSIZE 100

struct commandData{
    char cmd[MAXSIZE];
    char message[MAXSIZE];
    char user_name[MAXSIZE];
    
    bool firstTime;
};

struct userque
{
    char * username;
    int socket_num;
};

struct userque userlist[MAXUSER];



// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int checkValidUserName(char* userName, int fdNum){
   int socketnum = -1;
    for (int i = 0; i < MAXUSER; i++) {
        if (socketnum == -1 && userlist[i].username == NULL) {
            socketnum = i;
        }
        if(userlist[i].username == NULL){
            continue;
        }
        if (strcmp(userName, userlist[i].username) == 0) {
            return -1;
        }
    }

    if(socketnum==-1){
        printf("This server is full!\n");
        return -1;
    }


    userlist[socketnum].username = malloc(sizeof(char)*strlen(userName));
    userlist[socketnum].socket_num = fdNum;
    strcpy(userlist[socketnum].username,userName);
    printf("new client Inserted into the list\n");

    return socketnum;

}


void insertQue (int socketnum, char* userName){
    
    userlist[socketnum].username = malloc(sizeof(char)*strlen(userName));
    userlist[socketnum].socket_num = socketnum;
    strcpy(userlist[socketnum].username,userName);
    printf("new client Inserted into the list\n");
    

}

void deleteClient(int socketnum){

    for(int i = 0; i<MAXUSER; i++){
        if(userlist[i].username!=NULL){
            if(userlist[i].socket_num == socketnum){
                free(userlist[i].username);
                userlist[i].username =NULL;
                userlist[i].socket_num = -1;
            }
            
        }
    }

    return;
    
    
}

void initializeUserlist() {
    for (int i = 0; i < MAXUSER; i++) {
        userlist[i].username = NULL;
        userlist[i].socket_num = -1;
    }
    return;
}


int main(int argc, char *argv[])
{
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    struct commandData buf;    // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    initializeUserlist();

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, argv[1], &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                   
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        

                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, &buf, sizeof (buf), 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        // we got some data from a client
                        //---------------------broadcast-----------------------//
                        
                        if(strcmp(buf.cmd, "broadcast")==0){
                            if(buf.firstTime){
                               
                                if(checkValidUserName(buf.user_name,i)==-1){
                                    //error
                                    if (send(newfd, "Duplicate", strlen("Duplicate") + 1, 0) == -1) {
                                        perror("send");
                                    }
                                    close(i); // bye!
                                    FD_CLR(i, &master);
                                    continue;
                                }
                            // add the socket into the data structure 
                                //insertQue(newfd,buf->user_name);
                            //broadcast to alll other clients
                                for(j = 0; j <= fdmax; j++) {
                                    // send to everyone!
                                    if (FD_ISSET(j, &master)) {
                                        // broadcast option 
                                        if (j != listener && j != i && j!=newfd) {
                                            if (send(j, buf.user_name, strlen(buf.user_name)+1, 0) == -1) {
                                                perror("send");
                                            }
                                        }
                                    }
                                }
                            }else{
                                printf("broadcasting message from client %d\n",i);

                                for(j = 0; j <= fdmax; j++) {
                                    // send to everyone!
                                    if (FD_ISSET(j, &master)) {
                                        // broadcast option 
                                        if (j != listener && j != i) {
                                            printf("%s\n",buf.message);
                                            if (send(j, buf.message, strlen(buf.message)+1, 0) == -1) {
                                                perror("send");
                                            }
                                        }
                                    }
                                }

                            }

                           

                        }else if(strcmp(buf.cmd, "list")==0){

                            //------------------List-----------------//
                            printf("printing list for client %d\n", i);

                            char *listOfUser =malloc(sizeof("\nThe List of Users: \n"));
                            strcpy(listOfUser,"\n The List of Users: \n"); 

                            for(int i = 0; i<=MAXUSER; i++){
                                if(userlist[i].username != NULL){
                                    strcat(listOfUser,userlist[i].username);
                                    strcat(listOfUser,"\n");
                                }
                                
                            }
                            
                            if (send(i, listOfUser, strlen(listOfUser)+1, 0) == -1) {
                                perror("List Error\n");
                            }




                        }else if(strcmp(buf.cmd, "exit")==0){
                            //----------------------exit-------------//
                            if (send(i, "You have exited the chatroom.\n", strlen("You have exited the chatroom.\n")+1, 0) == -1) {
                                perror("Error\n");
                            }
                            close(i); // bye!
                            FD_CLR(i, &master); // remove from master set
                            deleteClient(i);
                            printf("CLose: %d\n", i);
                            printf("selectserver: socket %d hung up\n", i);


                        }else{
                            bool found= false;
                            for(j=0; j<=MAXUSER; j++){
                                if(userlist[j].username!=NULL){
                                    //printf("%s,%s\n",userlist[j].username,buf.cmd);
                                    if(strcmp(userlist[j].username, buf.cmd)==0){
                                        found = true;
                                        signal(SIGPIPE, SIG_IGN);
                                        if (send(userlist[j].socket_num, buf.message, strlen(buf.message) + 1, 0) == -1) {
                                            signal(SIGPIPE, SIG_IGN);
                                            perror("send");
                                            printf("Find the correct user, but cannot send\n");
                                            break;
                                        }

                                    }
                                }
                            }

                            if(!found){
                                if (send(i, "Client not found!\n", strlen("Client not found!\n") + 1, 0) == -1) {
                                            
                                            perror("send");
                                            
                                            break;
                                        }
                            }


                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
    return 0;
}

