#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <netdb.h>
#include<fstream>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAXDATASIZE 100
using namespace std;

void send_recv_command(void);
void *thread_send(void *ptr);
void *thread_recv(void *ptr);

void send_recv_chat(char *c_port, char *c_num);
void *thread_send_chat(void *ptr);
void *thread_recv_chat(void *ptr);

int sockfd;
bool join_room = false, join_chat = false;
string port, number;
char c_port[6], c_num[5];
struct addrinfo hints, *servinfo,*p;

pthread_t tid_send, tid_recv;

//transferorm std::string to char*
void stoc(char *p, string str){
        for(int i=0;i<str.length();i++)
            p[i] = str[i];
        p[str.length()] = '\0'; 
}
int set_socket(char *addr, char *port){
    //struct addrinfo *p;
    int rv;
    if ((rv = getaddrinfo(addr, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
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
            exit(0);
        return 2;
    }
	return sockfd;
}

int main(int argc, char *argv[])
{
	int numbytes;
	char buffer[MAXDATASIZE];
	int rv;

	if (argc != 3) {
		fprintf(stderr,"usage: client hostname and port number\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

   set_socket(argv[1], argv[2]);

    while(1){
        system("clear");
        printf("==========You are now in command mode===========\n");
        printf("==========Please use \"CREATE\", \"DELTE\" and \"JOIN\"\n");
        
        send_recv_command();

        printf("Connecting to ChatRoom");
        stoc(c_port, port);
        
        set_socket(argv[1], c_port);
        
        stoc(c_port, port);
        stoc(c_num, number);
        
        system("clear");
        send_recv_chat(c_port,c_num);

        set_socket(argv[1], argv[2]);
        
     }
    printf("press any key to quit\n");
	close(sockfd);
	return 0;
}

// =========================================
// handle input and ouput for command
void send_recv_command(void){

    pthread_create(&tid_recv, NULL, thread_recv, NULL);
    pthread_create(&tid_send, NULL, thread_send, NULL);

    pthread_join(tid_recv, NULL);
    return ;
}

void *thread_send(void *ptr){

    char sendBuffer[MAXDATASIZE + 24];
    // send command to server
    bzero(sendBuffer, sizeof(sendBuffer));

    while(!join_room){
        fgets(sendBuffer, MAXDATASIZE + 24, stdin);
        if(strcmp(sendBuffer,"exit\n") == 0) exit(0);
        send(sockfd, sendBuffer, strlen(sendBuffer), 0);
        bzero(sendBuffer, sizeof(sendBuffer));
        fflush(stdin);
    }

    return NULL;
}


void *thread_recv(void *ptr){

    char recvBuffer[MAXDATASIZE];

    bzero(recvBuffer, sizeof(recvBuffer));

    while(!join_room){
        
        // receive msgs
        if(recv(sockfd, recvBuffer, sizeof(recvBuffer), 0) > 0){

            string str(recvBuffer);
            // if we receive the specific msg from server("JOIN <portnumber>,<num of people in chat room>"),  client need to close the old socket and connect to the lisetning socket for chat room
            int index = str.find("JOIN");
            if(index == 0){
                int split = str.find(',');
                port = str.substr(5, split - 5);
                number = str.substr(split + 1, strlen(recvBuffer));
                pthread_cancel(tid_send);// kill sned thread(otherwise may be stucked by fgets());
                close(sockfd);
                join_room = true;
                join_chat = false;
            }
            else{
                printf("%s", recvBuffer);
            }
        }
        bzero(recvBuffer, sizeof(recvBuffer));
        fflush(stdout);
    }
    return NULL;
}

//============================================

// ============================================
// 	handler for chat mode
// we use two threads to handle the send and receive fuction in each client
void send_recv_chat(char *c_port, char *c_num){
    fflush(stdout);
    pthread_t tid_send_c, tid_recv_c;
    
    // create two threads(send, receive)
    // block send thread since we need to exit when user type \quit
    pthread_create(&tid_recv_c, NULL, thread_recv_chat, NULL);
    pthread_create(&tid_send_c, NULL, thread_send_chat, NULL);

    pthread_join(tid_send_c, NULL);

    return ;
}


void *thread_send_chat(void *ptr){
    char sendBuffer[MAXDATASIZE + 24];

    bzero(sendBuffer, sizeof(sendBuffer));

    while(1){
        // wait for client to inout
        fgets(sendBuffer, MAXDATASIZE + 24, stdin);
        //cheack whether the user need to back to command mode(by \quit);
        if(strcmp(sendBuffer, "\\quit\n") == 0){
            join_chat = true;
            join_room = false;
            close(sockfd);
            break;
        }
        
        //send msgs
        if(send(sockfd, sendBuffer, strlen(sendBuffer), 0) <= 0){
            perror("send");
            printf("Chat room has been deleted\n");
        }

        bzero(sendBuffer, sizeof(sendBuffer));
        fflush(stdin);
    }

    return NULL;
}


void *thread_recv_chat(void *ptr){
    
    printf("\n=============    Hello,  Welcome to Chatroom at PORT: %s, current number of people is  %s   =============\n\n\n\n > ", c_port, c_num);
    char recvBuffer[MAXDATASIZE];

    bzero(recvBuffer, sizeof(recvBuffer));

    // receive msgs from chat socket
    while(!join_chat){
        if(recv(sockfd, recvBuffer, sizeof(recvBuffer), 0) > 0){
			if(strlen(recvBuffer) != sizeof(recvBuffer) - 1) recvBuffer[strlen(recvBuffer) - 1] = '\0'; // delete enter char
            printf("%s\n", recvBuffer);
        }

        bzero(recvBuffer, sizeof(recvBuffer));
        fflush(stdout);
    }

    return NULL;
}
