#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sem.h>

#include "tools.h"
#include "socket_IPC.h"
#include "HDLC_lib.h"
#include "signal_lib.h"

char buffer[BUFFER_SIZE] = {0};
int isConnect = 0; //标记是否建立链接

/* 信号量标识符 */
int c_sem_id1;
int c_sem_id2;

int main(void)
{
    /* 创建k1,k2信号量，获得其信号量标识符 */
    c_sem_id1 = semget((key_t)k1, 1, 0666 | IPC_CREAT);
    c_sem_id1 = semget((key_t)k2, 1, 0666 | IPC_CREAT);

    int clientSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        handleError((char *)"创建socket失败");
    }

    connectServer(clientSocket);

    pthread_param params;
    initParam(&params, clientSocket, AddrClient, AddrServer);

    while (true)
    {
        printMenu();
        int choice;
        scanf("%d", &choice);
        switch (choice)
        {
        case 1:
            if (isConnect)
            {
                printf("您已建立HDLC链接，无需重新建立\n");
                break;
            }
            HDLC_request(params, SABM);
            printf("HDLC建立连接成功！！\n");
            isConnect = 1;
            break;
        case 2:
            HDLC_response(params, SABM);
            printf("HDLC建立连接成功！！\n");
            isConnect = 1;
            break;
        case 3:
            HDLC_request(params, DISC);
            printf("HDLC拆链成功！！\n");
            isConnect = 0;
            break;
        case 4:

            break;
        case 5:
            client_run(clientSocket);
        case 6:
            HDLC_response(params, DISC);
            printf("HDLC拆链成功！！\n");
            isConnect = 0;
            break;
        case 7:
            del_semvalue(c_sem_id1);
            del_semvalue(c_sem_id2);
            break;
        default:
            printf("输入有误\n");
        }
    }

    return 0;

    /*
    FILE* fp=fopen("../build/message.txt","r");
    if(fp == NULL)
    {
        handleError("file open error");
    }

    

    while(true) {
        if(fgets(buffer, BUFFER_SIZE, fp)==NULL)
            break;
        
        if(send(clientSocket, buffer, strlen(buffer), 0)==-1) {
            handleError("发送失败");
        }
        int numOfReaded = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if(numOfReaded==-1) {
            handleError("对端已经关闭");
        }
        buffer[numOfReaded]=0;
        printf("%s", buffer);
    }
*/
}