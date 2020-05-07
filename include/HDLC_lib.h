#ifndef __HDLC_LIB_H__
#define __HDLC_LIB_H__

#include "global.h"
#include "frame_structure.h"

#define MAX_INFO_SIZE 4
#define MAX_I_FRAME_SIZE 10 /* MAX_INFO_SIZE + 6 */

/* 一些工具函数，校验和计算 */
void initParam(pthread_param *param, int socketfd, int aimAddr, int sourceAddr);
uint16_t countFcs(unsigned char *frame, unsigned int len);

/* 构造无编号帧 */
unsigned int U_Framing(char *frame, int framelen, uint8_t addr, uint8_t ctrl);

/* HDLC请求和响应 （用于建立连接和拆链） */
void HDLC_request(pthread_param params, uint8_t ctrl);
void HDLC_response(pthread_param params, uint8_t ctrl);

/* 消息划分，组帧(I-frame) */
int splitInfo(char *msg, int dest);
char *I_Framing(char *info_beg, char *info_end, char *buf_pter, int dest);
void setControl(Control *control, int dest);

/* 对接收帧进行解析 */
void divideBuf(char *buffer, int b_len, pthread_param params);
/* 发送监督帧 */
void S_framing(pthread_param params, sType t, uint16_t num);

/* 处理收到的监督帧 */
void deal_SFrame(char *frame, int f_len);
void feedback_RR(int c);
void feedback_SREJ(int c);

#endif