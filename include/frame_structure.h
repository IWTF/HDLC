#ifndef __FRAME_STRUCTURE_H__
#define __FRAME_STRUCTURE_H__

#include <stdint.h>

//uint8_t addrClient=0x67;	//客户端地址
//uint8_t addrServer=0xb9;	//服务端地址

typedef struct /* 控制字段 */
{
	unsigned int NR : 3;
	unsigned int PF : 1;
	unsigned int t_SM : 4; /* 标志、N(S)、S/M 在收发时，需要特别处理 */
} Control;

/** 
 * 信息字段变长，在发送时构造
 * 由于information是变长，
 * 所以在接收时，应先获取前面和FCS，中间剩下的为information 
**/

typedef enum
{
	RR = 0x00,
	RNR = 0x20,
	REJ = 0x10,
	SREJ = 0x30
} sType;

#endif
