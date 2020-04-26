/* mathxxx.c与其头文件放在了不同文件夹下 */
#include <stdio.h>
#include <math.h>
// 这里引入了c语言标准库中的math.h头文件
// **记得在CMakeLists里面引入math的库文件！！！**
#include "MathFunctions.h"
// 这里没有使用相对路径，因为设置了include_directories

int power1(int a, int b) {
	if (!aa)
		return (int)pow(a, b);
	else
		return 1;
}