#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/sem.h>

#include "tools.h"

union semun
{
	int val;
	struct semid_ds *buf;
	unsigned short *arry;
};

// 关于信号量的使用，可参考: https://blog.csdn.net/ljianhui/article/details/10243617

/**
 * 给信号量赋初值
 * params: sem_id 信号量标识符
 **/
int set_semvalue(int sem_id) {
	//用于初始化信号量，在使用信号量前必须这样做
	union semun sem_union;
 
	sem_union.val = 0; /* 窗口大小 */
	if(semctl(sem_id, 0, SETVAL, sem_union) == -1)
		return 0;
	return 1;
}

/**
 * 删除已存在的信号量
 * params: sem_id 信号量标识符
 **/
void del_semvalue(int sem_id) {
	//删除信号量
	union semun sem_union;
 
	if(semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
		handleError((char *)"Failed to delete semaphore");
}
 
/**
 * P操作
 * params: sem_id 信号量标识符
 * 描述： 是对P操作的封装，真正的操作是semop()
 **/
int semaphore_p(int sem_id) {
	//对信号量做减1操作，即等待P（sv）
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = -1;//P()
	sem_b.sem_flg = SEM_UNDO;
	if(semop(sem_id, &sem_b, 1) == -1) {
		handleError((char *)"semaphore_p failed");
		return 0;
	}
	return 1;
}

/**
 * V操作
 * params: sem_id 信号量标识符
 **/
int semaphore_v(int sem_id) {
	//这是一个释放操作，它使信号量变为可用，即发送信号V（sv）
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = 1;//V()
	sem_b.sem_flg = SEM_UNDO;
	if(semop(sem_id, &sem_b, 1) == -1) {
		handleError((char *)"semaphore_v failed");
		return 0;
	}
	return 1;
}