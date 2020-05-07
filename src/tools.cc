#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "tools.h"

/* 错误处理函数 */
void handleError(char *msg) {
    perror(msg);
    exit(-1);
}

/*打印菜单*/
void printMenu()
{
    printf("---------------------Menu--------------------\n");
    printf("1.主动与对方建立HDLC连接\n");
    printf("2.等待对方发出HDLC链接\n");
    printf("3.拆链\n");
    printf("4.发送某文件内容给对方，command：[3]+文件名\n");
    printf("5.输入发送内容\n");
    printf("6.处理断开链接请求（暂时先这样）\n");
    printf("7.释放信号量\n");
    printf("---------------------------------------------\n");
}

/**
 * 打印帧
 * params:frame 字符数组类型的帧
 *        framelen 帧的长度（单位字节）即字符数组数组大小
 * 功能：打印帧便于调试，了解中间过程
 **/
void printFrame(char *frame,int framelen)
{
    uint8_t flag=frame[0]&0xff;
    uint8_t addr=frame[1]&0xff;         //frame[0]是Flag
    uint8_t ctrl=frame[2]&0xff;

    char *fcscp=frame+(framelen-2);     //从后向前，最后两个字节是fcs
    uint16_t fcs=((fcscp[0]<<8)&0xff00)|(fcscp[1]&0xff);

    printf("------------------------------------------------\n");
    printf("标志字段：0x%x\n",flag);
    printf("目标地址：0x%x\n",addr);
    printf("控制字段：0x%x\n",ctrl);
    printf("信息内容：");
    char *info=(frame+3);
    for (; info != (frame+framelen-2); info+=1) {
        printf("%c", *info);
    }
    // for(int i=0;info[i]!='\0';i++)
    // {
    //     printf("%c",(info[i]));
    // }
    printf("\n");
    printf("校验序列：0x%x\n",fcs);
    printf("------------------------------------------------\n");
}
