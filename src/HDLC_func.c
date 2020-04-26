#include <stdio.h>
#include <string.h>

#include "global.h"
#include "HDLC_func.h"

/* ======================================================
   为了函数的公用，且不增加函数参数数量
   可以将window设置为足够大的数组，文件描述符一般不大
   然后 以fd为下标来指示窗口的变化
 =========================================================*/
int sCur; /* 服务端待发送的帧编号 */
Window sWin; /* 服务端的发送窗口 */

const Flag flag = 0x7e; /* 标志位 0111 1110 */
const Address addr; /* 对端的地址，使用进程id */
Control control;
FCS fcs;

/**
 * 消息的分割
 * params: msg    待分割的消息
 *         buffer 对信道比特流的模拟
 * 描述：将要传递的msg分块，并组帧放入buffer
 **/
void splitInfo(char *msg, char *buffer) {
	char *msg_pter = msg;
	char *buf_pter = buffer;
	int len = strlen(msg);
	for (; msg_pter != msg + strlen(msg); msg_pter += (MAX_INFO_SIZE+1)) {
		/* 长度不够Information的最大长度 || 要传送的信息本来就很短 */
		if (strlen(msg_pter) < MAX_INFO_SIZE) { 
			Framing(msg_pter, msg_pter + strlen(msg_pter), buf_pter, CLIENT_ADDR);
			break;
		}
		int frame_size = Framing(msg_pter, msg_pter + MAX_INFO_SIZE, buf_pter, CLIENT_ADDR);
		buf_pter += frame_size; /* 更新信息流指针，移动一个帧长度 */
	}
}

/** 
 * 组帧 i-frame
 * params: info_beg 发送信息的开始地址
 * 		   info_end 发送信息的结束地址
 * 		   buf_pter 当前信道字节流 存放该帧 的首地址
 * 描述： 对信息进行组帧，并模拟帧的发送（帧是一帧一帧发出，但在信道上是连续的）
 **/
int I_Framing(char *info_beg, char *info_end, char *buf_pter) {
	char *buffer = buf_pter; /* 保留字节流起始位置buf_pter，方便计算校验和 */
	addr = (0x80 + CLIENT_ADDR); /* 最高位为1,表示不再扩展 */
	setControl(control); /* 设置control字段，并更新窗口 */

	memcpy(buffer, flag, 1); buffer += 2; /* Flag */
	memcpy(buffer, addr, 1); buffer += 2; /* Address */
	memcpy(buffer, control, 1); buffer += 2; /* Control */
	memcpy(buffer, info_beg, (info_beg-info_end)); buffer += (info_beg-info_end+1); /* Information */
	memset(buffer, 0, 2); buffer += 3; /* FCS */

	// 计算FCS
	// 比特填充
}

/**
 * 设置控制字段
 × Params： control 待设置的控制字段
 * 描述： 对发送帧的控制字段进行设置，同时更新窗口值 
 **/
void setControl(Control &control) {
	control.t_SM = (0x0 + sCur); /* t_SM是个组合字段 */
	control.PF = 0x1; /* 书中只提到了为1的含义，没有提0....的确是这样！！0没用 */
	control.NR = 0x0; /* 双向同时通信时才设置。 如果只实现单向则不需要设置 */
	
	/* 更新窗口 */
	sCur = (sCur + 1) % 8;
	sWin.beg = (sWin.beg + 1) % 8;
	sWin.size = sWin.end - sWin.beg; /* 看窗口大小是否有用？ 没用删掉 */
}