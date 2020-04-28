#ifndef __SOCKET_IPC_H__
#define __SOCKET_IPC_H__

#include "global.h"

/* 公共函数 */
void show_menu();
void *pthread_send(void *arg);
void *pthread_recv(void *arg);
void HDLC_option(p_param params);
void HDLC_start(p_param params);

/* 服务端函数 */
void bindToAddress(int socketfd);
void handleRequest(int serverSocket);

/* 客户端函数 */
void connectServer(int socketfd);

#endif