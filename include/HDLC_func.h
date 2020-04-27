#ifndef __HDLC_FUNC_H__
#define __HDLC_FUNC_H__

#include "frame_structure.h"

/* 连接建立 */
int U_Framing(char *frame, uint8_t t, int dest);
void initHDLC(int socketfd, int dest);
void listenSIM(int socketfd, int dest);

/* 通信过程，信息帧发送函数 */
void setControl(Control *control, int dest);
void splitInfo(char *msg, char *buffer, int dest);
char *I_Framing(char *info_beg, char *info_end, char *buf_pter, int dest) ;
uint16_t calFCS(unsigned char *frame,unsigned int f_len);

#endif