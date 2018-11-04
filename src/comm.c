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
static u08 V1_CrcXor(u08 * pBuf, u08 len)
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
static int V1_PackHeartCmd(u08 * pBuf, time_t NowTime)
{
	int i;
	for (i = 0; i < TP_HT_LN - 1; i++) {
		pBuf[i] = ((long)NowTime >> (TP_HT_LN - 2 - i) * 8) & 0xFF;
	}
	pBuf[i++] = 1;
	return i;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static int V1_PackStationCmd(u08 *pBuf, station_t * pstStation, \
											pCmdParam_t pCmdParam)
{
	int i = 0;
	pBuf[i++] = 0xAA;
	if (pCmdParam->port > 10 || pCmdParam->port < 1) {
		pCmdParam->port = 1;
	}
	pBuf[i++] = pCmdParam->port;
	pBuf[i++] = 0;
	pBuf[i++] = pCmdParam->cmd;
	switch (pCmdParam->cmd) {
	case 0xA0 :
		pBuf[i++] = 5;
		pBuf[i++] = pCmdParam->val >> 8;
		pBuf[i++] = pCmdParam->val & 0xFF;
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
		if (pstStation->NetPrice[1] < 10) {
			pstStation->NetPrice[1] = 10;
		}
		if (pstStation->CardPrice[1] < 10) {
			pstStation->NetPrice[1] = 10;
		}
		if (pstStation->CoinPrice[1] < 10) {
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
		pBuf[i++] = pCmdParam->val;
		break;
	case 0xC0 :
		pBuf[i++] = 1;
		pBuf[i++] = pCmdParam->val;
		break;
	case 0xC1 :
		pBuf[i++] = 1;
		pBuf[i++] = pCmdParam->val;
		break;
	case 0xC2 :
		pBuf[i++] = 1;
		pBuf[i++] = pCmdParam->val;
		break;
	case 0xC3 :
		pBuf[i++] = 1;
		pBuf[i++] = pCmdParam->val;
		break;
	case 0xC4 :
		pBuf[i++] = 1;
		pBuf[i++] = pCmdParam->val;
		break;
	case 0xC5 :
		pBuf[i++] = 1;
		pBuf[i++] = pCmdParam->val;
		break;
	case 0xF7 :
		pBuf[i++] = 1;
		pBuf[i++] = pCmdParam->val;
		break;
	default :
		pBuf[i++] = 1;
		pBuf[i++] = pCmdParam->val;
	}
	pBuf[i++] = V1_CrcXor(pBuf, i);
	return i;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void V1_SetID(u08 *pBuf, u64 id)
{
	int n = TP_ID_LN - 1;
	// 从个、十位开始倒序存入ID
	do {
		pBuf[n--] = id % 100;
		id /= 100;
		// 设备类型存入ID位对应的最高位,最高位置1
		if (DEV_TYPE >> (TP_ID_LN - n - 2) & 0x01) {
			pBuf[n + 1] |= 0x80;
		}
	} while (n >= 0);
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
u08 * V1_MakeCmd(station_t * pstStation, pCmdParam_t pCmdParam, \
														int * pLen)
{
	u08 buf[BUF_MAX], * pBuf;
	int i = 0, KeyWrdNum = 0, tmp1, tmp2;
	buf[i++] = TP_HEAD;
	buf[i++] = TP_MARK;
	V1_SetID(buf + i, pstStation->id);
	i += TP_ID_LN;
	if (pCmdParam->cmd) {
		buf[i++] = TP_TG_AB >> 8;
		buf[i++] = TP_TG_AB & 0xFF;
		tmp1 = i;
		i += 2;
		tmp2 = V1_PackStationCmd(buf + i, pstStation, pCmdParam);
		i += tmp2;
	}
	else {
		buf[i++] = TP_TG_BB >> 8;
		buf[i++] = TP_TG_BB & 0xFF;
		tmp1 = i;
		i += 2;
		tmp2 = V1_PackHeartCmd(buf + i, \
								pstStation->update - BASE_TIME);
		i += tmp2;
	}
	tmp2 += LEN_ADD;
	buf[tmp1] = tmp2 >> 8;
	buf[tmp1 + 1] = tmp2 & 0xFF;
	buf[i++] = V1_CrcXor(buf + 1, i - 1);
	buf[i++] = TP_TAIL;
	// 计算需要转义的字节个数
	for (tmp1 = 1; tmp1 < i - 1; tmp1++) {
		if (buf[tmp1] == 0x8D || buf[tmp1] == 0x8E \
											|| buf[tmp1] == 0x8F) {
			KeyWrdNum++;
		}
	}
	// 申请转义字节后所需内存大小
	* pLen = i + KeyWrdNum;
	if (!(pBuf = (u08 *)malloc(* pLen))) {
		perror("Not enough memory malloc for V1_MakeCmd:");
		exit(1);
	}
	for (tmp1 = 1, tmp2 = 1; tmp1 < i - 1; tmp1++, tmp2++) {
		if (buf[tmp1] == TP_FILT || buf[tmp1] == TP_HEAD \
										|| buf[tmp1] == TP_TAIL) {
			pBuf[tmp2++] = TP_FILT;
			pBuf[tmp2] = buf[tmp1] ^ TP_FT_NM;
		}
		else {
			pBuf[tmp2] = buf[tmp1];
		}
	}
	pBuf[0] = TP_HEAD;
	pBuf[tmp2] = TP_TAIL;
	return pBuf;
}
/***************************scale***********************************/
