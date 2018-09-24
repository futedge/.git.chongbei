#include "ManageServerPrivate.h"
#include "ManageServer.h"
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
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
	RCSR.num--;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void reply(void)
{
	recv_t * pRecv;
	P(SEMRCSR);
	pRecv = RCSR.pHead->pstNext;
	V(SEMRCSR);
	while (pRecv != RCSR.pHead) {
		// 临界点，ListenDevice线程会在此添加新节点
		if (pRecv->pstNext == RCSR.pHead) {
			P(SEMRCSR);
		}
		write(pRecv->fd, pRecv->pBuf, pRecv->len);
		free(pRecv->pBuf);
		DeleteThis(&pRecv);
		pRecv = pRecv->pstNext;
		if (pRecv == RCSR.pHead ) {
			flag = FALSE;
			V(SEMRCSR);
		}
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/										
static bool DataLenCheck(FSMCondition_t * pstFSMStep, \
													u08 VaildLen)
{
	if (pstFSMStep->count > VaildLen) {
		return FALSE;
	}
	return TRUE;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
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
		(* pTarget)[pstFSMStep->count - 1] = data;
	}
	else {
		pstFSMStep->bAnalyseStatus = FALSE;
		pstFSMStep->emStep = eErr;
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
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
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static bool ChaToDig(u08 data, u64 * pTarget)
{
	if (data >= '0' && data <= '9') {
		*pTarget = *pTarget * 10 + data - '0';
		return TRUE;
	}
	return FALSE;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static bool TrnfmXData(FSMCondition_t * pstFSMStep, u08 data, \
										u08 VaildLen, u08 * pTarget)
{
	if (DataLenCheck(++pstFSMStep->count, VaildLen)) {
		if (!ChaToXDig(data, pTarget)) {
			pstFSMStep->emStep = eErr;
			return FALSE;
		}
	}
	else {
		pstFSMStep->emStep = eErr;
		return FALSE;
	}
	return TRUE;
}

/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static bool TrnfmData(FSMCondition_t * pstFSMStep, u08 data, \
										u08 VaildLen, u64 * pTarget)
{
	if (DataLenCheck(++pstFSMStep->count, VaildLen)) {
		if (!ChaToDig(data, pTarget)) {
			pstFSMStep->emStep = eErr;
			return FALSE;
		}
	}
	else {
		pstFSMStep->emStep = eErr;
		return FALSE;
	}
	return TRUE;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void FSM(FSMCondition_t * pstFSMStep, u08 data)
{
	switch(pstFSMStep->emStep) {
	case EId :
		if (TrnfmData(pstFSMStep, data, IDLEN, \
											& pstFSMStep->id)) {
			pstFSMStep->bit.ValidId = 1;
		}
		else {
			pstFSMStep->bit.ValidId = 0;
		}
		break;
	case ECmd :
		if (TrnfmXData(pstFSMStep, data, CMDLEN, \
											& pstFSMStep->cmd)) {
			pstFSMStep->bit.ValidCmd = 1;
		}
		else {
			pstFSMStep->bit.ValidCmd = 0;
		}
		break;
	case EPort :
		if (TrnfmData(pstFSMStep, data, PORTLEN, \
											& pstFSMStep->port)) {
			pstFSMStep->bit.ValidPort = 1;
		}
		else {
			pstFSMStep->bit.ValidPort = 0;
		}
		break;
	case EData1 :
		if (TrnfmData(pstFSMStep, data, DATALEN, \
											& pstFSMStep->data1)) {
			pstFSMStep->bit.ValidData1 = 1;
		}
		else {
			pstFSMStep->bit.ValidData1 = 0;
		}
		break;
	case EData2 :
		if (TrnfmData(pstFSMStep, data, DATALEN, \
											& pstFSMStep->data2)) {
			pstFSMStep->bit.ValidData2 = 1;
		}
		else {
			pstFSMStep->bit.ValidData2 = 0;
		}
		break;
	case EOrdNo :
		if (ExtOrdNo(pstFSMStep, data, ORDNOLEN, \
											& pstFSMStep->pBuf)) {
			pstFSMStep->bit.ValidOrNo = 1;
		}
		else {
			pstFSMStep->bit.ValidOrNo = 0;
		}
		break;
	default :
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
void ReplyID(recv_t * pRecv, u64 id)
{
	char BufTmp[12], BufSend[28];
	memset(BufTmp, 0, 12);
	memset(BufSend, 0, 28);
	strcpy(BufSend, "type=ID&content=");
	sprintf(BufTmp, "%ld", id);
	strcat(BufSend, BufTmp);
	write(pRecv->pstSvr->fd, BufSend, strlen(BufSend));
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
void ReplyEE(recv_t * pRecv, u08 len)
{
	char * pBuf = (char *)malloc(len + 17);
	memset(pBuf, 0, len + 17);
	strcpy(pBuf, "type=EE&content=");
	strncat(pBuf, pRecv->pBuf, len);
	write(pRecv->pstSvr->fd, pBuf, SendLen);
	free(pBuf);
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static u08 * MakeCmdV1(FSMCondition_t * pstFSMStep)
{
	u08 Buf[BUFCMD];
	u08 n = 0;
	if (pstFSMStep->port < 1 && pstFSMStep->port > 10) {
		pstFSMStep->port = 1;
	}
	Buf[++n] = 0xAA;
	Buf[++n] = pstFSMStep->port;
	Buf[++n] = 0;
	Buf[++n] = pstFSMStep->cmd;
	switch (pstFSMStep->cmd) {
	case 0xA0 :
		Buf[++n] = 5;
		Buf[++n] = pstFSMStep->
		break;
	case 0xA4 :
		break;
	case 0xA5 :
		break;
	case 0xC0 :
	case 0xC1 :
	case 0xC2 :
	case 0xC3 :
	case 0xF7 :
		break;
	}		
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void SaveWebCmd(recv_t * pRecv, FSMCondition_t * pstFSMStep)
{
	station_t * pstStation;
	if (pstStation = PosStation(pstFSMStep->id)) {
		switch (pstStation->version) {
		case 1 :
			MakeCmdV1(pstFSMStep);
			break;
		case 2 :
			break;
		}
		SendCmd_t * pstCmd = NULL;
		if (!(pstCmd = (SendCmd_t *)mailloc(sizeof(SendCmd_t)))) {
			perror("Not enough memory malloc for SendCmdTmp:");
			exit(1);
		}
		memset(pstCmd, 0, sizeof(SendCmd_t));
		
		P(SEMOCCMD);
		pstCmd->pstPrev = OCHD->pstPrev;
		pstCmd->pstNext = OCHD;
		OCHD->pstPrev->pstNext = pstCmd;
		OCHD->pstPrev = pstCmd;
		V(SEMOCCMD);
		if (THDONCESLP) {
			THDONCESLP = FALSE;
			pthread_mutex_lock(&ONCEMUTEX);
			pthread_cond_signal(&ONCECOND);
			pthread_mutex_unlock(&ONCEMUTEX);
		}
	}
	else {
		ReplyID(pRecv, pstFSMStep->id)
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static bool CmdValidCheck(FSMCondition_t * pstFSMStep)
{
	switch(pstFSMStep->cmd) {
	case 0xA0 :
		if (pstFSMStep->word == EType3) return TRUE;
		break;
	case 0xA4 :
	case 0xA5 :
		if (pstFSMStep->word == EType2) return TRUE;
		break;
	default :
		if (pstFSMStep->word == EType1) return TRUE;
	}
	return FALSE;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void NextStepCheck(FSMCondition_t * pstFSMStep)
{
	if (pstFSMStep->emStep == ePort) {
		switch (pstFSMStep->ECmd) {
		case 0xA4 :
		case 0xA5 :
			pstFSMStep->step += 1;
			break;
		default :
		}
	}
	if (FSMStep.step > eErr) FSMStep.step = eErr;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void ClearFSM(FSMCondition_t * pstFSMStep)
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
static void analyse(recv_t * pRecv)
{
	int i;
	static FSMCondition_t stFSMStep;
	for (i = 0; i < pRecv->len; i++) {
		switch(pRecv->pBuf[i]) {
		case '+' :
			stFSMStep.count = 0;
			stFSMStep.step += 1;
			NextStepCheck(&stFSMStep);
			break;
		case '/' :
			if (CmdValidCheck(&stFSMStep)) {
				SaveWebCmd(pRecv, &stFSMStep);
			}
			else {
				ReplyEE(pRecv, i + 1);
			}
			ClearFSM(&stFSMStep);
			break;
		default :
			FSM(&stFSMStep, pRecv->pBuf[i]);
		}
	}
	if (stFSMStep.id) {
		ReplyEE(pRecv, pRecv->len);
		ClearFSM(&stFSMStep);
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void ProcessSvrMsg(void)
{
	recv_t * pstRecv, * pstTmp;
	P(SEMRCSR);
	pstRecv = RCSR.pstHead->pstNext;
	V(SEMRCSR);
	while (pstRecv != RCSR.pstHead) {
		pstTmp = pstRecv->pstNext;
		// 临界点，ListenServer线程会在此添加新节点
		if (pstTmp == RCSR.pstHead) {
			P(SEMRCSR);
		}
		analyse(pstRecv);
		DeleteThis(pstRecv);
		pstRecv = pstTmp;
		if (pstTmp == RCSR.pstHead) {
			V(SEMRCSR);
		}
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
void * ManageServer(void * arg)
{
	while(1) {
		pthread_mutex_lock(&COMMMUTEX);
		if (THDCOMMSLP) {
			pthread_cond_wait(&COMMCOND, &COMMMUTEX);
		}
		ProcessSvrMsg();
		PrintSvrMsg();
		THDCOMMSLP = TRUE;
		pthread_mutex_unlock(&COMMMUTEX);
	}
	return (void *)0;
}
/***************************scale***********************************/
