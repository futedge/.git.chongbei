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
		pBuf[i] = ((long)NowTime >> (TPHTLN - 2 - i) * 8);
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
	xxxxxxxxxxxxxxxx
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
	time_t NowTime;
	station_t * pstStation;
	u08 buf[TPHTLN];
	if (!(pstStation = PosStation(pstFSMStep->id))) {
		pstStation = NewStation(pstFSMStep->id);
		InitStation(pstStation, pstFSMStep);
	}
	NowTime = time(NULL);
	pstStation->update = NowTime;
	MakeHeartReply(buf, NowTime - BASETIME);
	pack(pstStation, 0xBB, buf);
	xxxxxxxxxxxxxxx
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void ClearData(FSMConditionCP_t * pstFSMStep)
{
	pstFSMStep->OrderNo = 0;
	pstFSMStep->SumMoney = 0;
	pstFSMStep->TotalTime = 0;
	pstFSMStep->EndTime = 0;
	pstFSMStep->CurStatus = 0;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void SaveData(FSMConditionCP_t * pstFSMStep)
{
	pstFSMStep->ret = pstFSMStep->pBuf[0];
	if (pstFSMStep->ret) {
		switch (pstFSMStep->cmd) {
		case 0xB5 :
			if (pstFSMStep->len != 16) {
				ClearData(pstFSMStep);
				return;
			}
			pstFSMStep->OrderNo = pstFSMStep->pBuf[1] << 8 | \
								  pstFSMStep->pBuf[2];
			pstFSMStep->SumMoney = pstFSMStep->pBuf[3] << 24 | \
								   pstFSMStep->pBuf[4] << 16 | \
								   pstFSMStep->pBuf[5] << 8  | \
								   pstFSMStep->pBuf[6];
			pstFSMStep->TotalTime = pstFSMStep->pBuf[7] << 24 | \
									pstFSMStep->pBuf[8] << 16 | \
									pstFSMStep->pBuf[9] << 8  | \
									pstFSMStep->pBuf[10];
			pstFSMStep->EndTime = pstFSMStep->pBuf[11] << 24 | \
								  pstFSMStep->pBuf[12] << 16 | \
								  pstFSMStep->pBuf[13] << 8  | \
								  pstFSMStep->pBuf[14];
			pstFSMStep->CurStatus = pstFSMStep->pBuf[15];
			break;
		case 0xC0 :
			if (pstFSMStep->len != 11) {
				ClearData(pstFSMStep);
				return;
			}
			pstFSMStep->NetPrice = pstFSMStep->pBuf[1] * 10;
			pstFSMStep->NetTime = pstFSMStep->pBuf[2] << 8 | \
								  pstFSMStep->pBuf[3];
			pstFSMStep->NetDiscount = pstFSMStep->pBuf[4];
			pstFSMStep->CardPrice = pstFSMStep->pBuf[5] * 10;
			pstFSMStep->CardTime = pstFSMStep->pBuf[6] << 8 | \
								   pstFSMStep->pBuf[7];
			pstFSMStep->CoinPrice = pstFSMStep->pBuf[8] * 10;
			pstFSMStep->CoinTime = pstFSMStep->pBuf[9] << 8 | \
								   pstFSMStep->pBuf[10];
			break;
		case 0xC1 :
			if (pstFSMStep->len != 4) {
				ClearData(pstFSMStep);
				return;
			}
			pstFSMStep->CoinQryTal = pstFSMStep->pBuf[1] << 16 | \
									 pstFSMStep->pBuf[2] << 8  | \
									 pstFSMStep->pBuf[3];
			break;
		case 0xC2 :
			if (pstFSMStep->len != 4) {
				ClearData(pstFSMStep);
				return;
			}
			pstFSMStep->CardQryTal = pstFSMStep->pBuf[1] << 16 | \
									 pstFSMStep->pBuf[2] << 8  | \
									 pstFSMStep->pBuf[3];
			break;
		case 0xC3 :
			if (pstFSMStep->len != 3) {
				ClearData(pstFSMStep);
				return;
			}
			pstFSMStep->StdCurrent = pstFSMStep->pBuf[1];
			pstFSMStep->MaxCurrent = pstFSMStep->pBuf[2];
			break;
		case 0xC4 :
			break;
		case 0xC5 :
			if (pstFSMStep->len != 5) {
				ClearData(pstFSMStep);
				return;
			}
			pstFSMStep->SumMoney = pstFSMStep->pBuf[0] << 16 | \
								   pstFSMStep->pBuf[1] << 8 | \
								   pstFSMStep->pBuf[2];
			pstFSMStep->TotalTime = pstFSMStep->pBuf[3] << 8 | \
									pstFSMStep->pBuf[4];
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
static void FSMChargePort(FSMConditionCP_t * pstFSMStep, u08 data)
{
	switch (pstFSMStep->eStepCP) {
	case EPortCP :
		pstFSMStep->port = data;
		pstFSMStep->eStepCP = EUserTypeCP;
		break;
	case EUserTypeCP :
		pstFSMStep->UserType = data;
		pstFSMStep->eStepCP = ECmdCP;
		break;
	case ECmdCP :
		pstFSMStep->cmd = data;
		pstFSMStep->eStepCP = ELenCP;
		break;
	case ELenCP :
		pstFSMStep->len = data;
		if (data) {
			pstFSMStep->eStepCP = EDataCP;
		}
		else {
			pstFSMStep->eStepCP = EChecksumCP;
		}
		break;
	case EDataCP :
		++pstFSMStep->count;
		if (pstFSMStep->len > 1 && pstFSMStep->pBuf == NULL) {
			if (!(pstFSMStep->pBuf = (u08 *)malloc(pstFSMStep->len))) {
				perror("Not enough memory malloc in FSMChargePort");
				exit(1);
			}
		}
		if (pstFSMStep->count >= pstFSMStep->len) {
			SaveData(pstFSMStep);
			free(pstFSMStep->pBuf);
			pstFSMStep->pBuf = NULL;
			pstFSMStep->count = 0;
			pstFSMStep->eStepCP = EChecksumCP;
		}
		break;
	case EChecksumCP :
		if (pstFSMStep->checksum == data) {
			Process(pFSMStep)
				xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
		}
		else {
			clear;
			xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
		}
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
static void AnalyseAB(FSMCondition_t * pstFSMStep)
{
	int i;
	FSMConditionCP_t stFSMStep;
	stFSMStep.pBuf = NULL;
	for (i = 0; i < pstFSMStep->len; i++) {
		switch(pstFSMStep->pBuf[i]) {
		case 0xAA :
			stFSMStep.eStepCP = EPortCP;
			stFSMStep.checksum ^= 0xAA;
			continue;
			break;
		default :
		}
		FSMChargePort(&stFSMStep, pstFSMStep->pBuf[i]);
	}
	xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
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
	pstFSMStep->eCondition = EMark;
	pstFSMStep->eStep = EStart;
	pstFSMStep->count = 0;
	pstFSMStep->id = 0;
	pstFSMStep->type = 0;
	pstFSMStep->tag = 0;
	pstFSMStep->len = 0;
	if (pstFSMStep->pBuf) {
		free(pstFSMStep->pBuf);
		pstFSMStep->pBuf = NULL;
	}
	pstFSMStep->checksum = 0;
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
