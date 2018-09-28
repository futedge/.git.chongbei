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
static void MakeHeartReply(u08 * pBuf, time_t NowTime)
{
	int i;
	for (i = 0; i < TPHTLN - 1; i++) {
		pBuf[i] = ((long)NowTime >> (TPHTLN - 2 - i) * 8) & 0xFF;
	}
	pBuf[i] = 1;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void InitStation(station_t pstStation, \
										FSMCondition_t * pstFSMStep)
{
	pstStation->id = pstFSMStep->id;
	pstStation->version = 1;
	pstStation->launch = time(NULL);
	pstStation->update = pstStation->launch;
	pstStation->StdCurrent[1] = 13;
	pstStation->MaxCurrent[1] = 45;
	pstStation->NetPrice[1] = 1;
	pstStation->NetTime[1] = 1;
	pstStation->NetDiscount[1] = 100;
	pstStation->CardPrice[1] = 100;
	pstStation->CardTime[1] = 240;
	pstStation->CoinPrice[1] = 100;
	pstStation->CoinTime[1] = 240;
	pstStation->pCmd = NULL;
	pstStation->pChk = NULL;
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
static void AnalyseBB(FSMCondition_t * pstFSMStep)
{
	station_t * pstStation;
	u08 buf[TPHTLN];
	if (!(pstStation = PosStation(pstFSMStep->id))) {
		pstStation = NewStation(pstFSMStep->id);
		InitStation(pstStation, pstFSMStep);
	}
	pstStation->update = time(NULL);
	pstStation->addr = pstFSMStep->addr;
	MakeHeartReply(buf, pstStation->update - BASETIME);
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void ClearData(FSMConditionCP_t * pstFSMStepCP)
{
	if (pstFSMStepCP->pBuf) {
		free(pstFSMStepCP->pBuf);
	}
	memset(pstFSMStepCP, 0, sizeof(FSMConditionCP_t));
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void SaveData(FSMConditionCP_t * pstFSMStepCP)
{
	pstFSMStepCP->ret = pstFSMStepCP->pBuf[0];
	if (pstFSMStepCP->ret) {
		switch (pstFSMStepCP->cmd) {
		case 0xB5 :
			if (pstFSMStepCP->len != 16) {
				ClearData(pstFSMStepCP);
				return;
			}
			pstFSMStepCP->OrderNo = pstFSMStepCP->pBuf[1] << 8 \
									| pstFSMStepCP->pBuf[2];
			pstFSMStepCP->SumMoney = pstFSMStepCP->pBuf[3] << 24 \
									| pstFSMStepCP->pBuf[4] << 16 \
									| pstFSMStepCP->pBuf[5] << 8 \
									| pstFSMStepCP->pBuf[6];
			pstFSMStepCP->TotalTime = pstFSMStepCP->pBuf[7] << 24 \
									| pstFSMStepCP->pBuf[8] << 16 \
									| pstFSMStepCP->pBuf[9] << 8 \
									| pstFSMStepCP->pBuf[10];
			pstFSMStepCP->EndTime = pstFSMStepCP->pBuf[11] << 24 \
									| pstFSMStepCP->pBuf[12] << 16 \
									| pstFSMStepCP->pBuf[13] << 8 \
									| pstFSMStepCP->pBuf[14];
			pstFSMStepCP->CurStatus = pstFSMStepCP->pBuf[15];
			break;
		case 0xC0 :
			if (pstFSMStepCP->len != 11) {
				ClearData(pstFSMStepCP);
				return;
			}
			pstFSMStepCP->NetPrice = pstFSMStepCP->pBuf[1] * 10;
			pstFSMStepCP->NetTime = pstFSMStepCP->pBuf[2] << 8 \
									| pstFSMStepCP->pBuf[3];
			pstFSMStepCP->NetDiscount = pstFSMStepCP->pBuf[4];
			pstFSMStepCP->CardPrice = pstFSMStepCP->pBuf[5] * 10;
			pstFSMStepCP->CardTime = pstFSMStepCP->pBuf[6] << 8 \
									| pstFSMStepCP->pBuf[7];
			pstFSMStepCP->CoinPrice = pstFSMStepCP->pBuf[8] * 10;
			pstFSMStepCP->CoinTime = pstFSMStepCP->pBuf[9] << 8 \
									| pstFSMStepCP->pBuf[10];
			break;
		case 0xC1 :
			if (pstFSMStepCP->len != 4) {
				ClearData(pstFSMStepCP);
				return;
			}
			pstFSMStepCP->CoinQryTal = pstFSMStepCP->pBuf[1] << 16 \
									| pstFSMStepCP->pBuf[2] << 8 \
									| pstFSMStepCP->pBuf[3];
			break;
		case 0xC2 :
			if (pstFSMStepCP->len != 4) {
				ClearData(pstFSMStepCP);
				return;
			}
			pstFSMStepCP->CardQryTal = pstFSMStepCP->pBuf[1] << 16 \
									| pstFSMStepCP->pBuf[2] << 8 \
									| pstFSMStepCP->pBuf[3];
			break;
		case 0xC3 :
			if (pstFSMStepCP->len != 3) {
				ClearData(pstFSMStepCP);
				return;
			}
			pstFSMStepCP->StdCurrent = pstFSMStepCP->pBuf[1];
			pstFSMStepCP->MaxCurrent = pstFSMStepCP->pBuf[2];
			break;
		case 0xC4 :
			break;
		case 0xC5 :
			if (pstFSMStepCP->len != 5) {
				ClearData(pstFSMStepCP);
				return;
			}
			pstFSMStepCP->SumMoney = pstFSMStepCP->pBuf[0] << 16 \
									| pstFSMStepCP->pBuf[1] << 8 \
									| pstFSMStepCP->pBuf[2];
			pstFSMStepCP->TotalTime = pstFSMStepCP->pBuf[3] << 8 \
									| pstFSMStepCP->pBuf[4];
			break;
		default :
			return;
		}
	}
}

/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void FSMChargePort(FSMConditionCP_t * pstFSMStepCP, \
							u08 data, FSMCondition_t * pstFSMStep)
{
	switch (pstFSMStepCP->eStepCP) {
	case EPortCP :
		pstFSMStepCP->port = data;
		pstFSMStepCP->eStepCP = EUserTypeCP;
		break;
	case EUserTypeCP :
		pstFSMStepCP->UserType = data;
		pstFSMStepCP->eStepCP = ECmdCP;
		break;
	case ECmdCP :
		pstFSMStepCP->cmd = data;
		pstFSMStepCP->eStepCP = ELenCP;
		break;
	case ELenCP :
		pstFSMStepCP->len = data;
		if (data) {
			pstFSMStepCP->eStepCP = EDataCP;
		}
		else {
			pstFSMStepCP->eStepCP = EChecksumCP;
		}
		break;
	case EDataCP :
		++pstFSMStepCP->count;
		if (pstFSMStepCP->len > 1 && pstFSMStepCP->pBuf == NULL) {
			if (!(pstFSMStepCP->pBuf = (u08 *)malloc\
											(pstFSMStepCP->len))) {
				perror("Not enough memory malloc in FSMChargePort");
				exit(1);
			}
		}
		if (pstFSMStepCP->count >= pstFSMStepCP->len) {
			SaveData(pstFSMStepCP);
			free(pstFSMStepCP->pBuf);
			pstFSMStepCP->pBuf = NULL;
			pstFSMStepCP->eStepCP = EChecksumCP;
		}
		break;
	case EChecksumCP :
		if (pstFSMStepCP->checksum == data && data != 0) {
			station_t * pstStation;
			if (!(pstStation = PosStation(pstFSMStep->id))) {
				pstStation = NewStation(pstFSMStep->id);
				InitStation(pstStation, pstFSMStep);
			}
			pstStation->addr = pstFSMStep->addr;
			ProcessAB(pstStation, pstFSMStepCP);
		}
		ClearData(pstFSMStepCP);
		break;
	default :
		return;
	}
	pstFSMStepCP->checksum ^= data;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void AnalyseAB(FSMCondition_t * pstFSMStep)
{
	int i;
	FSMConditionCP_t stFSMStepCP;
	stFSMStepCP.pBuf = NULL;
	for (i = 0; i < pstFSMStep->len; i++) {
		switch(pstFSMStep->pBuf[i]) {
		case 0xAA :
			stFSMStepCP.eStepCP = EPortCP;
			stFSMStepCP.checksum ^= 0xAA;
			continue;
			break;
		default :
		}
		FSMChargePort(&stFSMStepCP, pstFSMStep->pBuf[i], pstFSMStep);
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void ProcessDevCmd(FSMCondition_t * pFSMStep)
{
	switch(pFSMStep->tag) {
	case 0xAB :
		AnalyseAB(pFSMStep);
		break;
	case 0xBB :
		AnalyseBB(pFSMStep);
		break;
	default :
		return;
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void ClearFSMTrnsProt(FSMCondition_t * pstFSMStep)
{
	if (pstFSMStep->pBuf) {
		free(pstFSMStep->pBuf);
	}
	memset(pstFSMStep, 0, sizeof(FSMCondition_t));
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void FSMTrnsPort(FSMCondition_t * pstFSMStep, u08 data)
{
	switch(pstFSMStep->eCondition) {
	case EMark :
		ClearFSMTrnsProt(pstFSMStep);
		if (data == TPMARK) {
			pstFSMStep->eStep = EId;
			pstFSMStep->id = 1;
			pstFSMStep->checksum ^= data;
			return;
		}
		else {
			return;
		}
		break;
	case EXor :
		data ^= TPFTNM;
		break;
	case EEnd :
		return;
	default :
	}
	switch(pstFSMStep->eStep) {
	case EId :
		++pstFSMStep->count;
		if (pstFSMStep->count >= TPIDLN) {
			pstFSMStep->count = 0;
			pstFSMStep->eStep = ETag;
		}
		if (data >= 0x80) {
			pstFSMStep->type |= 1 << (TPIDLN - pstFSMStep->count);
			data &= 0x7F;
		}
		pstFSMStep->id = pstFSMStep->id * 100 + data;
		break;
	case ETag :
		++pstFSMStep->count;
		if (pstFSMStep->count >= TPTGLN) {
			pstFSMStep->count = 0;
			pstFSMStep->eStep = ELen;
		}
		pstFSMStep->tag |= data << ((TPTGLN - pstFSMStep->count) * 8);
		break;
	case ELen :
		++pstFSMStep->count;
		if (pstFSMStep->count >= TPLNLN) {
			pstFSMStep->count = 0;
			pstFSMStep->eStep = EData;
		}
		pstFSMStep->len |= data << ((TPLNLN - pstFSMStep->count) * 8);
		break;
	case EData :
		if (pstFSMStep->pBuf == NULL) {
			pstFSMStep->len -= 2;
			if (!(pstFSMStep->pBuf = (u08 *)malloc(pstFSMStep->len))) {
				perror("Not enough memory malloc for \
												pstFSMStep->pBuf:");
				exit(1);
			}
		}
		pstFSMStep->pBuf[pstFSMStep->count] = data;
		++pstFSMStep->count;
		if (pstFSMStep->count >= pstFSMStep->len) {
			pstFSMStep->count = 0;
			pstFSMStep->eStep = EChecksum;
		}
		break;
	case EChecksum :
		if (pstFSMStep->checksum == data) {
			ProcessDevCmd(pstFSMStep);
		}
		pstFSMStep->eStep = EStart;
		break;
	default :
		return;
	}
	pstFSMStep->checksum ^= data;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void AnalyseTrnsProt(recv_t * pstRecv)
{
	int i;
	// 此处必须用堆，否则存在内存泄漏
	static FSMCondition_t stFSMStep;
	for (i = 0; i < pstRecv->len; i++) {
		switch(pstRecv->pBuf[i]) {
		case TPHEAD :
			stFSMStep.eCondition = EMark;
			stFSMStep.addr = pstRecv->addr;
			continue;
			break;
		case TPFILT :
			stFSMStep.eCondition = EXor;
			continue;
			break;
		case TPTAIL :
			stFSMStep.eCondition = EEnd;
			ClearFSMTrnsProt(&stFSMStep);
			continue;
			break;
		default :
			stFSMStep.eCondition = EContent;
		}
		FSMTrnsPort(&stFSMStep, pstRecv->pBuf[i]);
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
		AnalyseTrnsProt(pstRecv);
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
