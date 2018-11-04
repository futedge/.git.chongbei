#include "ManageOnceCmd.h"
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
void SendCmd(void)
{
	SendCmd_t * pstCmd, * pstTmp;
	P(SEM_ON_CM);
	pstCmd = ON_CM_HD->pstNext;
	V(SEM_ON_CM);
	while (pstCmd != ON_CM_HD) {
		pstTmp = pstCmd->pstNext;
		sendto(DV_HD.SvrFd, ON_CM_HD->pstStation->pCmd->pBuf, \
						 ON_CM_HD->pstStation->pCmd->len, 0, \
			 (struct sockaddr *)&ON_CM_HD->pstStation->addr, \
								  sizeof(struct sockaddr));
		if (pstTmp == ON_CM_HD) {
			P(SEM_ON_CM);
		}
		pstCmd->pstPrev->pstNext = pstCmd->pstNext;
		pstCmd->pstNext->pstPrev = pstCmd->pstPrev;
		P(SEM_TR_CM);
		pstCmd->pstPrev = TR_CM_HD->pstPrev;
		pstCmd->pstNext = TR_CM_HD;
		TR_CM_HD->pstPrev->pstNext = pstCmd;
		TR_CM_HD->pstPrev = pstCmd;
		V(SEM_TR_CM);
		pstCmd = pstTmp;
		if (pstTmp == ON_CM_HD) {
			V(SEM_ON_CM);
		}
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
void * ManageOnceCmd(void * arg)
{
	while(1) {
		pthread_mutex_lock(&ONCE_MUTEX);
		if (THD_ONCE_SLP) {
			pthread_cond_wait(&ONCE_COND, &ONCE_MUTEX);
		}
		SendCmd();
		THD_ONCE_SLP = TRUE;
		pthread_mutex_unlock(&ONCE_MUTEX);
	}
	return (void *)0;
}
/***************************scale***********************************/
