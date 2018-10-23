#include "ListenDevice.h"
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
static void SaveStationMsg(u08 * pBuf, int len, \
										struct sockaddr_in * pAddr)
{
	recv_t * pRecv;
	if (!(pRecv = (recv_t *)malloc(sizeof(recv_t)))) {
			perror("Not enough memory malloc to save recv web:");
			exit(1);
	}
	pRecv->pBuf = pBuf;
	pRecv->len = len;
	pRecv->time = time(NULL);
	pRecv->addr = *pAddr;
	P(SEM_RC_ST);
	pRecv->pPrevRecv = RC_ST.pHead->pPrevRecv;
	pRecv->pNextRecv = RC_ST.pHead;
	pRecv->pPrevRecv->pNextRecv = pRecv;
	RC_ST.pHead->pPrevRecv = pRecv;
	++(RC_ST.num);
	V(SEM_RC_ST);
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
void * ListenDevice(void * arg)
{
	int ret;
	u08 * pBuf;
	struct sockaddr_in addr;
	socklen_t SocketLen;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY); 
					 // == inet_addr("0.0.0.0");
	addr.sin_port = htons(ST_PORT);
	SocketLen = sizeof(addr);
	if ((DV_HD.SvrFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Datagram socket error:");
		exit(1);
	}
	if (bind(DV_HD.SvrFd, (struct sockaddr *)&addr, \
									sizeof(struct sockaddr)) < 0) {
		perror("Bind datagram socket error:");
		exit(1);
	}
	while (1) {
		if (!(pBuf = (u08 *)malloc(BUF_MAX))) {
			perror("Not enough memory for malloc, recv device:");
			exit(1);
		}
		memset(pBuf, 0, BUF_MAX);
		memset(&addr, 0, sizeof(addr));
		ret = recvfrom(DV_HD.SvrFd, pBuf, BUF_MAX, 0, \
							(struct sockaddr *)&addr, &SocketLen);
		if (ret > 0) {
			SaveStationMsg(pBuf, ret, &addr);
			if (THD_DEVI_SLP) {
				THD_DEVI_SLP = FALSE;
				pthread_mutex_lock(&DEVI_MUTEX);
				pthread_cond_signal(&DEVI_COND);
				pthread_mutex_unlock(&DEVI_MUTEX);
			}
		}
	}
	return (void *)0;
}
/***************************scale***********************************/
