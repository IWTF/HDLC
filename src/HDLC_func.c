#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>

#include "tools.h"
#include "global.h"
#include "HDLC_func.h"

/** 
 * 为了省事，直接定义了客户端和服务端各字段
 * 由splitInfo的参数dest决定 选择哪组
 * 规定：  dest=1 服务器
 *        dest=0 客户端 
 **/
int sCur; 	 /* 服务端待发送的帧编号 */
Window sWin; /* 服务端的发送窗口 */
int cCur; 	 /* 客户端待发送的帧编号 */
Window cWin; /* 客户端的发送窗口 */

const Flag flag = 0x7e; /* 标志位 0111 1110 */
const Address client_addr = CLIENT_ADDR; /* 对端的地址，使用进程id */
const Address server_addr = SERVER_ADDR; /* 对端的地址，使用进程id */

///////////////////////////////////////////////////////////////
////// 连接建立

/**
 * 创建SIM无编号帧
 * params: frame 帧的首地址
 * 		   t     无编号帧控制字段的值
 *         dest  指明目标主机
 * 返回值： 帧长度
 * 描述： 使用异步平衡方式，无编号帧控制字段自定义设置（因为找不到控制字段的相关资料）
 * 0xcb 是自己规定的SIM帧; 0xc6为确认帧（UA）
 */
int U_Framing(char *frame, uint8_t t, int dest) {
	FCS fcs;
	char *frame_pter = frame;
	
	/* flag */
	memcpy(frame, &flag, 1); frame+=1;
	/* address */
	if (dest == 1) {
		memcpy(frame, &server_addr, 1); frame+=1;
	} else {
		memcpy(frame, &client_addr, 1); frame+=1;
	}
	/* control */
	uint8_t ctrl = t;
	memcpy(frame, &ctrl, 1); frame+=1; 
	/* information */
	frame+=1; // 之前memset 0了，这里相当于一个 '\0'
	/* FCS */
	fcs = calFCS((unsigned char *)frame_pter, (unsigned int)(frame - frame_pter)); /* 计算校验和 */
	
	memcpy(frame, &fcs, 2);

	printFrame(frame_pter, frame+2);

	return (frame-frame_pter+2);
}

/**
 * 初始化HDLC连接
 * params: socketfd 对端的文件描述符
 * 		   dest 	指明目标主机
 * 描述： HDLC连接建立包括 发送SIM帧 & 接收对方的确认帧 
 **/
void initHDLC(int socketfd, int dest) {
	/* 构造SIM帧 */
	char frame[FRAME_SIZE];
	memset(frame, 0, sizeof(frame));
	int f_len = U_Framing(frame, 0xcb, dest);
	
	/* 发送SIM帧 */
	if (send(socketfd, frame, f_len, 0) == -1) {
		handleError("发送失败");
	}
	printf("等待无编号确认帧（UA）\n");

	/* 监听对方的确认帧 */
	memset(frame, 0, sizeof(frame));
	uint8_t myaddr;			 // 我的地址
    uint16_t fcs;            // 计算得到的帧校验位
    uint16_t fcs_recv;		 // 帧内解析出的fcs

    if (dest == 1)
    	myaddr = client_addr;
    else
    	myaddr = server_addr;

	while (1) {
		int numOfReaded = recv(socketfd, frame, FRAME_SIZE, 0);
        if(numOfReaded==-1) {
            handleError("读取数据出错");
        }
        frame[numOfReaded]='\0';

        /* 判断帧的目的地址 */
        if((frame[1]&0xff) != myaddr){ 
        	printf("Its target is not me.\n");
            continue; // 不是发给“我”的，直接抛弃
        }

        /* 判断是否为UA帧 */
        if((frame[2]&0xff) != 0xc6){ 
            printf("it's not a UA frame.\n");
            continue;
        }
        printf("接收到一个确认帧（UA）\n");

        /* FCS校验 */
        fcs = calFCS(frame, numOfReaded-2);
        memcpy(&fcs_recv, (frame+numOfReaded-2), 2);

        if(fcs != fcs_recv) { /* 帧出错，抛弃 */
            printf("传输出错，CRC校验不通过,继续侦听UA帧\n");
            continue;
        }
        printf("HDLC建立连接成功！！\n");
        break;
	}
}

/**
 * 监听对端对自己发起的HDLC连接请求
 * params: scoketfs 文件描述符
 * 		   dest     指明目标主机
 * 描述：一直监听信道，收到请求帧后，向对方发送确认帧，连接建立成功
 **/
void listenSIM(int socketfd, int dest) {
	char frame[FRAME_SIZE];
	memset(frame, 0, sizeof(frame));
    uint16_t fcs;            // 计算得到的帧校验位
    uint16_t fcs_recv;		 // 帧内解析出的fcs
	uint8_t myaddr = server_addr;
	if (dest == 1)
		myaddr = client_addr;

	/* 监听连接初始化请求 */
	while (true) {
		int numOfReaded = recv(socketfd, frame, FRAME_SIZE, 0);
        if(numOfReaded==-1) {
            handleError("读取数据出错");
        }
        frame[numOfReaded]='\0';

		/* 判断帧的目的地址 */
        if((frame[1]&0xff) != myaddr){ 
        	printf("Its target is not me.\n");
            continue; // 不是发给“我”的，直接抛弃
        }

        /* 判断是否为UA帧 */
        if((frame[2]&0xff) != 0xcb){ 
            printf("it's not a SIM frame.\n");
            continue;
        }
        printf("接收到一个请求帧（SIM）\n");

        /* 判断传输过程中是否有帧出错 */
        fcs = calFCS(frame, numOfReaded-2); /* 这里计算FCS，带最后一字节 */
        memcpy(&fcs_recv, (frame+numOfReaded-2), 2); /* 帧长度-2为FCS的位置 */
        if(fcs != fcs_recv) { /* 帧出错，抛弃 */
            printf("传输出错，CRC校验不通过,继续侦听SIM帧\n");
            continue;
        }	
        printf("接收到对端的请求帧！！\n");
        break;
	}
	/* 向对方发送确认帧 */
	memset(frame, 0, sizeof(frame));
	int f_len = U_Framing(frame, 0xc6, dest);
	if (send(socketfd, frame, f_len, 0) == -1) {
		handleError("发送失败");
	}
	printf("HDLC 链接建立成功\n");
}



///////////////////////////////////////////////////////////////
////// 信息帧发送函数

/**
 * 消息的分割
 * params: msg    待分割的消息
 *         buffer 对信道比特流的模拟
 * 描述：将要传递的msg分块，并组帧放入buffer
 **/
void splitInfo(char *msg, char *buffer, int dest) {
	char *msg_pter = msg;
	char *buf_pter = buffer;
	int len = strlen(msg);
	for (; msg_pter != msg + strlen(msg); msg_pter += MAX_INFO_SIZE) {
		/* 长度不够Information的最大长度 || 要传送的信息本来就很短 */
		if (strlen(msg_pter) < MAX_INFO_SIZE) { 
			I_Framing(msg_pter, msg_pter + strlen(msg_pter), buf_pter, dest);
			break;
		}
		buf_pter = I_Framing(msg_pter, msg_pter + MAX_INFO_SIZE, buf_pter, dest); /* 更新信息流指针，移动一个帧长度 */
	}
}

/** 
 * 组帧 i-frame
 * params: info_beg 发送信息的开始地址
 * 		   info_end 发送信息的结束地址
 * 		   buf_pter 当前信道字节流 存放该帧 的首地址
 * 返回值： 信息流的下一个地址
 * 描述： 对信息进行组帧，并模拟帧的发送（帧是一帧一帧发出，但在信道上是连续的）
 **/
char *I_Framing(char *info_beg, char *info_end, char *buf_pter, int dest) {
	FCS fcs;
	Control control;
	char *buffer = buf_pter; /* 保留信息流起始位置buf_pter，方便计算校验和 */
	setControl(&control, dest); /* 设置control字段，并更新窗口 */

	memcpy(buffer, &flag, 1); buffer += 1; /* Flag */

	if (dest == 1) {
		memcpy(buffer, &server_addr, 1); buffer += 1; /* 目的地址为 服务端Address */
	} else {
		memcpy(buffer, &client_addr, 1); buffer += 1; /* 目的地址为 客户端Address */
	}

	memcpy(buffer, &control, 1); buffer += 1; /* Control */
	memcpy(buffer, info_beg, (info_end-info_beg)); buffer += (info_end-info_beg); /* Information */
	fcs = calFCS((unsigned char *)buf_pter, (unsigned int)(buffer - buf_pter-1)); /* 计算校验和 */
	memcpy(buffer, &fcs, 2); buffer += 2; /* FCS */

	// buffer = bit_filling(buf_pter, buffer); /* 比特填充 */
	return buffer;
}

/**
 * 设置控制字段
 × Params： control 待设置的控制字段
 * 描述： 对发送帧的控制字段进行设置，同时更新窗口值 
 **/
void setControl(Control *control, int dest) {
	control->PF = 0x1; /* 书中只提到了为1的含义，没有提0....的确是这样！！0没用 */
	control->NR = 0x0; /* 双向同时通信时才设置。 如果只实现单向则不需要设置 */
	
	if (dest == 1) {
		control->t_SM = (0x0 + cCur); /* t_SM是个组合字段 */
		/* 更新窗口 */
		cCur = (cCur + 1) % 8;
		cWin.beg = (cWin.beg + 1) % 8;
	} else {
		control->t_SM = (0x0 + sCur); /* t_SM是个组合字段 */
		/* 更新窗口 */
		sCur = (sCur + 1) % 8;
		sWin.beg = (sWin.beg + 1) % 8;
	}
}

/**
 * 计算校验序列
 * 参数：frame 需要计算校验序列的字符数组
 *      f_len 帧数组的长度
 * 返回值：FCS标志位（uint16_t），使用CRC-CCITT（x^16 + x^12 + x^5 + 1）
 **/
uint16_t calFCS(unsigned char *frame,unsigned int f_len) {
    int i, j;
    uint16_t crc_reg;
       
    crc_reg = (frame[0] << 8) + frame[1]; /* 获取16bit数据 */
    for (i = 0; i < f_len; i++) {
        if (i < f_len - 2)
            for (j = 0; j <= 7; j++) { /* 对第一位开始的每16bit进行运算（一个char中8bit） */
                if ((int16_t)crc_reg < 0) /* 如果最高位为1, 则与除数进行异或 */
                    crc_reg = ((crc_reg << 1) + (frame[i + 2] >> (7 - i))) ^ 0x1021;
                else /* 与0x00异或 */
                    crc_reg = (crc_reg << 1) + (frame[i + 2] >> (7 - i));     
            }
         else /* CRC计算过程中，被除数末尾补上了16bit 0, 该代码里没补，所以在这里模拟计算 */
            for (j = 0; j <= 7; j++) {
                if ((int16_t)crc_reg < 0)
                    crc_reg = (crc_reg << 1) ^ 0x1021;
                else
                    crc_reg <<= 1;            
            }        
    }
    return crc_reg;
}

/**
 * 比特填充
 * params: beg 需要进行比特填充的帧开始地址
 *         end 结束地址
 * 返回值: 填充后，帧的结束地址
 **/
char *bit_filling(char *beg, char *end) {
	int num = 0;
	for (int i=0; i<(end - beg); ++i) {

	}
}