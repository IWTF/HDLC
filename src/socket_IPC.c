#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "global.h"
#include "tools.h"
#include "socket_IPC.h"

char path[] = "./namo_amitabha"; /* unix socket 通信使用的系统路径 */

///////////////////////////////////////////////////////////////
////// 公共函数

/* 线程执行函数，监听接收对方发来的消息 */
void *pthread_recv(void *arg) {
    int *socketfd = (int *)arg;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int numberOfReaded = 0;
    while (true) {
        numberOfReaded = recv(*socketfd, buffer, BUFFER_SIZE, 0); /*读取客户端进程发送的数据, 比发送的字节数多1（带回车） */
        sscanf(buffer, "%s\n", buffer); /* 将末尾的 回车 去掉 */
        buffer[numberOfReaded] = '\0'; /* 加上终止符，避免出错 */
        if (numberOfReaded == -1) {
            handleError("读取数据错误");
        } else if (numberOfReaded == 0) {
            printf("对端已经关闭\n");
            close(*socketfd);
            pthread_exit((void *)NULL);
            // return (void*);
        }
        printf("收到对端进程数据长度为%s\n", buffer);
    }
}

///////////////////////////////////////////////////////////////
////// 服务端函数

/* 将socket与某个地址绑定 */
void bindToAddress(int socketfd) {
    struct sockaddr_un address;
    address.sun_family = AF_UNIX;//使用Unix domain
    strncpy(address.sun_path, path, sizeof(path));//这个地址的类型有3种，参考上文所说，这里我们使用“系统路径”这一类型
    
    if (remove(path) == -1 && errno != ENOENT) { //绑定之前先要将这个路径对应的文件删除，否则会报EADDRINUSE
        handleError("删除失败");
    }
    if (bind(socketfd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        handleError("地址绑定失败");
    }
}

/* socket IPC 通信的简单例子 */
void echo(int socket) {
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, sizeof(buffer));
    int numberOfReaded, numberOfWrited = 0;
    while (true) {
        numberOfReaded = recv(socket, buffer, BUFFER_SIZE, 0);//读取客户端进程发送的数据
        if (numberOfReaded == -1) {
            handleError("读取数据错误");
        } else if (numberOfReaded == 0) {
            printf("客户端关闭连接\n");
            close(socket);
            return;
        }
        printf("收到对端进程数据长度为%d，开始echo", numberOfReaded);
        if (numberOfReaded > 0) {
            numberOfWrited = write(socket, buffer, numberOfReaded);//然后原版返回
            printf("  写入的结果为%d\n", numberOfWrited);
        }
    }
}


/* 处理客户端的“连接”请求，非HDLC连接 */
void handleRequest(int socketfd) {
    int childfd = accept(socketfd, NULL, NULL);//监听客户端的请求，没有请求到来的话会一直阻塞
    if (childfd == -1) {
        handleError("accept 错误");
    }
    puts("client发起连接...");
    // echo(socket); // 执行连接建立后的代码，这里是简单的打印接收内容
    pthread_t tid1; 
    pthread_t tid2;
    pthread_create(&tid1, NULL, pthread_recv, (void *)&childfd);
    pthread_create(&tid2, NULL, pthread_server_send, (void *)&childfd);
    pthread_join(tid1, (void *)NULL);
    pthread_join(tid2, (void *)NULL);
    /* 这里没有处理 接受/发送的同时关闭， 后面根据HDLC拆链来设置 */
}

/* 线程执行函数，用于服务器向对方发送消息 */
void *pthread_server_send(void *arg) {
    int *childfd = (int *)arg;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int numberOfSended;
    while (true) {
        numberOfSended = read(0, buffer, sizeof(buffer));
        buffer[numberOfSended] = '\0';
        if (numberOfSended < 0) {
            handleError("发送数据错误");
        } else {
            if (strncmp(buffer, "quit", 4) == 0) {
                printf("本机关闭连接\n");
                close(*childfd);
                pthread_exit((void *)NULL);
            }
        }
        if (write(*childfd, buffer, strlen(buffer)) == -1) {
            handleError("发送失败");
        }
    }
}

///////////////////////////////////////////////////////////////
////// 客户端函数

/* 与服务端建立连接 */
void connectServer(int socketfd) {
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path));

    if (connect(socketfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        handleError("连接服务端失败");
    }
}

/* 线程执行函数，用于客户向服务器发送消息 */
void *pthread_client_send(void *arg) {
    int *socketfd = (int *)arg;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int numberOfSended;
    while (true) {
        numberOfSended = read(0, buffer, sizeof(buffer));
        buffer[numberOfSended] = '\0';
        if (numberOfSended < 0) {
            handleError("发送数据错误");
        } else {
            if (strncmp(buffer, "quit", 4) == 0) {
                printf("本机关闭连接\n");
                close(*socketfd);
                pthread_exit((void *)NULL);
            }
        }
        if (send(*socketfd, buffer, strlen(buffer), 0) == -1) {
            handleError("发送失败");
        }
    }
}