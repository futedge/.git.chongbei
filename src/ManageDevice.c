#include "ManageDevicePrivate.h"
#include "ManageDevice.h"

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

static void CP_Analyse_BB(FSMCondition_t * pFSMStep)
{
	time_t NowTime;
	station_t * pStation;
	u08 buf[TPHTLN];
	NowTime = time(NULL);
	if (!(pStation = position(pFSMStep->id))) {
		pStation = NewStation(pFSMStep->id);
	}
	if (!pStation) {
		return;
	}
	pStation->update = NowTime;
	MakeHeartReply(buf, NowTime - BASETIME);
	pack(pStation, 0xBB, buf);
}

static void MakeHeartReply(u08 * pBuf, time_t NowTime)
{
	int i;
	for (i = 0; i < TPHTLN - 1; i++) {
		pBuf[i] = ((long)NowTime >> (TPHTLN - 2 - i) * 8);
	}
	pBuf[i] = 1;
}

static void CP_Analyse_AB(FSMCondition_t * pFSMStep)
{
	
}

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

static void TP_ClearFSM(FSMCondition_t * pFSMStep)
{
	pFSMStep->condition = eEnd;
	pFSMStep->step = eError;
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

static status_t TP_FSM(FSMCondition_t * pFSMStep, u08 data)
{
	switch(pFSMStep->condition) {
	case eMark :
		TP_ClearFSM(pFSMStep);
		if (data == TPMARK) {
			pFSMStep->step = eID;
			pFSMStep->id = 1;
			pFSMStep->checksum ^= data;
			return eContinue;
		}
		else {
			return eErrTPMark}
		break;
	case eXor :
		data ^= TPFTNM;
		break;
	case eContent :
		break;
	case eEnd :
	default :
		return eContinue;
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
			return eProcDev;
		}
		else {
			TP_ClearFSM(pFSMStep);
			return eErrChksum;
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

static void TP_Analyse(recv_t * pRecv)
{
	int i;
	// 此处必须用堆，否则存在内存泄漏
	static FSMCondition_t FSMStep;
	for (i = 0; i < pRecv->len; i++) {
		switch(pRecv->pBuf[i]) {
		case TPHEAD :
			FSMStep.condition = eMark;
			continue;
			break;
		case TPFILT :
			FSMStep.condition = eXor;
			continue;
			break;
		case TPTAIL :
			FSMStep.condition = eEnd;
			continue;
			break;
		default :
			FSMStep.condition = eContent;
		}
		switch(TP_FSM(&FSMStep, pBuf[i])) {
		case eContinue :
			break;
		case eErrTPMark :
			break;
		case eErrChksum :
			break;
		case eProcDev :
			ProcessDevCmd(&FSMStep);
			break;
		default :
		}
	}
}

static void ProcessDevMsg(void)
{
	recv_t * pRecv;
	int count = 0;
	bool flag = FALSE;
	P(SEMRCST);
	pRecv = RCST.pHead->pstNext;
	V(SEMRCST);
	while (pRecv != RCST.pHead) {
		++count;
		// 临界点，ListenDevice线程会在此添加新节点
		if (RCST.num == count) {
			flag = TRUE;
		}
		if (flag) {
			P(SEMRCST);
		}
		TP_Analyse(pRecv);
		pRecv = pRecv->pstNext;
		if (flag) {
			V(SEMRCST);
			flag = FALSE;
		}
	}
}

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
