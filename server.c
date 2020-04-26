#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>

#include "tools.h"
#include "socket_IPC.h"

#define BACK_LOG 1000

int main(void) {
    int serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        handleError("创建socket失败");
    }
    bindToAddress(serverSocket);
    if (listen(serverSocket, BACK_LOG) == -1) {//转为被动模式
        handleError("监听失败");
    }
    while (true) {
        handleRequest(serverSocket);
    }
}