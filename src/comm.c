#include "comm.h"
#include "ManageCommunicationPrivate.h"
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
int MakeSemid(key_t key)
{
	int semid;
	static int num = 0;
	int KeyNum = key + num;
	
	semid = semget(KeyNum, 1, IPC_CREAT | 0666);
	++num;
	if (semid < 0) {
		printf("semget %d error", KeyNum);
		exit(1);
	}
	return semid;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
void SetSemidVal(int semid, int val)
{
	SemidVal_t SemidVal;
	
	if (semid < 0) {
		return;
	}
	SemidVal.val = val;
	semctl(semid, 0, SETVAL, SemidVal);
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
void P(int semid)
{
	struct sembuf SemBuf = {0, -1, SEM_UNDO};
	semop(semid, &SemBuf, 1);
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
void V(int semid)
{
	struct sembuf SemBuf = {0, 1, SEM_UNDO};
	semop(semid, &SemBuf, 1);
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
station_t * PosStation(u64 id)
{
	station_t * pStation;
	pStation = DVHD.pstHead->pstNext;
	while (pStation != DVHD.pstHead) {
		if (pStation->id == id) {
			return pStation;
		}
	}
	return NULL;
}
//void MakeCmd(u08 * pBuf, u08 cmd, )
//{
//}
/***************************scale***********************************/
