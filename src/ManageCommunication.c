#include "ManageCommunication.h"
#include "ManageCommunicationPrivate.h"

static void DeleteThis(recv_t ** ppRecv)
{
	recv_t * pRecvTmp;
	(*ppRecv)->pPrevRecv->pstNext = (*ppRecv)->pstNext;
	(*ppRecv)->pstNext->pPrevRecv = (*ppRecv)->pPrevRecv;
	pRecvTmp = (*ppRecv);
	(*ppRecv) = (*ppRecv)->pPrevRecv;
	free(pRecvTmp->pBuf);
	free(pRecvTmp);
	--(RCSR.num);
}

static void reply(void)
{
	recv_t * pRecv;
	int count = 0;
	bool flag;
	P(SEMRCSR);
	pRecv = RCSR.pHead->pstNext;
	V(SEMRCSR);
	while (pRecv != RCSR.pHead) {
		++count;
		// 临界点，ListenDevice线程会在此添加新节点
		if (RCSR.num == count) {
			flag = TRUE;
		}
		if (flag) {
			P(SEMRCSR);
		}
		write(pRecv->fd, pRecv->pBuf, pRecv->len);
		free(pRecv->pBuf);
		DeleteThat(&pRecv);
		pRecv = pRecv->pstNext;
		if (flag) {
			flag = FALSE;
			V(SEMRCSR);
		}
	}
}

static void ReplyErrFmt(recv_t * pRecv, \
										FSMCondition_t * pstFSMStep)
{
	if (! pstFSMStep->bAnalyseStatus) {
		char * pBuf = (char *)malloc(pRecv->len + 17);
		strcpy(pBuf, "type=EE&content=");
		strcat(pBuf, pRecv->pBuf);
		write(pRecv->fd, pBuf, strlen(pBuf));
	}
}
										
static bool DataLenCheck(FSMCondition_t * pstFSMStep, \
													u08 VaildLen)
{
	if (pstFSMStep->count > VaildLen) {
		return FALSE;
	}
	return TRUE;
}

static void ExtOrdNo(FSMCondition_t * pstFSMStep, u08 data, \
									u08 VaildLen, char ** pTarget)
{
	if (DataLenCheck(++pstFSMStep->count, VaildLen)) {
		if (! * pTarget) {
			if (! (* pTarget = (char *)malloc(VaildLen + 1))) {
				perror("Not enough memory malloc for \
											pstFSMStep->pBuf:");
				exit(1);
			}
			memset(* pTarget, '\0', VaildLen + 1);
		}
		* pTarget[pstFSMStep->count - 1] = data;
	}
	else {
		pstFSMStep->bAnalyseStatus = FALSE;
		pstFSMStep->emStep = eErr;
	}
}

static bool ChaToXDig(u08 data, u08 * pTarget)
{
	if (data >= '0' && data <= '9') {
		*pTarget = *pTarget * 16 + data - '0';
		return TRUE;
	} else if (data >= 'A' && data <= 'F') {
		*pTarget = *pTarget * 16 + data - 'A' + 10;
		return TRUE;
	} else if (data >= 'a' && data <= 'f') {
		*pTarget = *pTarget * 16 + data - 'a' + 10;
		return TRUE;
	}
	return FALSE;
} 

static bool ChaToDig(u08 data, u64 * pTarget)
{
	if (data >= '0' && data <= '9') {
		*pTarget = *pTarget * 10 + data - '0';
		return TRUE;
	}
	return FALSE;
}

static void TrnfmXData(FSMCondition_t * pstFSMStep, u08 data, \
										u08 VaildLen, u08 * pTarget)
{
	if (DataLenCheck(++pstFSMStep->count, VaildLen)) {
		if (!ChaToXDig(data, pTarget)) {
			pstFSMStep->bAnalyseStatus = FALSE;
			pstFSMStep->emStep = eErr;
		}
	}
	else {
		pstFSMStep->bAnalyseStatus = FALSE;
		pstFSMStep->emStep = eErr;
	}
}

static void TrnfmData(FSMCondition_t * pstFSMStep, u08 data, \
										u08 VaildLen, u64 * pTarget)
{
	if (DataLenCheck(++pstFSMStep->count, VaildLen)) {
		if (!ChaToDig(data, pTarget)) {
			pstFSMStep->bAnalyseStatus = FALSE;
			pstFSMStep->emStep = eErr;
		}
	}
	else {
		pstFSMStep->bAnalyseStatus = FALSE;
		pstFSMStep->emStep = eErr;
	}
}

static void FSM(FSMCondition_t * pstFSMStep, u08 data)
{
	switch(pstFSMStep->emStep) {
	case eId :
		TrnfmData(pstFSMStep, data, IDLEN, & pstFSMStep->id);
		break;
	case eCmd :
		TrnfmXData(pstFSMStep, data, CMDLEN, & pstFSMStep->cmd);
		break;
	case ePort :
		TrnfmData(pstFSMStep, data, PORTLEN, & pstFSMStep->port);
		break;
	case eData1 :
		TrnfmData(pstFSMStep, data, DATALEN, & pstFSMStep->data1);
		break;
	case eData2 :
		TrnfmData(pstFSMStep, data, DATALEN, & pstFSMStep->data2);
		break;
	case eOrdNo :
		ExtOrdNo(pstFSMStep, data, ORDNOLEN, & pstFSMStep->pBuf);
		break;
	default :
	}
}

static void SaveWebCmd(FSMCondition_t * pstFSMStep)
{
	if (pstFSMStep->bAnalyseStatus) {
		SendCmd_t pstCmd = NULL;
		if (!(pstCmd = (SendCmd_t *)mailloc(sizeof(SendCmd_t)))) {
			perror("Not enough memory malloc for SendCmdTmp:");
			exit(1);
		}
		memset(pstCmd, 0, sizeof(SendCmd_t));
		P(SEMOCCMD);
		pstCmd.pstPrev = TRHD->pstPrev;
		pstCmd.pstNext = TRHD;
		TRHD->pstPrev->pstNext = pstCmd;
		TRHD->pstPrev = pstCmd;
		V(SEMOCCMD);
		if (THDONCESLP) {
			THDONCESLP = FALSE;
			pthread_mutex_lock(&ONCEMUTEX);
			pthread_cond_signal(&ONCECOND);
			pthread_mutex_unlock(&ONCEMUTEX);
		}
	}
}

static void NextStepCheck(FSMCondition_t * pstFSMStep)
{
	if (pstFSMStep->emStep == ePort) {
		switch (pstFSMStep->eCmd) {
		case 0xA4 :
		case 0xA5 :
			pstFSMStep->step += 1;
			break;
		default :
		}
	}
	if (FSMStep.step > eErr) FSMStep.step = eErr;
}

static void InitFSM(FSMCondition_t * pstFSMStep)
{
	if (pstFSMStep->pBuf) {
		free(pstFSMStep->pBuf);
	}
	memset(pstFSMStep, 0, sizeof(FSMCondition_t));
	pstFSMStep->bAnalyseStatus = TRUE;
}

static void analyse(recv_t * pRecv)
{
	int i;
	static FSMCondition_t stFSMStep;
	InitFSM(&stFSMStep);
	for (i = 0; i < pRecv->len; i++) {
		switch(pRecv->pBuf[i]) {
		case '+' :
			stFSMStep.count = 0;
			stFSMStep.step += 1;
			NextStepCheck(&stFSMStep);
			break;
		case '/' :
			SaveWebCmd(&stFSMStep);
			InitFSM(&stFSMStep);
			break;
		default :
			FSM(&stFSMStep, pRecv->pBuf[i]);
		}
	}
	ReplyErrFmt(pRecv);
}

static void ProcessSvrMsg(void)
{
	recv_t * pRecv, * pstTmp;
	P(SEMRCSR);
	pRecv = RCSR.pHead->pstNext;
	V(SEMRCSR);
	while (pRecv != RCSR.pHead) {
		pstTmp = pRecv->pstNext;
		// 临界点，ListenServer线程会在此添加新节点
		if (pstTmp == RCSR.pHead) {
			P(SEMRCSR);
		}
		analyse(pRecv);
		DeleteThis(&pRecv);
		pRecv = pstTmp;
		if (pstTmp == RCSR.pHead) {
			V(SEMRCSR);
		}
	}
}

static void PrintSvrMsg(void)
{
	recv_t * pRecv, * pstTmp;
	char buf[32];
	struct tm * LocalTime;
	P(SEMRCSR);
	pRecv = RCSR.pHead->pstNext;
	V(SEMRCSR);
	printf("***************start***************\n");
	while (pRecv != RCSR.pHead) {
		pstTmp = pRecv->pstNext;
		// 临界点，ListenDevice线程会在此添加新节点
		if (pstTmp == RCSR.pHead) {
			P(SEMRCSR);
		}
		LocalTime = localtime(&pRecv->time);
		strftime(buf, 31, "%Y-%m-%d %H:%M:%S", LocalTime);
		printf("Update:%s, len=%d, data:%s, fd=%d, ", buf, \
								pRecv->len, pRecv->pBuf, pRecv->fd);
		LocalTime = localtime(&pRecv->launch);
		strftime(buf, 31, "%Y-%m-%d %H:%M:%S", LocalTime);
		printf("%s\n", buf);
		pRecv = pRecv->pstNext;
		if (pstTmp == RCSR.pHead) {
			V(SEMRCSR);
		}
	}
	printf("****************end****************\n\n\n");
}

void * ManageCommunication(void * arg)
{
	while(1) {
		pthread_mutex_lock(&COMMMUTEX);
		if (THDCOMMSLP) {
			pthread_cond_wait(&COMMCOND, &COMMMUTEX);
		}
		PrintSvrMsg();
		ProcessSvrMsg();
		PrintSvrMsg();
		THDCOMMSLP = TRUE;
		pthread_mutex_unlock(&COMMMUTEX);
	}
	return (void *)0;
}
/***************************scale***********************************/
