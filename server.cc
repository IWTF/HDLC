#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <sys/sem.h>

#include "tools.h"
#include "socket_IPC.h"
#include "HDLC_lib.h"
#include "signal_lib.h"

#define BACK_LOG 1000
int isConnect = 0;

/* 信号量标识符 */
int s_sem_id1;
int s_sem_id2;

int main(void)
{
    /* 创建k1,k2信号量，获得其信号量标识符 */
    s_sem_id1 = semget((key_t)k1, 1, 0666 | IPC_CREAT);
    s_sem_id1 = semget((key_t)k2, 1, 0666 | IPC_CREAT);

    /* 给信号量赋初值, 一定要释放，不然信号量会一直存在！！！ */
    // if(!set_semvalue(s_sem_id1) || !set_semvalue(s_sem_id2)) {
    //     fprintf(stderr, "Failed to initialize semaphore\n");
    //     exit(EXIT_FAILURE);
    // }

    int serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        handleError((char *)"创建socket失败");
    }
    bindToAddress(serverSocket);
    if (listen(serverSocket, BACK_LOG) == -1)
    { //转为被动模式
        handleError((char *)"监听失败");
    }
    int recvsocket = handleRequest(serverSocket);

    pthread_param params;
    initParam(&params, recvsocket, AddrServer, AddrClient);

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
                printf("您已建立HDLC链接，无需重复建立\n");
                break;
            }
            isConnect = 1;
            HDLC_request(params, SABM);
            printf("HDLC建立连接成功！！\n");
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
            server_run(recvsocket);
            break;
        case 6:
            HDLC_response(params, DISC);
            printf("HDLC拆链成功！！\n");
            isConnect = 0;
            break;
        case 7:
            del_semvalue(s_sem_id1);
            del_semvalue(s_sem_id2);
            break;
        default:
            printf("输入有误,请重新输入\n");
        }
    }

    return 0;
}