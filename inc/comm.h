#ifndef _COMM_H
#define _COMM_H

#include "type.h"

// 信号量设置联合体
typedef union tagSemidVal_t{
	int					val;
	unsigned short   *	pArray;
	struct	semid_ds *	pSemid;
} SemidVal_t;

int MakeSemid(key_t key);
void SetSemidVal(int semid, int val);
void P(int semid);
void V(int semid);
#endif
/***************************scale***********************************/
