#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "tools.h"
#include "socket_IPC.h"

#define BUFFER_SIZE 1024
char buffer[BUFFER_SIZE] = {0};

int main(void) {
    int clientSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        handleError("创建socket失败");
    }
    
    connectServer(clientSocket);

    pthread_t tid1; 
    pthread_t tid2;
    pthread_create(&tid1, NULL, pthread_recv, (void *)&clientSocket);
    pthread_create(&tid2, NULL, pthread_client_send, (void *)&clientSocket);
    pthread_join(tid1, (void *)NULL);
    pthread_join(tid2, (void *)NULL);
}