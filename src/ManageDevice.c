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
	P(SEM_RC_ST);
	pRecv = RC_ST.pHead->pstNext;
	V(SEM_RC_ST);
	printf("***************start***************\n");
	while (pRecv != RC_ST.pHead) {
		count++;
		// 临界点，ListenDevice线程会在此添加新节点
		if (RC_ST.num == count) {
			P(SEM_RC_ST);
		}
		LocalTime = localtime(&pRecv->time);
		strftime(buf, 31, "%Y-%m-%d %H:%M:%S", LocalTime);
		printf("Update:%s, len=%d, data:%s, addr=%s, ", buf, \
									pRecv->len, pRecv->pBuf, \
								inet_ntoa(pRecv->stAddr.sin_addr));
		pRecv = pRecv->pstNext;
		if (RC_ST.num == count) {
			V(SEM_RC_ST);
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
	RC_ST.num--;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void MakeHeartReply(u08 * pBuf, time_t NowTime)
{
	int i;
	for (i = 0; i < TP_HT_LN - 1; i++) {
		pBuf[i] = ((long)NowTime >> (TP_HT_LN - 2 - i) * 8) & 0xFF;
	}
	pBuf[i] = 1;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void InitStationDevice(station_t * pstStation)
{
	
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void InitStationData(station_t * pstStation, u64 id)
{
	pstStation->id = id;
	pstStation->version = 1;
	pstStation->launch = time(NULL);
	pstStation->update = pstStation->launch;
	pstStation->StdCurrent[1] = 13;
	pstStation->MaxCurrent[1] = 45;
	pstStation->NetPrice[1] = 10;
	pstStation->NetTime[1] = 10;
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
static station_t * NewStation(FSM_Condition_t * pstFSM_Step)
{
	station_t * pStation;
	if (!(pStation = (station_t *)malloc(sizeof(station_t)))) {
		perror("Not enough memory malloc for NewStation:");
		exit(1);
	}
	memset(pStation, 0, sizeof(station_t));
	pStation->pstPrev = DV_HD.pHead->pstPrev;
	pStation->pstNext = DV_HD.pHead;
	DV_HD.pHead->pstPrev->pstNext = pStation;
	DV_HD.pHead->pstPrev = pStation;
	InitStationData(pStation, pstFSM_Step->id);
	InitStationDevice(pstFSM_Step);
	return pStation;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void AnalyseBB(FSM_Condition_t * pstFSM_Step)
{
	station_t * pstStation;
	u08 buf[TP_HT_LN];
	if (!(pstStation = PosStation(pstFSM_Step->id))) {
		pstStation = NewStation(pstFSM_Step);
		InitStation(pstStation, pstFSM_Step);
		// 添加初始化发送命令
	}
	pstStation->update = time(NULL);
	pstStation->stAddr = pstFSM_Step->stAddr;
	MakeHeartReply(buf, pstStation->update - BASE_TIME);
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void ClearData(FSM_ConditionCP_t * pstFSMStepCP)
{
	if (pstFSMStepCP->pBuf) {
		free(pstFSMStepCP->pBuf);
	}
	memset(pstFSMStepCP, 0, sizeof(FSM_ConditionCP_t));
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void SaveData(FSM_ConditionCP_t * pstFSMStepCP)
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
static void FSM_ChargePort(FSM_ConditionCP_t * pstFSMStepCP, \
							u08 data, FSM_Condition_t * pstFSM_Step)
{
	switch (pstFSMStepCP->eStepCP) {
	case E_PortCP :
		pstFSMStepCP->port = data;
		pstFSMStepCP->eStepCP = E_UserTypeCP;
		break;
	case E_UserTypeCP :
		pstFSMStepCP->UserType = data;
		pstFSMStepCP->eStepCP = E_CmdCP;
		break;
	case E_CmdCP :
		pstFSMStepCP->cmd = data;
		pstFSMStepCP->eStepCP = E_LenCP;
		break;
	case E_LenCP :
		pstFSMStepCP->len = data;
		if (data) {
			pstFSMStepCP->eStepCP = E_DataCP;
		}
		else {
			pstFSMStepCP->eStepCP = E_ChecksumCP;
		}
		break;
	case E_DataCP :
		pstFSMStepCP->count++;
		if (pstFSMStepCP->len > 1 && pstFSMStepCP->pBuf == NULL) {
			if (!(pstFSMStepCP->pBuf = (u08 *)malloc\
											(pstFSMStepCP->len))) {
				perror("Not enough memory malloc in FSM_ChargePort");
				exit(1);
			}
		}
		if (pstFSMStepCP->count >= pstFSMStepCP->len) {
			SaveData(pstFSMStepCP);
			free(pstFSMStepCP->pBuf);
			pstFSMStepCP->pBuf = NULL;
			pstFSMStepCP->eStepCP = E_ChecksumCP;
		}
		break;
	case E_ChecksumCP :
		if (pstFSMStepCP->checksum == data) {
			station_t * pstStation;
			if (!(pstStation = PosStation(pstFSM_Step->id))) {
				pstStation = NewStation(pstFSM_Step->id);
				InitStation(pstStation, pstFSM_Step);
				// 添加发送初始化命令
			}
			pstStation->stAddr = pstFSM_Step->stAddr;
			ProcessAB(pstStation, pstFSMStepCP);
		}
		ClearData(pstFSMStepCP);
		return;
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
static void AnalyseAB(FSM_Condition_t * pstFSM_Step)
{
	int i;
	FSM_ConditionCP_t stFSMStepCP;
	stFSMStepCP.pBuf = NULL;
	stFSMStepCP.checksum = 0;
	for (i = 0; i < pstFSM_Step->len; i++) {
		switch(pstFSM_Step->pBuf[i]) {
		case 0xAA :
			stFSMStepCP.eStepCP = E_PortCP;
			stFSMStepCP.checksum ^= 0xAA;
			continue;
			break;
		default :
		}
		FSM_ChargePort(&stFSMStepCP, pstFSM_Step->pBuf[i], pstFSM_Step);
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void ProcessDevCmd(FSM_Condition_t * pFSMStep)
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
static void ClearFSMTrnsProt(FSM_Condition_t * pstFSM_Step)
{
	if (pstFSM_Step->pBuf) {
		free(pstFSM_Step->pBuf);
	}
	memset(pstFSM_Step, 0, sizeof(FSM_Condition_t));
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void FSMTrnsPort(FSM_Condition_t * pstFSM_Step, u08 data)
{
	switch(pstFSM_Step->eCondition) {
	case E_mark :
		ClearFSMTrnsProt(pstFSM_Step);
		if (data == TP_MARK) {
			pstFSM_Step->eStep = E_id;
			pstFSM_Step->id = 1;
			pstFSM_Step->checksum ^= data;
			return;
		}
		else {
			return;
		}
		break;
	case E_xor :
		data ^= TP_FT_NM;
		break;
	case E_end :
		return;
	default :
	}
	switch(pstFSM_Step->eStep) {
	case E_id :
		pstFSM_Step->count++;
		if (pstFSM_Step->count >= TP_ID_LN) {
			pstFSM_Step->count = 0;
			pstFSM_Step->eStep = E_tag;
		}
		if (data >= 0x80) {
			pstFSM_Step->type |= 1 << (TP_ID_LN - pstFSM_Step->count);
			data &= 0x7F;
		}
		pstFSM_Step->id = pstFSM_Step->id * 100 + data;
		break;
	case E_tag :
		pstFSM_Step->count++;
		if (pstFSM_Step->count >= TP_TG_LN) {
			pstFSM_Step->count = 0;
			pstFSM_Step->eStep = E_len;
		}
		pstFSM_Step->tag |= data << ((TP_TG_LN - pstFSM_Step->count) * 8);
		break;
	case E_len :
		pstFSM_Step->count++;
		if (pstFSM_Step->count >= TP_LN_LN) {
			pstFSM_Step->count = 0;
			pstFSM_Step->eStep = E_data;
		}
		pstFSM_Step->len |= data << ((TP_LN_LN - pstFSM_Step->count) * 8);
		break;
	case E_data :
		if (pstFSM_Step->pBuf == NULL) {
			pstFSM_Step->len -= 2;
			if (!(pstFSM_Step->pBuf = (u08 *)malloc \
												(pstFSM_Step->len))) {
				perror("Not enough memory malloc for \
												pstFSM_Step->pBuf:");
				exit(1);
			}
		}
		pstFSM_Step->pBuf[pstFSM_Step->count] = data;
		pstFSM_Step->count++;
		if (pstFSM_Step->count >= pstFSM_Step->len) {
			pstFSM_Step->count = 0;
			pstFSM_Step->eStep = E_checksum;
		}
		break;
	case E_checksum :
		if (pstFSM_Step->checksum == data) {
			ProcessDevCmd(pstFSM_Step);
		}
		pstFSM_Step->eStep = E_start;
		break;
	default :
		return;
	}
	pstFSM_Step->checksum ^= data;
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
	static FSM_Condition_t stFSMStep;
	for (i = 0; i < pstRecv->len; i++) {
		switch(pstRecv->pBuf[i]) {
		case TP_HEAD :
			stFSMStep.eCondition = E_mark;
			stFSMStep.stAddr = pstRecv->stAddr;
			continue;
			break;
		case TP_FILT :
			stFSMStep.eCondition = E_xor;
			continue;
			break;
		case TP_TAIL :
			stFSMStep.eCondition = E_end;
			ClearFSMTrnsProt(&stFSMStep);
			continue;
			break;
		default :
			stFSMStep.eCondition = E_content;
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
	P(SEM_RC_ST);
	pstRecv = RC_ST.pstHead->pstNext;
	V(SEM_RC_ST);
	while (pstRecv != RC_ST.pstHead) {
		pstTmp = pstRecv->pstNext;
		// 临界点，ListenServer线程会在此添加新节点
		if (pstTmp == RC_ST.pstHead) {
			P(SEM_RC_ST);
		}
		AnalyseTrnsProt(pstRecv);

		DeleteThis(pstRecv);
		pstRecv = pstTmp;
		if (pstTmp == RC_ST.pstHead) {
			V(SEM_RC_ST);
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
		pthread_mutex_lock(&DEVI_MUTEX);
		if (THD_DEVI_SLP) {
			pthread_cond_wait(&DEVI_COND, &DEVI_MUTEX);
		}
		ProcessDevMsg();
		THD_DEVI_SLP = TRUE;
		pthread_mutex_unlock(&DEVI_MUTEX);
	}
	return (void *)0;
}
/***************************scale***********************************/
