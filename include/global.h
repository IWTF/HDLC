#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#define BUFFER_SIZE 1024 /* 通信用的字节流大小 */
#define MAX_INFO_SIZE 64 /* 信息字段的最大size */
#define CLIENT_ADDR 0x81
#define SERVER_ADDR 0x82
struct Window /* 只实现3位序号 */
{
	unsigned int beg; /* 滑动窗口起始指针 */
	unsigned int end; /* 滑动窗口末尾指针 */
	Window () {
		this->beg = 0;
		this->end = 7;
	}
};

#endif