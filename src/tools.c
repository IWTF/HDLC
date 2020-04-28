#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "tools.h"

/* 错误处理函数 */
void handleError(char *msg) {
    perror(msg);
    exit(-1);
}

void show_menu() {
    printf("--------------------menu----------------------\n");
    printf("|\t1. 与对方建立HDLC连接\n");
    printf("|\t2. 等待对方连接请求\n");
    printf("|\t3. 拆链\n");
    printf("--------------------menu----------------------\n");
}

/**
 * 打印帧结构
 * params: f_beg 帧开始地址
 *         f_end 帧结束地址（'\0'的位置）
 * (f_end-f_beg)等于帧长
 * 描述： 打印输出帧的结构，方便调试
 **/
void printFrame(char *f_beg, char *f_end) {
    uint8_t tmp = 0;
    tmp = *f_beg++;
    printf("Flag:    %02x\n", tmp);
    tmp = *f_beg++;
    printf("Address: %02x\n", tmp);
    tmp = *f_beg++;
    printf("Control: %02x\n", tmp);

    printf("Information: ");
    for (; f_beg != (f_end-2); f_beg+=1)
        printf("%c", *f_beg);
    puts("");

    uint16_t fcs = 0;
    memcpy(&fcs, f_beg, 2);
    printf("FCS:     %04x\n", fcs);
}