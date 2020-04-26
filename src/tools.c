#include <stdio.h>
#include<stdlib.h>
#include "tools.h"

/* 错误处理函数 */
void handleError(char *msg) {
    perror(msg);
    exit(-1);
}

