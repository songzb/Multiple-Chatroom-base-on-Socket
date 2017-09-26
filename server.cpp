#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>

#define PORT "3490" // server port
#define BACKLOG 10 // pending connections queue
#define MAXDATASIZE 100
#define MAX_ROOM 20
using namespace std;



void recv(int sock);
//thread functions
void *thread_request_handler(void *ptr);
void *thread_chatroom(void *ptr);

//room struct
typedef struct{
    pthread_t tid_room;
    char room_name[256];
    int port_num;
    int num_numbers;
    bool used;
}room;
room room_db[MAX_ROOM];
int PORT_COUNT = 20000;

void sigchld_handler(int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
  int sockfd,numbytes; // sockfd->listener
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // client
  char buffer[MAXDATASIZE];
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  bool isExit = false;
  int new_fd;
  pthread_t tid_chat;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; //localhost

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // traversal and bind
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
      p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
        sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    return 2;
  }
  freeaddrinfo(servinfo); 

  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  printf("Creating chartromm\n");

  printf("server: waiting for connections...\n");

  while(1) { //main while loop for request handling
  
  sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family,
    get_in_addr((struct sockaddr *)&their_addr),
      s, sizeof s);
    printf("server: got connection from %s on socket %d\n", s, new_fd);
    //handle client request( CREATE, DELETE, JOIN)
    recv(new_fd);

}

  return 0;
}


void recv(int sock){

    pthread_t tid_init, tid_send, tid_recv;

    pthread_create(&tid_recv, NULL, thread_request_handler, &sock);
    return ;
}

/* ================================================ */

/* handle the request from clients by using pthread*/

void *thread_request_handler(void *ptr){
    char recvBuffer[MAXDATASIZE];
    /* Initialization */
    bzero(recvBuffer, sizeof(recvBuffer));
    int sock = (int)(*((int*)ptr));
    int numofbytes;
    while(1){
        if(numofbytes = recv(sock, recvBuffer, sizeof(recvBuffer), 0) > 0){
            if(strlen(recvBuffer) != sizeof(recvBuffer) - 1) recvBuffer[strlen(recvBuffer) - 1] = '\0'; // delete enter char since we use fgets as input;
            string str(recvBuffer);
            int len = strlen(recvBuffer), room_num;
            //CREATE
            if(str.find("CREATE") == 0 && recvBuffer[6] == ' ' && len > 7) {
                int count  = 0;
                string name = str.substr(7, len);
                const char *n = name.c_str();
                bool existed = false;
                //search in DB
                for(int i = 0 ; i < MAX_ROOM; i++){
                    if(strcmp(room_db[i].room_name, n) == 0 && room_db[i].used == true){ // find same name, return info to client.Manipulate DB
                        send(sock, "The name for chatroom has alredy existed.\n" , 100, 0);
                        send(sock, "Please use another name or JOIN the exist room\n", 100, 0);
                        existed = true;
                        break;
                    }
                }
                if(!existed){// Initialize a new room and run chatroom in the new thread;
                for(int i = 0; i < MAX_ROOM; i++){
                    if(room_db[i].used == false){
                        room_num = i;
                        strcpy(room_db[i].room_name, n);
                        room_db[i].port_num = ++PORT_COUNT;
                        room_db[i].num_numbers = 0;
                        room_db[i].used = true;
                        break;
                    }
                    count++;
                }
                if(count == MAX_ROOM) send(sock, "The rooms are full, please delete some\n" , 100, 0);
                else{
                send(sock, "The room have been created!!!\n" , 100, 0);
                pthread_create(&room_db[room_num].tid_room, NULL, thread_chatroom, &room_num);
                }
                }
            }
            //JOIN
        else if(str.find("JOIN") == 0 && recvBuffer[4] == ' ' && len > 5){
                string name = str.substr(5, len);
                const char *n = name.c_str();
                bool existed = false;
                send(sock, "Searching the room.......\n" , 100, 0);
                for(int i = 0 ; i < MAX_ROOM; i++){
                    if(strcmp(room_db[i].room_name, n) == 0 && room_db[i].used == true){ // find same name, join the room, manipulate DB
                        room_db[i].num_numbers++;
                        send(sock, "Room has been founded.......\n" , 100, 0);
                        string str = "JOIN ";
                        //port number
                        char port[7];
                        sprintf(port, "%d,", room_db[i].port_num);
                        //room number
                        char number[4];
                        sprintf(number, "%d", room_db[i].num_numbers);
                        char tosend[15];
                        strcat(tosend, "JOIN ");
                        strcat(tosend, port);
                        strcat(tosend, number);
                        send(sock, tosend , 100, 0);
                        existed = true;
                        break;
                    }
                }
                if(!existed) send(sock, "Room name doesn't exist\n" , 100, 0);
            }
            // DELETE
            else if(str.find("DELETE") == 0 && recvBuffer[6] == ' ' && len > 7){
                string name = str.substr(7, len);
                const char *n = name.c_str();
                bool existed = false;
                for(int i = 0 ; i < MAX_ROOM; i++){
                    if(strcmp(room_db[i].room_name, n) == 0 && room_db[i].used == true){//find the room
                        room_db[i].used = false;
                        room_db[i].num_numbers = 0;
                        existed = true;
                        send(sock, "Room has been deleted\n" , 100, 0);
                        printf("Room %s has been removed", room_db[i].room_name);
                        break;
                    }
                }
                if(!existed) send(sock, "Room name doesn't exist\n" , 100, 0);
            }
            //UNKNOWN COMMAND
            else{
                send(sock, "Unknown command\n" , 100, 0);
            }
        }
        else if(numofbytes == 0){
            printf("Server: socket %d hung up\n",sock);
            close(sock);
            break;
        }
        else{
            perror("recv");
        }
        bzero(recvBuffer, sizeof(recvBuffer));
        fflush(stdout);
    }

    //return ;
}
/*make a new thread for "CREATE" -> a new room(new master socket)*/
/*use select to foward message*/
void *thread_chatroom(void *ptr){
    fd_set master;   // master file descriptor
    fd_set read_fds;  // temporary for reading
    int fdmax;  // curretn maximum

    int listener; // Master socket
    int newfd;  // Slave Socket
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[256];  
    int nbytes;
    int room_number = (int)(*((int*)ptr));
    int port = room_db[room_number].port_num;
    char po[6];
    char txt[100];
    sprintf(po, "%d", port);
    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1; 
    int i, j, rv;

    struct addrinfo hints, *ai, *p;
    struct timeval tv;
    FD_ZERO(&master);  // erase master and read sets
    FD_ZERO(&read_fds);

    //bind master socket, set it up
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    tv.tv_sec = 2;
    tv.tv_usec = 500000;

    if ((rv = getaddrinfo(NULL, po, &hints, &ai)) != 0) {
      fprintf(stderr, "Chatroom: %s\n", gai_strerror(rv));
      exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next) {
      listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (listener < 0) {
        continue;
      }

      // "address already in use"
      //setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

      if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
        close(listener);
        continue;
      }

      break;
    }
    // check if we failed when bind
    if (p == NULL) {
      fprintf(stderr, "Chatroom: failed to bind\n");
      exit(2);
    }
    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
      perror("listen");
      exit(3);
    }

   
    FD_SET(listener, &master);

    
    fdmax = listener;

    // Main loop for select, forward all msgs get from clients.
    printf("Room %s is running\n", room_db[room_number].room_name);
    for( ; ; ) {
        //if this room has been delete, send alert to every one in the room. close the socket.
        if(!room_db[room_number].used){
            for(j = 0; j <= fdmax; j++) {
                //Do not need to inform master socket
              if (FD_ISSET(j, &master)) {
                if (j != listener) {
                  if (send(j, "This chat room has been deleted, shutting down connection\n", 100, 0) == -1) {
                    perror("send");
                  }
                  if (send(j, "Please use \\quit to return to command mode\n", 100, 0) == -1) {
                    perror("send");
                  }
                }
              }
            }
            close(listener);
            break;
        }
      read_fds = master; 
      if (select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1) {
        perror("select");
        continue;
      }

      //if we get some msgs
      for(i = 0; i <= fdmax; i++) {
        if (FD_ISSET(i, &read_fds)) { 
          if (i == listener) {
            // handle new connections
            addrlen = sizeof remoteaddr;
            newfd = accept(listener,
              (struct sockaddr *)&remoteaddr,
              &addrlen);

            if (newfd == -1) {
              perror("accept");
            } else {
              FD_SET(newfd, &master); 
              if (newfd > fdmax) { 
                fdmax = newfd;
              }
              printf("Chatroom %s: new connection from %s on "
                "socket %d\n", room_db[room_number].room_name,
                inet_ntop(remoteaddr.ss_family,
                  get_in_addr((struct sockaddr*)&remoteaddr),
                  remoteIP, INET6_ADDRSTRLEN),
                newfd);
            }
            
            sprintf(txt,"Socket %d join in, the current number of members is %d\n", newfd, room_db[room_number].num_numbers);
            //send info to the members if someone quit
            for(j = 0; j <= fdmax; j++) {
                // forward to all clients
              if (FD_ISSET(j, &master)) {
                if (j != listener && j != i) {
                  if (send(j,txt, 100, 0) == -1) {
                    perror("send");
                  }
                }
              }
            }

          } else {
            // handle the msgs from clients
            if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
              // got error or connection closed by client
              if (nbytes == 0) {
                // shut down connection
                printf("Chatroom %s: socket %d hung up\n", room_db[room_number].room_name,i);
                room_db[room_number].num_numbers--;
              } else {
                perror("recv");
              }
              sprintf(txt,"Socket %d quit, the current number of members is %d\n", i, room_db[room_number].num_numbers);
              //send info to the members if someone quit
              for(j = 0; j <= fdmax; j++) {
                  // forward to all clients
                if (FD_ISSET(j, &master)) {
                  if (j != listener && j != i) {
                    if (send(j,txt, 100, 0) == -1) {
                      perror("send");
                    }
                  }
                }
              }
              close(i); 
              FD_CLR(i, &master); 
              
            } else {
              for(j = 0; j <= fdmax; j++) {
                  // forward to all clients
                if (FD_ISSET(j, &master)) {
                  if (j != listener && j != i) {
                    if (send(j, buf, nbytes, 0) == -1) {
                      perror("send");
                    }
                  }
                }
              }
            }
          }
        }
      }
    } 
    return NULL;
}
