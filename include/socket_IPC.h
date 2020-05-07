#ifndef __SOCKET_IPC_H__
#define __SOCKET_IPC_H__

#include <stdint.h>

/* 公共函数 */
void *pthread_recv(void *arg);

/* 服务端函数 */
void bindToAddress(int socketfd);
void *pthread_server_send(void *arg);
void server_run(int socket);
int handleRequest(int socketfd);

/* 客户端函数 */
void connectServer(int socketfd);
void *pthread_client_send(void *arg);
void client_run(int socket);


#endif