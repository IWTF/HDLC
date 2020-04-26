#ifndef __FRAME_STRUCTURE_H__
#define __FRAME_STRUCTURE_H__

#include <stdint.h>

typedef uint8_t Flag; /* 标志字段 */
typedef uint8_t Address; /* 地址字段 */
/**
 ****帧结构各字段介绍****
 * N(S) = 发送序号
 * N(R) = 接收序号（双工时才设置）
 * S = 监控帧功能比特
 * M = 无编号帧功能比特(两段5bit，没明确编号含义，自定义吧)
 * P/F = 轮寻/结束比特（恒置为1就好）
 *  1. 在命令帧中，他指P，设置为1时，表示向对等实体请求响应帧
 *  2. 在响应帧里，他指F，设置为1时，表示该帧是对一个请求命令的响应结果
 * 
 *---- 结构体各字段介绍 -----
 * t_SM 代表前4个字节（t + S/M)
 * PF   代表P/F
 * NR   代表N(R) / M
 **/
typedef union /* 控制字段 */
{
	struct
	{
		unsigned int t_SM:4; /* 标志、N(S)、S/M 在收发时，需要特别处理 */
		unsigned int PF:1;
		unsigned int NR:3;
	};
	uint8_t control;
}Control;
/** 
 * 信息字段变长，在发送时构造
 * 由于information是变长，
 * 所以在接收时，应先获取前面和FCS，中间剩下的为information 
**/
typedef uint16_t FCS; /* 帧检验字段 */


#endif

