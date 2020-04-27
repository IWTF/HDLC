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

    return 0;
}