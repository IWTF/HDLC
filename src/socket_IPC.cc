#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <queue>
#include <time.h>

#include "global.h"
#include "tools.h"
#include "HDLC_lib.h"
#include "socket_IPC.h"

char path[] = "../build/namo_amitabha"; /* unix socket 通信使用的系统路径 */
extern Window sWin;
extern Window rWin;
extern std::queue<char *> sendQueue;
extern std::queue<char *> sendCache;

///////////////////////////////////////////////////////////////
////// 公共函数
/* 线程执行函数，监听接收对方发来的消息 */
void *pthread_recv(void *arg)
{
    pthread_param *params = (pthread_param *)arg;

    printf("fd: %d\n", params->fd);

    int socketfd = params->fd;
    char buffer[BUFFER_SIZE]; /* 收到的信息流 */
    memset(buffer, 0, sizeof(buffer));
    int numberOfReaded = 0;
    while (true)
    {
        numberOfReaded = recv(socketfd, buffer, BUFFER_SIZE, 0); /*读取客户端进程发送的数据, 比发送的字节数多1（带回车） */
        buffer[numberOfReaded] = '\0';                           /* 加上终止符，避免出错 */
        if (numberOfReaded == -1)
        {
            handleError((char *)"读取数据错误");
        }
        else if (numberOfReaded == 0)
        {
            printf("对端已经关闭\n");
            close(socketfd);
            pthread_exit((void *)NULL);
            // return (void*);
        }
        // 数据校验，拆帧
        printf("收到对端进程数据长度为%d\n", numberOfReaded);
        divideBuf(buffer, numberOfReaded, *params);
    }
}

/* 线程执行函数，用于客户向服务器发送消息 */
void *pthread_send(void *arg)
{
    pthread_param *params = (pthread_param *)arg;

    printf("fd: %d\n", params->fd);

    int socketfd = params->fd;
    char msg[BUFFER_SIZE];    /* 要发送的信息 */
    char buffer[BUFFER_SIZE]; /* 信道中的信息流 */
    memset(msg, 0, sizeof(msg));
    memset(buffer, 0, sizeof(buffer));
    int numberOfSended;

    srand((unsigned int)time(NULL));
    while (true)
    {
        numberOfSended = read(0, msg, sizeof(msg));
        sscanf(msg, "%s\n", msg); /* 将末尾的 回车 去掉 */
        msg[numberOfSended - 1] = '\0';
        if (numberOfSended < 0)
        {
            handleError((char *)"发送数据错误");
        }
        else
        {
            if (strncmp(msg, "quit", 4) == 0)
            {
                printf("本机关闭连接\n");
                close(socketfd);
                pthread_exit((void *)NULL);
            }
        }
        /* 进行组帧 */
        splitInfo(msg, params->aimAddr);
        /* 模拟[将待发送的帧发送到信道(buffer)上] */
        int buf_len = 0;
        while (!sendQueue.empty())
        {
            char *aaa = sendQueue.front();
            sendCache.push(aaa); /* 将发送的帧，加入到缓存队列 */
            sendQueue.pop();

            int rn = rand() % LOST_ERROR;
            if (rn == 0)
            {
                int fnum = (aaa[2] & 0x70) >> 3;
                printf("------帧丢失: 丢失帧编号为 %d \n", fnum);
            }
            else
            {
                int flen = strlen(aaa);
                memcpy(buffer + buf_len, aaa, flen);
                buf_len += flen;
            }
        }
        /* 模拟发送出的帧在信道上传输 */
        if (send(socketfd, buffer, buf_len, 0) == -1)
        {
            handleError((char *)"发送失败");
        }
    }
}

///////////////////////////////////////////////////////////////
////// 服务端函数

/**
 * 将socket与某个地址绑定
 * params: socketfd 文件描述符
**/
void bindToAddress(int socketfd)
{
    struct sockaddr_un address;
    address.sun_family = AF_UNIX;                  //使用Unix domain
    strncpy(address.sun_path, path, sizeof(path)); //这个地址的类型有3种，参考上文所说，这里我们使用“系统路径”这一类型
    if (remove(path) == -1 && errno != ENOENT)
    { //绑定之前先要将这个路径对应的文件删除，否则会报EADDRINUSE
        handleError((char *)"删除失败");
    }
    if (bind(socketfd, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        handleError((char *)"地址绑定失败");
    }
}

/**
 * server端建立完链接后开始执行收发函数线程
 * 参数：socket
 **/

void server_run(int socket)
{
    pthread_param param;
    initParam(&param, socket, AddrClient, AddrServer);

    /* 初始化窗口值 */
    sWin.beg = 0;
    sWin.end = 6;
    rWin.beg = 0;
    rWin.end = 6;

    pthread_t tid1;
    pthread_t tid2;
    pthread_create(&tid1, NULL, pthread_recv, (void *)&param);
    pthread_create(&tid2, NULL, pthread_send, (void *)&param);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    /* 这里没有处理 接受/发送的同时关闭， 后面根据HDLC拆链来设置 */
}

/* 处理客户端的“连接”请求，非HDLC连接 */
int handleRequest(int socketfd)
{
    int socket = accept(socketfd, NULL, NULL); //监听客户端的请求，没有请求到来的话会一直阻塞
    if (socket == -1)
    {
        handleError((char *)"accept 错误");
    }
    puts("client发起连接...");
    return socket;
    //建立HDLC连接

    //echo(socket); // 执行连接建立后的代码，这里是简单的打印接收内容
}

///////////////////////////////////////////////////////////////
////// 客户端函数

/* 与服务端建立连接 */
void connectServer(int socketfd)
{
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path));

    if (connect(socketfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        handleError((char *)"连接服务端失败");
    }
}

void client_run(int socket)
{
    pthread_param param;
    initParam(&param, socket, AddrServer, AddrClient);

    /* 初始化窗口值 */
    sWin.beg = 0;
    sWin.end = 6;
    rWin.beg = 0;
    rWin.end = 6;

    pthread_t tid1;
    pthread_t tid2;
    pthread_create(&tid1, NULL, pthread_recv, (void *)&param);
    pthread_create(&tid2, NULL, pthread_send, (void *)&param);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
}