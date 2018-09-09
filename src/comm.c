#include "comm.h"
#include "ManageCommunicationPrivate.h"

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

void SetSemidVal(int semid, int val)
{
	SemidVal_t SemidVal;
	
	if (semid < 0) {
		return;
	}
	SemidVal.val = val;
	semctl(semid, 0, SETVAL, SemidVal);
}

void P(int semid)
{
	struct sembuf SemBuf = {0, -1, SEM_UNDO};
	semop(semid, &SemBuf, 1);
}

void V(int semid)
{
	struct sembuf SemBuf = {0, 1, SEM_UNDO};
	semop(semid, &SemBuf, 1);
}

station_t * position(FSMCondition_t * pstFSMStep)
{
	station_t * pStation;
	pStation = DVHD.pHead->pNext;
	while (DVHD.pHead != pStation) {
		if (pStation->id == pstFSMStep->id) {
			return pStation;
		}
	}
	return NULL;
}

station_t * NewStation(void)
{
	station_t * pStation;
	if (!(pStation = (station_t *)malloc(sizeof(station_t)))) {
		perror("Not enough memory malloc for NewStation:");
		exit(1);
	}
	memset(pStation, 0, sizeof(station_t));
	pStation->pPrev = DVHD.pHead->pPrev;
	pStation->pNext = DVHD.pHead;
	DVHD.pHead->pPrev->pNext = pStation;
	DVHD.pHead->pPrev = pStation;
	return pStation;
}

//void MakeCmd(u08 * pBuf, u08 cmd, )
//{
//}
/***************************scale***********************************/
