#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

//Arbitrarily set to 10 since 10 VMS are provided
#define MAX_CLIENTS 10

void *processClient(void *arg);
void write_to_clients(const char *message, int len);

int endingServer;
int serverSocket;

int clients[MAX_CLIENTS];
int clientsConnected;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
volatile sig_atomic_t stop;


// handle ctrl + c
// Server will clean up and close
void close_server() {
    shutdown(serverSocket, SHUT_RDWR);
    int i;
    for(i = 0; i < clientsConnected; i++){
        close(clients[i]);
    }
    stop = 1;
}

int main(int argc, char **argv) {
    //signal handler for ctrl + c
    signal(SIGINT, close_server);

    //Set up server socket: sock, bind, listen accept
    int s;
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    //port is arbitrarily set to 10000
    s = getaddrinfo(NULL, "10000", &hints, &result);
    if(s != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    }

    int optval = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    if(bind(serverSocket, result->ai_addr, result->ai_addrlen) != 0){
        perror("bind()");
        exit(1);
    }

    if(listen(serverSocket, MAX_CLIENTS) != 0){
        perror("listen()");
        exit(1);
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    while(stop == 0){
        int client_fd = accept(serverSocket, NULL, NULL);
        
        //update clients connected
        pthread_mutex_lock(&mutex);
        int num_clients = clientsConnected;
        clients[clientsConnected] = client_fd;
        clientsConnected++;
        pthread_mutex_unlock(&mutex);
        pthread_t tid;

        //create thread to process clients request
        pthread_create(&tid, &attr, processClient, (void*)(intptr_t)clients[clientsConnected - 1]);
    }

    close(serverSocket);
    freeaddrinfo(result);
}

void *processClient(void *arg) {
    int client_fd = (intptr_t)arg;
    
    //buffer to receive grep command from client
    char buffer[1000];
    int bytesRead = 0;

    //Keep reading client 1 character at a time until the cleint sends a
    // newline character. Once a newline character is read, stop reading from client
    while(1){
        char tmp[1];
        int n = read(client_fd, &tmp, 1);
        if(n < 0)
            error("Error reading from client");
        else{
            if(tmp[0] == '\n'){
                buffer[bytesRead] = '\0';
                break;
            }
            buffer[bytesRead] = tmp[0];
            bytesRead += n;
        }
    }

    //redirect standard out to go to client
    dup2(client_fd, 1);
    system(buffer);

    //Client will stop reading when a (char)176 is read followed by (char)179
    char c = 176;
    char cc = 179;
    write(client_fd, &c, 1);
    write(client_fd, &cc, 1);

    //close client and update clients connected
    close(client_fd);
    pthread_mutex_lock(&mutex);
    clientsConnected--;
    pthread_mutex_unlock(&mutex);
    return NULL;
}
