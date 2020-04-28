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
#include "HDLC_func.h"
#include "socket_IPC.h"

char path[] = "./namo_amitabha"; /* unix socket 通信使用的系统路径 */

///////////////////////////////////////////////////////////////
////// 公共函数

/* 线程执行函数，监听接收对方发来的消息 */
void *pthread_recv(void *arg) {
    p_param *param = (p_param *)arg;

    int socketfd = param->fd;
    char buffer[BUFFER_SIZE]; /* 收到的信息流 */
    memset(buffer, 0, sizeof(buffer));
    int numberOfReaded = 0;
    while (true) {
        numberOfReaded = recv(socketfd, buffer, BUFFER_SIZE, 0); /*读取客户端进程发送的数据 */
        buffer[numberOfReaded] = '\0'; /* 加上终止符，避免出错 */
        if (numberOfReaded == -1) {
            handleError("读取数据错误");
        } else if (numberOfReaded == 0) {
            printf("对端已经关闭\n");
            close(socketfd);
            pthread_exit((void *)NULL);
            // return (void*);
        }
        // 数据校验，拆帧
        printf("收到对端进程数据长度为%d\n", numberOfReaded);
        divideBuf(buffer, numberOfReaded, param->source);
    }
}

/* 线程执行函数，用于客户向服务器发送消息 */
void *pthread_send(void *arg) {
    p_param *param = (p_param *)arg;

    int socketfd = param->fd;
    char msg[BUFFER_SIZE];    /* 要发送的信息 */
    char buffer[BUFFER_SIZE]; /* 信道中的信息流 */
    memset(msg, 0, sizeof(msg));
    memset(buffer, 0, sizeof(buffer));
    int numberOfSended;
    while (true) {
        numberOfSended = read(0, msg, sizeof(msg));
        sscanf(msg, "%s\n", msg); /* 将末尾的 回车 去掉 */
        msg[numberOfSended-1] = '\0';
        if (numberOfSended < 0) {
            handleError("发送数据错误");
        } else {
            if (strncmp(msg, "quit", 4) == 0) {
                printf("本机关闭连接\n");
                close(socketfd);
                pthread_exit((void *)NULL);
            }
        }
        /* 进行组帧 */
        int buf_len = splitInfo(msg, buffer, param->dest);
        /* 模拟发送出的帧在信道上传输 */
        if (send(socketfd, buffer, buf_len, 0) == -1) {
            handleError("发送失败");
        }
    }
}

/**
 * HDLC通信开始前的准备，各主机确定自己的状态...(不知道怎麽描述了)
 * params: p_param参数，具体内容看结构体定义
 *
 */
void HDLC_option(p_param params) {
    int commend;
    bool connected = false;
    /* 由用户选择，是否发起连接请求 */
    while(true) {
        show_menu();
        scanf("%d", &commend);
        if (commend == 1) {
            // 执行HDLC发送请求
            initHDLC(params);
            break;
        } else if (commend == 2) {
            // 发送指定文件的内容
            listenSIM(params);
            break;
        } else if (commend == 3) {
            // 拆链
        } else {
            printf("请输入正确的指令\n");
        }
    }
    
    HDLC_start(params);
}

/** 
 * 启动HDLC通信
 * params: 进程id
 * 描述： HDLC连接已建立，正式开始HDLC通信
 **/
void HDLC_start(p_param params) {
    pthread_t tid1;
    pthread_t tid2;
    pthread_create(&tid1, NULL, pthread_recv, (void *)&params);
    pthread_create(&tid2, NULL, pthread_send, (void *)&params);
    pthread_join(tid1, (void *)NULL);
    pthread_join(tid2, (void *)NULL);
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


/* 处理客户端的“连接”请求，非HDLC连接 */
void handleRequest(int socketfd) {
    int childfd = accept(socketfd, NULL, NULL);//监听客户端的请求，没有请求到来的话会一直阻塞

    if (childfd == -1) {
        handleError("accept 错误");
    }
    puts("socket IPC建立成功...");

    /* 监听信道，但只有HDLC连接建立后， 可以正式通信 */
    p_param params;
    params.fd = childfd;
    params.source = SERVER_ADDR;
    params.dest = CLIENT_ADDR;

    HDLC_option(params);
}


///////////////////////////////////////////////////////////////
////// 客户端函数

/* 
 * socket 通信建立过程 
 * 客户端请求与服务端建立连接 
 * params: socketfd 文件描述符
 * 描述：此过程建立的仅仅是socket，HDLC通信还未开始
 **/
void connectServer(int socketfd) {
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path));

    if (connect(socketfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        handleError("连接服务端失败");
    }

    p_param params;
    params.fd = socketfd;
    params.source = CLIENT_ADDR;
    params.dest = SERVER_ADDR;

    HDLC_option(params);
}

