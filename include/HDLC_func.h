#ifndef __HDLC_FUNC_H__
#define __HDLC_FUNC_H__

#include "frame_structure.h"

/* 连接建立 */
int U_Framing(char *frame, uint8_t t, int dest);
void initHDLC(p_param params);
void listenSIM(p_param params);

/* 通信过程，信息帧发送函数 */
void setControl(Control *control, uint8_t dest);
int splitInfo(char *msg, char *buffer, uint8_t dest);
char *I_Framing(char *info_beg, char *info_end, char *buf_pter, uint8_t dest);
uint16_t calFCS(unsigned char *frame,unsigned int f_len);
void divideBuf(char *buffer, int b_len, int source);

#endif