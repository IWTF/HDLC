#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <queue>

#include "tools.h"
#include "global.h"
#include "HDLC_lib.h"
#include "frame_structure.h"

uint8_t Flag = 0x7e;

int sCur = 0; /* 待发送的帧编号 */
Window sWin;  /* 发送窗口 */
int rCur = 0; /* 待接收的帧编号 */
Window rWin;  /* 接收窗口 */

std::queue<char *> sendQueue; /* 发送队列 */
std::queue<char *> sendCache; /* 发送缓存 */
std::queue<int> recvQueue;    /* 接收队列,优先级队列，升序 */

///////////////////////////////////////////////////////////////////////
/////////// 校验和计算

/**
 * 计算校验序列
 * 参数：frame 需要计算校验序列的字符数组
 *      len 数组的长度
 * 返回值：FCS标志位（uint16_t）
 **/
uint16_t countFcs(unsigned char *frame, unsigned int len)
{
    int i, j;
    uint16_t crc_reg;

    crc_reg = (frame[0] << 8) + frame[1];
    for (i = 0; i < len; i++)
    {
        if (i < len - 2)
            for (j = 0; j <= 7; j++)
            {
                if ((int16_t)crc_reg < 0)
                    crc_reg = ((crc_reg << 1) + (frame[i + 2] >> (7 - i))) ^ 0x1021;
                else
                    crc_reg = (crc_reg << 1) + (frame[i + 2] >> (7 - i));
            }
        else
            for (j = 0; j <= 7; j++)
            {
                if ((int16_t)crc_reg < 0)
                    crc_reg = (crc_reg << 1) ^ 0x1021;
                else
                    crc_reg <<= 1;
            }
    }
    return crc_reg;
}

///////////////////////////////////////////////////////////////////////
/////////// HDLC连接初始化

/**
 * 初始化子线程参数
 * param: socketfd   文件描述符
 *        aimAddr    目标地址
 *        sourceAddr 源地址
 * 描述：为了代码美观，把参数的设置集中在一起
 **/
void initParam(pthread_param *param, int socketfd, int aimAddr, int sourceAddr)
{
    param->fd = socketfd;
    param->aimAddr = aimAddr;
    param->sourceAddr = sourceAddr;
}

/**
 * 组帧 U-Frame
 * params:addr 目标地址
 *        ctrl 控制字段
 *        info 信息段
 * 功能：为地址、控制字段、信息段加上前面的标志，以及后面的FCS校验和Flag
 * 返回值：char 类型的帧利用参数返回
 *      unsigned int 帧的长度，利用返回值返回
 **/
unsigned int U_Framing(char *frame, int framelen, uint8_t addr, uint8_t ctrl)
{
    uint16_t fcs;
    uint8_t dest = addr;
    uint8_t control = ctrl;
    char *tmpFrame = frame;
    char info = '\0';

    memcpy(frame, &Flag, 1);
    frame += 1; /* Flag */
    memcpy(frame, &dest, 1);
    frame += 1; /* 设置目的地址 */
    memcpy(frame, &control, 1);
    frame += 1; /* Control */
    memcpy(frame, &info, 1);
    frame += 1;                                                                            /* Information */
    fcs = countFcs((unsigned char *)(tmpFrame + 1), (unsigned int)(frame - tmpFrame - 1)); /* 计算校验和 */
    memcpy(frame, &fcs, 2);
    frame += 2; /* FCS */
    *frame = '\0';

    printFrame(tmpFrame, (frame - tmpFrame));

    return 6;
}

/**
 * 发送一个无编号帧请求，并等待回应
 * 参数：socket 套接字
 *      aimAddr 目标地址
 *      sourceAddr 源地址
 *      ctrl    控制字段
 **/

void HDLC_request(pthread_param params, uint8_t ctrl)
{
    char frame[FRAME_SIZE];
    char info[BUFFER_SIZE];

    /*组SABME帧 并发送*/
    unsigned int framelen = 0;
    framelen = U_Framing(frame, framelen, params.aimAddr, ctrl);
    printFrame(frame, framelen);

    if (send(params.fd, frame, framelen, 0) == -1)
    {
        handleError((char *)"发送失败");
    }
    printf("等待应答（UA）帧\n");
    /*等待一个UA帧，如果等到了即连接成功*/
    uint8_t tmpaddr; //解析帧地址
    uint8_t tmpctrl; //解析帧控制位
    uint16_t tmpfcs; //解析帧校验位

    while (1)
    {
        int numOfReaded = recv(params.fd, frame, FRAME_SIZE, 0);
        if (numOfReaded == -1)
        {
            handleError((char *)"对端已经关闭");
        }
        frame[numOfReaded] = '\0';
        if ((frame[1] & 0xff) != params.sourceAddr)
        {
            //判断地址是否为发给客户端的
            printf("addr is error!\n");
            continue;
        }
        //printFrame(frame,numOfReaded);
        if ((frame[2] & 0xff) != UA)
        {
            //判断是否为UA帧
            printf("it's not a UA frame\n");
            continue;
        }
        printf("接收到一个回复帧（UA）\n");
        tmpfcs = countFcs((unsigned char *)(frame + 1), (unsigned int)(numOfReaded - 3));
        uint16_t fcs = 0;
        memcpy(&fcs, (frame + numOfReaded - 2), 2);
        if (fcs != tmpfcs)
        {
            printf("传输出错，CRC校验不通过,继续侦听UA帧\n");
            continue;
        }
        printFrame(frame, numOfReaded);
        break;
    }
}

void HDLC_response(pthread_param params, uint8_t ctrl)
{
    char frame[FRAME_SIZE];
    int numberOfReaded;
    while (true)
    {
        numberOfReaded = recv(params.fd, frame, FRAME_SIZE, 0); //读取客户端进程发送的数据
        if (numberOfReaded == -1)
        {
            handleError((char *)"读取数据错误");
        }
        else if (numberOfReaded == 0)
        {
            printf("客户端关闭连接\n");
            close(params.fd);
            return;
        }
        else
        {
            //接收到帧 判断是否为发给“我”的SABME帧
            uint8_t tmpaddr = (frame[1] & 0xff);
            if (tmpaddr != params.sourceAddr)
            { //目标地址不是“我”
                continue;
            }

            uint8_t tmpctrl = (frame[2] & 0xff);
            if (tmpctrl != ctrl)
            { //控制字段不是SABM
                continue;
            }
            printf("接收到一个链接请求帧（SABME）大小为：%d\n", numberOfReaded);

            uint16_t tmpfcs = countFcs((unsigned char *)(frame + 1), (unsigned int)(numberOfReaded - 3));
            uint16_t fcs = 0;
            memcpy(&fcs, (frame + numberOfReaded - 2), 2);
            if (fcs != tmpfcs)
            { //帧校验不通过，传输出错
                printf("传输出错，CRC校验不通过,继续侦听SABM帧\n");
                continue;
            }
            break; //受到正确的SABME帧
        }
    }
    printFrame(frame, numberOfReaded);
    uint8_t addr = params.aimAddr;
    //接收到后需发送ＵA帧回应

    uint8_t tempctrl = UA; //控制字段为UA
    char info[BUFFER_SIZE];
    info[0] = '\0';
    //组UA帧
    unsigned int framelen = 0;
    framelen = U_Framing(frame, framelen, params.aimAddr, tempctrl);

    //将UA帧发出去
    int numberofWrited = 0;
    numberofWrited = send(params.fd, frame, framelen, 0);
    printf("发送帧：\n");
    printFrame(frame, framelen);
}

///////////////////////////////////////////////////////////////////////
/////////// 对要发送信息的分割 和 组帧

/**
 * 消息的分割
 * params: msg    待分割的消息
 *         dest   目的地址
 * 描述：将要传递的msg分块，并组帧后放入消息队列
 * 附：消息队列的好处：
 *    1. 实现缓存功能
 *    2. 在发送帧时，可以实现不超过接收方窗口大小
 **/
int splitInfo(char *msg, int dest)
{
    char *msg_pter = msg;
    int len = strlen(msg);
    for (; msg_pter != msg + strlen(msg); msg_pter += MAX_INFO_SIZE)
    {
        char *buffer = (char *)malloc(sizeof(char *) * MAX_I_FRAME_SIZE);
        if (buffer == NULL)
        {
            handleError((char *)"申请内存失败!");
        }

        /* 长度不够Information的最大长度 || 要传送的信息本来就很短 */
        if (strlen(msg_pter) < MAX_INFO_SIZE)
        {
            I_Framing(msg_pter, msg_pter + strlen(msg_pter), buffer, dest);
            sendQueue.push(buffer);
            break;
        }
        I_Framing(msg_pter, msg_pter + MAX_INFO_SIZE, buffer, dest); /* 更新信息流指针，移动一个帧长度 */
        sendQueue.push(buffer);
    }
}

/** 
 * 组帧 i-frame
 * params: info_beg 发送信息的开始地址
 *         info_end 发送信息的结束地址
 *         buf_pter 当前信道字节流 存放该帧 的首地址
 * 返回值： 信息流的下一个地址
 * 描述： 对信息进行组帧，并模拟帧的发送（帧是一帧一帧发出，但在信道上是连续的）
 **/
char *I_Framing(char *info_beg, char *info_end, char *buf_pter, int dest)
{
    uint16_t fcs;
    uint8_t addr = dest;
    Control control;
    char *buffer = buf_pter;       /* 保留信息流起始位置buf_pter，方便计算校验和 */
    setControl(&control, dest);    /* 设置control字段，并更新窗口 */
    int rn = rand() % TRANS_ERROR; /* 随机数，用于模拟帧传输错误 */

    memcpy(buffer, &Flag, 1);
    buffer += 1; /* Flag */
    memcpy(buffer, &addr, 1);
    buffer += 1; /* 设置目的地址 */
    memcpy(buffer, &control, 1);
    buffer += 1; /* Control */
    memcpy(buffer, info_beg, (info_end - info_beg));
    buffer += (info_end - info_beg);                                                        /* Information */
    if (rn == 0)                                                                            /* 出错概率为1/4 */
        fcs = countFcs((unsigned char *)(buf_pter + 1), (unsigned int)(buffer - buf_pter)); /* 计算校验和 */
    else
        fcs = countFcs((unsigned char *)buf_pter, (unsigned int)(buffer - buf_pter)); /* 计算校验和 */
    memcpy(buffer, &fcs, 2);
    buffer += 2; /* FCS */
    *buffer = '\0';

    printFrame(buf_pter, (buffer - buf_pter));
    // buffer = bit_filling(buf_pter, buffer); /* 比特填充 */
    return buffer;
}

/**
 * 设置控制字段
 * Params： control 待设置的控制字段
 * 描述： 对发送帧的控制字段进行设置，同时更新窗口值 
 **/
void setControl(Control *control, int dest)
{
    control->PF = 0x1;         /* 书中只提到了为1的含义，没有提0....的确是这样！！0没用 */
    control->NR = rCur & 0x07; /* 双向同时通信时才设置 */

    control->t_SM = (0x0 | sCur); /* t_SM是个组合字段 */
    /* 更新窗口 */
    sCur = (sCur + 1) % 8;
    sWin.beg = (sWin.beg + 1) % 8;
}

///////////////////////////////////////////////////////////////////////
//////////// 对接收帧的解析

/** 
 * 根据flag划分帧
 * params: buffer 信道中的信息流
 *         b_len  信息流的字节数
 *         source 本机的地址（用于判断是否是发给自己的）
 **/
void divideBuf(char *buffer, int b_len, pthread_param params)
{
    char *f_beg;   /* 一个帧的开始位置 */
    char *f_end;   /* 一个帧的结束位置 */
    int f_len = 0; /* 帧长度 */
    bool isRight = true;

    f_beg = strstr(buffer, (char *)&Flag);
    while (f_beg != NULL)
    {
        f_end = strstr(f_beg + 1, (char *)&Flag);

        if (f_end)
        { /* 不为NULL */
            f_len = (f_end - f_beg);
        }
        else
        { /* 找不到Flag字段 */
            f_len = (b_len - (f_beg - buffer));
        }

        uint8_t addr;
        memcpy(&addr, f_beg + 1, 1);
        if (addr != params.sourceAddr)
        {
            printf("addr is error!\n");
            /* 在这里进行丢弃，rej操作 */
            break;
        }

        /* 进行FCS校验 */
        uint16_t tmpfcs = countFcs((unsigned char *)f_beg, f_len - 2);
        uint16_t fcs = 0;
        memcpy(&fcs, (f_beg + f_len - 2), 2);
        printf("fcs: %x   tmpfcs: %x\n", fcs, tmpfcs);
        if (fcs != tmpfcs)
        { //帧校验不通过，传输出错
            printf("传输出错，CRC校验不通过,向对方发送srej帧\n");
            /* 帧传输过程中出错，在这里进行丢弃，srej操作 */
            int num = (f_beg[2] & 0x70) >> 4;
            S_framing(params, SREJ, num);
            /* 更新指针，继续下一帧解析 */
            f_beg = strstr(f_beg + 1, (char *)&Flag);
            isRight = false;
            continue;
        }

        /**
         * 再添加一次判断，看收到的帧，是否为监督帧帧
         * 是：执行相应操作
         * 否：当信号帧处理
         **/
        if ((f_beg[2] & 0xc0) == 0x80)
        {
            deal_SFrame(f_beg, f_len);
            /* 更新指针，继续下一帧解析 */
            f_beg = strstr(f_beg + 1, (char *)&Flag);
            continue;
        }

        /* 后续有什么处理就加在这里，现在只简单的打印得到的帧 */
        printFrame(f_beg, f_len);

        int num = (f_beg[2] & 0x70) >> 4;
        recvQueue.push(num);

        /* 窗口左边收缩（遇到错误帧后，后面的怎均不改变窗口值） */
        if (isRight)
        {
            rCur += 1;
            rWin.beg += 1;
        }

        /* 判断窗口值大小,若窗口值为0,则丢弃后面的包 */
        if (rWin.beg == rWin.end)
            break;

        f_beg = f_end;
    }
    /* 这里判断如果收到的有信息帧时，就需要发送rr帧 */
    if (strstr(buffer + 1, (char *)&Flag) != NULL || (buffer[2] & 0xc0) != 0x80)
        S_framing(params, RR, rCur + 1); /* 发送RR帧 */
}

///////////////////////////////////////////////////////////////////////
/////////// 监督帧的构造和发送

/**
 * 构造并发送监督帧
 * params: addr: 目的地址
 *         t:    监督帧的类型
 *         num:  N(R)
 * 描述：该函数在构造完监督帧
 **/
void S_framing(pthread_param params, sType t, uint16_t num)
{
    char frame[10];
    frame[0] = (unsigned char)Flag;
    frame[1] = (unsigned char)params.aimAddr;
    frame[2] = (unsigned char)(0x80 | t | 0x08 | num);
    uint16_t fcs = countFcs((unsigned char *)frame, 3); //从地址字段开始校验，到信息字段结束
    frame[3] = (fcs & 0xff);                            /* 这里是因为，接收端校验和是小端xxxxxx */
    frame[4] = ((fcs >> 8) & 0xff);
    frame[5] = '\0';

    printf("构造的监督帧\n");
    printFrame(frame, 5);

    /* 将监督帧加入发送队列 */
    // sendQueue.push(frame);

    // 疑惑？？？？？？监督帧发送需不需要考虑窗口大小,我觉得不需要

    /* 将监督帧发送 */
    if (send(params.fd, frame, 5, 0) == -1)
    {
        char ct[20];
        switch (t)
        {
        case RR:
            memcpy(ct, "RR", 2);
            ct[2] = '\0';
            break;
        case RNR:
            memcpy(ct, "RNR", 3);
            ct[3] = '\0';
            break;
        case REJ:
            memcpy(ct, "REJ", 3);
            ct[3] = '\0';
            break;
        case SREJ:
            memcpy(ct, "SREJ", 4);
            ct[4] = '\0';
            break;
        }
        strcat(ct, "帧发送失败");
        handleError(ct);
    }
}

///////////////////////////////////////////////////////////////////////
/////////// 对监督帧的响应

/** 
 * 对监督帧的响应函数
 * params: frame 帧的起始地址
 *         f_len 帧长
 **/
void deal_SFrame(char *frame, int f_len)
{
    int num = (int)frame[2] & 0x30;
    switch (num)
    {
    case RR:
        printf("该监督帧为RR帧\n");
        feedback_RR((int)frame[2]);
        break;
    case RNR:
        printf("该监督帧为RNR帧\n");
        break;
    case REJ:
        printf("该监督帧为REJ帧\n");
        break;
    case SREJ:
        printf("该监督帧为SREJ帧\n");
        feedback_SREJ((int)frame[2]);
        break;
    default:
        break;
    }
}

/**
 * RR帧的处理
 * 描述： 该函数主要要做以下2个事情
 *       1. 更新窗口
 *       2. 清除缓存队列
 **/
void feedback_RR(int c)
{
    /* 窗口右边进行扩展 */
    int num = (c & 0x07);
    // printf("feedback_RR num: %d   %x\n", num, c);
    sWin.end = ((sWin.beg + 6) - (sWin.beg - num)) % 8;
    // printf("sWin beg: %d\n", sWin.beg);
    // printf("sWin end: %d\n", sWin.end);
    // printf("清空缓存前，缓存队列长度: %d\n", sendCache.size());

    /* 清除缓存 */
    while (!sendCache.empty())
    {
        printf("num is: %d\n", num);
        char *frame = sendCache.front();
        int i = (int)((frame[2] & 0x70) >> 4);
        printf("i is: %d\n", i);
        if (i < num)
            sendCache.pop();
        else
            break;
    }
    // printf("清空缓存后，缓存队列长度: %d\n", sendCache.size());
}

/**
 * SREJ帧的处理
 * 描述： 该函数主要要做以下2个事情
 *       1. 更新窗口
 *       2. 清除缓存队列
 **/
void feedback_SREJ(int c)
{
    int num = (c & 0x07);

    while (!sendCache.empty())
    {
        char *frame = sendCache.front();
        int i = (int)((frame[2] & 0x70) >> 4);
        if (i == num)
        {
            sendQueue.push(frame); // 将需要重发的帧加入发送队列
            break;
        }
        else if (i < num)
            sendCache.pop(); // 迫不得已系列....如果收到了srej[i]，就默认i之前都收到了
        else
        {
            printf("未找到相应缓存帧\n");
            exit(1);
        }
    }
}
