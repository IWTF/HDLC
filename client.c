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
#include "global.h"
#include "HDLC_func.h"
#include "socket_IPC.h"

#define BUFFER_SIZE 1024
char buffer[BUFFER_SIZE] = {0};

int main(void) {
    int clientSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        handleError("创建socket失败");
    }
    
    connectServer(clientSocket);

    p_param *param;
    param->fd = clientSocket;
    param->dest = 1;

    pthread_t tid1; 
    pthread_create(&tid1, NULL, pthread_recv, (void *)param);


    /* 由用户选择，是否发起连接请求 */
    while(true) {
        bool connected = false;

        show_menu();
        char commend[30];
        scanf("%s", commend);
        if (commend[0] == '1') {
            // 执行HDLC发送请求
            initHDLC(clientSocket, 0);
            break;
        } else if (commend[0] == '2') {
            // 发送指定文件的内容
        } else if (commend[0] == '3') {
            // 执行拆链
        } else {
            printf("请输入正确的指令\n");
        }
    }

    pthread_t tid2;
    pthread_create(&tid2, NULL, pthread_send, (void *)param);
    pthread_join(tid1, (void *)NULL);
    pthread_join(tid2, (void *)NULL);
}