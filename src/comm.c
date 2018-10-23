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
	pStation = DV_HD.pstHead->pstNext;
	while (pStation != DV_HD.pstHead) {
		if (pStation->id == id) {
			return pStation;
		}
	}
	return NULL;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
u08 CrcXor(u08 * pBuf, u08 len)
{
	u08 i;
	u08 val = pBuf[0];
	for (i = 1; i < len; i++) {
		val ^= pBuf[i];
	}
	return val;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
void MakeCmd(station_t * pstStation, u08 cmd, u08 port, u16 val)
{
	u08 * pBuf;
	int i = 0;
	if (!(pBuf = (u8 *)malloc(BUF_MAX))) {
		perror("Not enough memory malloc for MakeCmd:");
		exit(1);
	}
	pBuf[i++] = 0xAA;
	if (port > 10 || port < 1) port = 1;
	pBuf[i++] = port;
	pBuf[i++] = 0;
	pBuf[i++] = cmd;
	switch (cmd) {
	case 0xA0 :
		pBuf[i++] = 5;
		pBuf[i++] = val >> 8;
		pBuf[i++] = val & 0xFF;
		pBuf[i++] = 0;
		pBuf[i++] = 0;
		pBuf[i++] = 0;
		break;
	case 0xA4 :
		pBuf[i++] = 2;
		pBuf[i++] = pstStation->StdCurrent[1];
		pBuf[i++] = pstStation->MaxCurrent[1];
		break;
	case 0xA5 :
		pBuf[i++] = 10;
		if（pstStation->NetPrice[1] < 10） {
			pstStation->NetPrice[1] = 10;
		}
		if（pstStation->CardPrice[1] < 10） {
			pstStation->NetPrice[1] = 10;
		}
		if（pstStation->CoinPrice[1] < 10） {
			pstStation->NetPrice[1] = 10;
		}
		pBuf[i++] = pstStation->NetPrice[1] / 10;
		pBuf[i++] = pstStation->NetTime[1] >> 8;
		pBuf[i++] = pstStation->NetTime[1] & 0xFF;
		pBuf[i++] = pstStation->NetDiscount[1];
		pBuf[i++] = pstStation->CardPrice[1] / 10;
		pBuf[i++] = pstStation->CardTime[1] >> 8;
		pBuf[i++] = pstStation->CardTime[1] & 0xFF;
		pBuf[i++] = pstStation->CoinPrice[1] / 10;
		pBuf[i++] = pstStation->CoinTime[1] >> 8;
		pBuf[i++] = pstStation->CoinTime[1] & 0xFF;
		break;
	case 0xB5 :
		pBuf[i++] = 1;
		pBuf[i++] = val;
		break;
	case 0xC0 :
		pBuf[i++] = 1;
		pBuf[i++] = val;
		break;
	case 0xC1 :
		pBuf[i++] = 1;
		pBuf[i++] = val;
		break;
	case 0xC2 :
		pBuf[i++] = 1;
		pBuf[i++] = val;
		break;
	case 0xC3 :
		pBuf[i++] = 1;
		pBuf[i++] = val;
		break;
	case 0xC4 :
		pBuf[i++] = 1;
		pBuf[i++] = val;
		break;
	case 0xC5 :
		pBuf[i++] = 1;
		pBuf[i++] = val;
		break;
	case 0xF7 :
		pBuf[i++] = 1;
		pBuf[i++] = val;
		break;
	default :
		pBuf[i++] = 1;
		pBuf[i++] = val;
	}
	pBuf[i++] = CrcXor(pBuf, i);
}
/***************************scale***********************************/
