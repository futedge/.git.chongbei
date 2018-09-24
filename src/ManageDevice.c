#include "ManageDevicePrivate.h"
#include "ManageDevice.h"
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void PrintDevMsg(void)
{
	recv_t * pRecv;
	struct tm * LocalTime;
	char buf[32];
	int count = 0;
	P(SEMRCST);
	pRecv = RCST.pHead->pstNext;
	V(SEMRCST);
	printf("***************start***************\n");
	while (pRecv != RCST.pHead) {
		++count;
		// 临界点，ListenDevice线程会在此添加新节点
		if (RCST.num == count) {
			P(SEMRCST);
		}
		LocalTime = localtime(&pRecv->time);
		strftime(buf, 31, "%Y-%m-%d %H:%M:%S", LocalTime);
		printf("Update:%s, len=%d, data:%s, addr=%s, ", buf, \
									pRecv->len, pRecv->pBuf, \
								inet_ntoa(pRecv->addr.sin_addr));
		pRecv = pRecv->pstNext;
		if (RCST.num == count) {
			V(SEMRCST);
		}
	}
	printf("****************end****************\n\n\n");
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void DeleteThis(recv_t * pRecv)
{
	pRecv->pstPrev->pstNext = pRecv->pstNext;
	pRecv->pstNext->pstPrev = pRecv->pstPrev;
	free(pRecv->pBuf);
	free(pRecv);
	RCST.num--;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static station_t * NewStation(void)
{
	station_t * pStation;
	if (!(pStation = (station_t *)malloc(sizeof(station_t)))) {
		perror("Not enough memory malloc for NewStation:");
		exit(1);
	}
	memset(pStation, 0, sizeof(station_t));
	pStation->pstPrev = DVHD.pHead->pstPrev;
	pStation->pstNext = DVHD.pHead;
	DVHD.pHead->pstPrev->pstNext = pStation;
	DVHD.pHead->pstPrev = pStation;
	return pStation;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void CP_Analyse_BB(FSMCondition_t * pFSMStep)
{
	time_t NowTime;
	station_t * pStation;
	u08 buf[TPHTLN];
	NowTime = time(NULL);
	if (!(pStation = PosStation(pFSMStep->id))) {
		pStation = NewStation(pFSMStep->id);
	}
	if (!pStation) {
		return;
	}
	pStation->update = NowTime;
	MakeHeartReply(buf, NowTime - BASETIME);
	pack(pStation, 0xBB, buf);
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void MakeHeartReply(u08 * pBuf, time_t NowTime)
{
	int i;
	for (i = 0; i < TPHTLN - 1; i++) {
		pBuf[i] = ((long)NowTime >> (TPHTLN - 2 - i) * 8);
	}
	pBuf[i] = 1;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void CP_Analyse_AB(FSMCondition_t * pFSMStep)
{
	
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void ProcessDevCmd(FSMCondition_t * pFSMStep)
{
	switch(pFSMStep->tag) {
	case 0xBB :
		CP_Analyse_BB(pFSMStep);
		break;
	case 0xAB :
		CP_Analyse_AB(pFSMStep);
		break;
	default :
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void TP_ClearFSM(FSMCondition_t * pFSMStep)
{
	pFSMStep->condition = EError;
	pFSMStep->step = EId;
	pFSMStep->count = 0;
	pFSMStep->checksum = 0;
	pFSMStep->id = 0;
	pFSMStep->type = 0;
	pFSMStep->tag = 0;
	pFSMStep->len = 0;
	if (pFSMStep->pBuf) {
		free(pFSMStep->pBuf);
		pFSMStep->pBuf = NULL;
	}
	pFSMStep->checksum = 0;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void TP_FSM(FSMCondition_t * pFSMStep, u08 data)
{
	switch(pFSMStep->condition) {
	case EMark :
		TP_ClearFSM(pFSMStep);
		if (data == TPMARK) {
			pFSMStep->step = eID;
			pFSMStep->id = 1;
			pFSMStep->checksum ^= data;
			return;
		}
		else {
			return;
		}
		break;
	case eXor :
		data ^= TPFTNM;
		break;
	case eEnd :
		return;
	default :
		return;
	}
	switch(pFSMStep->step) {
	case eID :
		++pFSMStep->count;
		if (pFSMStep->count >= TPIDLN) {
			pFSMStep->count = 0;
			pFSMStep->step = eTag;
		}
		if (data >= 0x80) {
			pFSMStep->type |= 1 << (TPIDLN - pFSMStep->count);
			data &= 0x7F;
		}
		pFSMStep->id = pFSMStep->id * 100 + data;
		break;
	case eTag :
		++pFSMStep->count;
		if (pFSMStep->count >= TPTGLN) {
			pFSMStep->count = 0;
			pFSMStep->step = eLen;
		}
		pFSMStep->tag |= data << ((TPTGLN - pFSMStep->count) * 8);
		break;
	case eLen :
		++pFSMStep->count;
		if (pFSMStep->count >= TPLNLN) {
			pFSMStep->count = 0;
			pFSMStep->step = eData;
		}
		pFSMStep->len |= data << ((TPLNLN - pFSMStep->count) * 8);
		break;
	case eData :
		if (pFSMStep->pBuf == NULL) {
			pFSMStep->len -= 2;
			if (!(pFSMStep->pBuf = (u08 *)malloc(pFSMStep->len))) {
				perror("Not enough memory malloc for \
												pFSMStep->pBuf:");
				exit(1);
			}
		}
		pFSMStep->pBuf[pFSMStep->count] = data;
		++pFSMStep->count;
		if (pFSMStep->count >= pFSMStep->len) {
			pFSMStep->count = 0;
			pFSMStep->step = eCheckSum;
		}
		break;
	case eChecksum :
		if (pFSMStep->CheckSum == data) {
			ProcessDevCmd(&FSMStep);
		}
		else {
			TP_ClearFSM(pFSMStep);
		}
		break;
	case eError :
	default :
		TP_ClearFSM(pFSMStep);
		return eContinue;
	}
	pFSMStep->checksum ^= data;
	return eContinue;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void TP_Analyse(recv_t * pstRecv)
{
	int i;
	// 此处必须用堆，否则存在内存泄漏
	static FSMCondition_t stFSMStep;
	for (i = 0; i < pRecv->len; i++) {
		switch(pstRecv->pBuf[i]) {
		case TPHEAD :
			stFSMStep.eCondition = EMark;
			continue;
			break;
		case TPFILT :
			stFSMStep.eCondition = EXor;
			continue;
			break;
		case TPTAIL :
			stFSMStep.eCondition = EEnd;
			continue;
			break;
		default :
			stFSMStep.eCondition = EContent;
		}
		TP_FSM(&stFSMStep, pstRecv->pBuf[i]);
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void ProcessDevMsg(void)
{
	recv_t * pstRecv, * pstTmp;
	P(SEMRCST);
	pstRecv = RCST.pstHead->pstNext;
	V(SEMRCST);
	while (pstRecv != RCST.pstHead) {
		pstTmp = pstRecv->pstNext;
		// 临界点，ListenServer线程会在此添加新节点
		if (pstTmp == RCST.pstHead) {
			P(SEMRCST);
		}
		TP_Analyse(pstRecv);
		DeleteThis(pstRecv);
		pstRecv = pstTmp;
		if (pstTmp == RCST.pstHead) {
			V(SEMRCST);
		}
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
void * ManageDevice(void * arg)
{
	while(1) {
		pthread_mutex_lock(&DEVIMUTEX);
		if (THDDEVISLP) {
			pthread_cond_wait(&DEVICOND, &DEVIMUTEX);
		}
		ProcessDevMsg();
		THDDEVISLP = TRUE;
		pthread_mutex_unlock(&DEVIMUTEX);
	}
	return (void *)0;
}
/***************************scale***********************************/
