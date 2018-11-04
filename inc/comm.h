#ifndef _COMM_H
#define _COMM_H

#include "type.h"

// 信号量设置联合体
typedef union {
	int					val;
	unsigned short   *	pArray;
	struct	semid_ds *	pSemid;
} SemidVal_t;

// 站命令设置参数结构体
typedef struct {
	u08		cmd;
	u08		port;
	u16		val;
} CmdParam_t, * pCmdParam_t;

int MakeSemid(key_t key);
void SetSemidVal(int semid, int val);
void P(int semid);
void V(int semid);
station_t * PosStation(u64 id);
#endif
/***************************scale***********************************/
