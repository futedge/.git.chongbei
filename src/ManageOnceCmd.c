#include "ManageOnceCmd.h"

void SendCmd()
{
	SendCmd_t * pstCmd, * pstTmp;
	P(SEMOCCMD);
	pstCmd = OCHD->pstNext;
	V(SEMOCCMD);
	while (pstCmd != OCHD) {
		pstTmp = pstCmd->pstNext;
		sendto(DVHD.SvrFd, OCHD->pstStation->pCmd->pBuf, \
						 OCHD->pstStation->pCmd->len, 0, \
			 (struct sockaddr *)&OCHD->pstStation->addr, \
								  sizeof(struct sockaddr));
		if (pstTmp == OCHD) {
			P(SEMOCCMD);
		}
		pstCmd->pstPrev->pstNext = pstCmd->pstNext;
		pstCmd->pstNext->pstPrev = pstCmd->pstPrev;
		P(SEMTRCMD);
		pstCmd->pstPrev = TRHD->pstPrev;
		pstCmd->pstNext = TRHD;
		TRHD->pstPrev->pstNext = pstCmd;
		TRHD->pstPrev = pstCmd;
		V(SEMTRCMD);
		pstCmd = pstTmp;
		if (pstTmp == OCHD) {
			V(SEMOCCMD);
		}
	}
}

void * ManageOnceCmd(void * arg)
{
	while(1) {
		pthread_mutex_lock(&ONCEMUTEX);
		if (THDONCESLP) {
			pthread_cond_wait(&ONCECOND, &ONCEMUTEX);
		}
		SendCmd();
		THDONCESLP = TRUE;
		pthread_mutex_unlock(&ONCEMUTEX);
	}
	return (void *)0;
}
/***************************scale***********************************/
