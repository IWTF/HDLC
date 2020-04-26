#ifndef __SOCKET_IPC_H__
#define __SOCKET_IPC_H__

/* 公共函数 */
void *pthread_recv(void *arg);

/* 服务端函数 */
void bindToAddress(int socketfd);
void handleRequest(int serverSocket);
void *pthread_server_send(void *arg);

/* 客户端函数 */
void connectServer(int socketfd);
void *pthread_client_send(void *arg);

#endif