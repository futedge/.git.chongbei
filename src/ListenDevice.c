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
	P(SEMRCST);
	pRecv->pPrevRecv = RCST.pHead->pPrevRecv;
	pRecv->pNextRecv = RCST.pHead;
	pRecv->pPrevRecv->pNextRecv = pRecv;
	RCST.pHead->pPrevRecv = pRecv;
	++(RCST.num);
	V(SEMRCST);
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
	addr.sin_port = htons(STPORT);
	SocketLen = sizeof(addr);
	if ((DVHD.SvrFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Datagram socket error:");
		exit(1);
	}
	if (bind(DVHD.SvrFd, (struct sockaddr *)&addr, \
									sizeof(struct sockaddr)) < 0) {
		perror("Bind datagram socket error:");
		exit(1);
	}
	while (1) {
		if (!(pBuf = (u08 *)malloc(BUFMAX))) {
			perror("Not enough memory for malloc, recv device:");
			exit(1);
		}
		memset(pBuf, 0, BUFMAX);
		memset(&addr, 0, sizeof(addr));
		ret = recvfrom(DVHD.SvrFd, pBuf, BUFMAX, 0, \
							(struct sockaddr *)&addr, &SocketLen);
		if (ret > 0) {
			SaveStationMsg(pBuf, ret, &addr);
			if (THDDEVISLP) {
				THDDEVISLP = FALSE;
				pthread_mutex_lock(&DEVIMUTEX);
				pthread_cond_signal(&DEVICOND);
				pthread_mutex_unlock(&DEVIMUTEX);
			}
		}
	}
	return (void *)0;
}
/***************************scale***********************************/
