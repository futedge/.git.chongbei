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
	P(SEM_RC_SR);
	pRecv = RC_SR.pHead->pstNext;
	V(SEM_RC_SR);
	printf("***************start***************\n");
	while (pRecv != RC_SR.pHead) {
		pstTmp = pRecv->pstNext;
		// 临界点，ListenDevice线程会在此添加新节点
		if (pstTmp == RC_SR.pHead) {
			P(SEM_RC_SR);
		}
		LocalTime = localtime(&pRecv->time);
		strftime(buf, 31, "%Y-%m-%d %H:%M:%S", LocalTime);
		printf("Update:%s, len=%d, data:%s, fd=%d, ", buf, \
								pRecv->len, pRecv->pBuf, pRecv->fd);
		LocalTime = localtime(&pRecv->launch);
		strftime(buf, 31, "%Y-%m-%d %H:%M:%S", LocalTime);
		printf("%s\n", buf);
		pRecv = pRecv->pstNext;
		if (pstTmp == RC_SR.pHead) {
			V(SEM_RC_SR);
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
	RC_SR.num--;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void reply(void)
{
	recv_t * pRecv;
	P(SEM_RC_SR);
	pRecv = RC_SR.pHead->pstNext;
	V(SEM_RC_SR);
	while (pRecv != RC_SR.pHead) {
		// 临界点，ListenDevice线程会在此添加新节点
		if (pRecv->pstNext == RC_SR.pHead) {
			P(SEM_RC_SR);
		}
		write(pRecv->fd, pRecv->pBuf, pRecv->len);
		free(pRecv->pBuf);
		DeleteThis(&pRecv);
		pRecv = pRecv->pstNext;
		if (pRecv == RC_SR.pHead ) {
			flag = FALSE;
			V(SEM_RC_SR);
		}
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/										
static bool DataLenCheck(FSM_Condition_t * pstFSM_Step, \
													u08 VaildLen)
{
	if (pstFSM_Step->count > VaildLen) {
		return FALSE;
	}
	return TRUE;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void ExtOrdNo(FSM_Condition_t * pstFSM_Step, u08 data, \
									u08 VaildLen, char ** pTarget)
{
	if (DataLenCheck(++pstFSM_Step->count, VaildLen)) {
		if (! * pTarget) {
			if (! (* pTarget = (char *)malloc(VaildLen + 1))) {
				perror("Not enough memory malloc for \
											pstFSM_Step->pBuf:");
				exit(1);
			}
			memset(* pTarget, '\0', VaildLen + 1);
		}
		(* pTarget)[pstFSM_Step->count - 1] = data;
	}
	else {
		pstFSM_Step->bAnalyseStatus = FALSE;
		pstFSM_Step->emStep = eErr;
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
static bool TrnfmXData(FSM_Condition_t * pstFSM_Step, u08 data, \
										u08 VaildLen, u08 * pTarget)
{
	if (DataLenCheck(++pstFSM_Step->count, VaildLen)) {
		if (!ChaToXDig(data, pTarget)) {
			pstFSM_Step->emStep = eErr;
			return FALSE;
		}
	}
	else {
		pstFSM_Step->emStep = eErr;
		return FALSE;
	}
	return TRUE;
}

/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static bool TrnfmData(FSM_Condition_t * pstFSM_Step, u08 data, \
										u08 VaildLen, u64 * pTarget)
{
	if (DataLenCheck(++pstFSM_Step->count, VaildLen)) {
		if (!ChaToDig(data, pTarget)) {
			pstFSM_Step->emStep = eErr;
			return FALSE;
		}
	}
	else {
		pstFSM_Step->emStep = eErr;
		return FALSE;
	}
	return TRUE;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void FSM(FSM_Condition_t * pstFSM_Step, u08 data)
{
	switch(pstFSM_Step->emStep) {
	case E_id :
		if (TrnfmData(pstFSM_Step, data, ID_LEN, \
											& pstFSM_Step->id)) {
			pstFSM_Step->bit.ValidId = 1;
		}
		else {
			pstFSM_Step->bit.ValidId = 0;
		}
		break;
	case E_cmd :
		if (TrnfmXData(pstFSM_Step, data, CMD_LEN, \
											& pstFSM_Step->cmd)) {
			pstFSM_Step->bit.ValidCmd = 1;
		}
		else {
			pstFSM_Step->bit.ValidCmd = 0;
		}
		break;
	case E_port :
		if (TrnfmData(pstFSM_Step, data, PORT_LEN, \
											& pstFSM_Step->port)) {
			pstFSM_Step->bit.ValidPort = 1;
		}
		else {
			pstFSM_Step->bit.ValidPort = 0;
		}
		break;
	case E_data1 :
		if (TrnfmData(pstFSM_Step, data, DATA_LEN, \
											& pstFSM_Step->data1)) {
			pstFSM_Step->bit.ValidData1 = 1;
		}
		else {
			pstFSM_Step->bit.ValidData1 = 0;
		}
		break;
	case E_data2 :
		if (TrnfmData(pstFSM_Step, data, DATA_LEN, \
											& pstFSM_Step->data2)) {
			pstFSM_Step->bit.ValidData2 = 1;
		}
		else {
			pstFSM_Step->bit.ValidData2 = 0;
		}
		break;
	case E_OrdNo :
		if (ExtOrdNo(pstFSM_Step, data, ORD_NO_LEN, \
											& pstFSM_Step->pBuf)) {
			pstFSM_Step->bit.ValidOrNo = 1;
		}
		else {
			pstFSM_Step->bit.ValidOrNo = 0;
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
static u08 * MakeCmdV1(FSM_Condition_t * pstFSM_Step)
{
	u08 Buf[BUF_CMD];
	u08 n = 0;
	if (pstFSM_Step->port < 1 && pstFSM_Step->port > 10) {
		pstFSM_Step->port = 1;
	}
	Buf[++n] = 0xAA;
	Buf[++n] = pstFSM_Step->port;
	Buf[++n] = 0;
	Buf[++n] = pstFSM_Step->cmd;
	switch (pstFSM_Step->cmd) {
	case 0xA0 :
		Buf[++n] = 5;
		Buf[++n] = pstFSM_Step->
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
static void SaveWebCmd(recv_t * pRecv, FSM_Condition_t * pstFSM_Step)
{
	station_t * pstStation;
	if (pstStation = PosStation(pstFSM_Step->id)) {
		switch (pstStation->version) {
		case 1 :
			MakeCmdV1(pstFSM_Step);
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
		
		P(SEM_ON_CM);
		pstCmd->pstPrev = ON_CM_HD->pstPrev;
		pstCmd->pstNext = ON_CM_HD;
		ON_CM_HD->pstPrev->pstNext = pstCmd;
		ON_CM_HD->pstPrev = pstCmd;
		V(SEM_ON_CM);
		if (THD_ONCE_SLP) {
			THD_ONCE_SLP = FALSE;
			pthread_mutex_lock(&ONCE_MUTEX);
			pthread_cond_signal(&ONCE_COND);
			pthread_mutex_unlock(&ONCE_MUTEX);
		}
	}
	else {
		ReplyID(pRecv, pstFSM_Step->id)
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static bool CmdValidCheck(FSM_Condition_t * pstFSM_Step)
{
	switch(pstFSM_Step->cmd) {
	case 0xA0 :
		if (pstFSM_Step->word == E_type3) return TRUE;
		break;
	case 0xA4 :
	case 0xA5 :
		if (pstFSM_Step->word == E_type2) return TRUE;
		break;
	default :
		if (pstFSM_Step->word == E_type1) return TRUE;
	}
	return FALSE;
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void NextStepCheck(FSM_Condition_t * pstFSM_Step)
{
	if (pstFSM_Step->emStep == ePort) {
		switch (pstFSM_Step->E_cmd) {
		case 0xA4 :
		case 0xA5 :
			pstFSM_Step->step += 1;
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
static void ClearFSM(FSM_Condition_t * pstFSM_Step)
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
static void analyse(recv_t * pRecv)
{
	int i;
	static FSM_Condition_t stFSMStep;
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
	P(SEM_RC_SR);
	pstRecv = RC_SR.pstHead->pstNext;
	V(SEM_RC_SR);
	while (pstRecv != RC_SR.pstHead) {
		pstTmp = pstRecv->pstNext;
		// 临界点，ListenServer线程会在此添加新节点
		if (pstTmp == RC_SR.pstHead) {
			P(SEM_RC_SR);
		}
		analyse(pstRecv);
		DeleteThis(pstRecv);
		pstRecv = pstTmp;
		if (pstTmp == RC_SR.pstHead) {
			V(SEM_RC_SR);
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
		pthread_mutex_lock(&COMM_MUTEX);
		if (THD_COMM_SLP) {
			pthread_cond_wait(&COMM_COND, &COMM_MUTEX);
		}
		ProcessSvrMsg();
		PrintSvrMsg();
		THD_COMM_SLP = TRUE;
		pthread_mutex_unlock(&COMM_MUTEX);
	}
	return (void *)0;
}
/***************************scale***********************************/
