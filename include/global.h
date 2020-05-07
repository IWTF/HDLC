#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <stdint.h>

#define BUFFER_SIZE 1017 /*缓冲区读取最大长度即info字段*/
#define FRAME_SIZE 1024	 /*帧的最大长度*/
#define AddrServer 0x69
#define AddrClient 0x6c

/* 信号量的Key */
#define k1 1234 /* k1用于同步server发送，和client接收 */
#define k2 2345 /* k2用于同步client发送，和server接收 */

/*可以在这里定义控制字段一些常用的*/
#define UA 0xc6		//11 00 0 110
#define SABM 0xcb //11 00 1 011
#define DISC 0xcd //11 00 0 110

/* 差错模拟，差错出现的概率 */
#define TRANS_ERROR 4
#define LOST_ERROR 8

/* 滑动窗口 */
typedef struct
{
	unsigned int beg; /* 滑动窗口起始指针 */
	unsigned int end; /* 滑动窗口末尾指针 */
} Window;

typedef struct
{
	int fd;
	int aimAddr;
	int sourceAddr;
} pthread_param;

#endif