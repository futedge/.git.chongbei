#include "ListenServer.h"

static void AddFds(fd_set * pReadFds, int * pMaxFd);
static void CheckConnectionTimeout(time_t NowTime);
static void DeleteThis(server_t ** ppServer);
static void AddServer(int SvrFd, time_t NowTime);
static void CheckServerMsg(fd_set * pReadFds, time_t NowTime);
static void SaveServrMsg(server_t * pServer, u08 * pBuf, int len);

void * ListenServer(void * arg)
{
	int ret, SvrFd, MaxFd, val;
	fd_set ReadFds;
	struct timeval TimeOutVal;
	struct sockaddr_in addr;
	time_t NowTime;
	// val=1时设置端口可复用,否则未主动关闭端口则超时时间内不能再连上
	val = 1;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // == inet_addr("0.0.0.0");
	addr.sin_port = htons(SRPORT);
	if ((SvrFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Stream socket error:");
		exit(1);
	}
	// 设置套接字关联选项
	setsockopt(SvrFd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
	if (bind(SvrFd, (struct sockaddr *)&addr, \
									sizeof(struct sockaddr)) < 0) {
		perror("Bind stream socket error:");
		exit(1);
	}
	if (listen(SvrFd, SOKVAL) < 0) {
		perror("Listen stream socket error:");
		exit(1);
	}
	// 保存socket fd
	WBHD.SvrFd = SvrFd;
	while (1) {
		// !!!注意,select后会清空TimeOutVal,连续ret == 0,无法阻塞
		// 每次select前需重新设置TimeOutVal值
		// 若TimeOutVal为NULL是否需要每次设NULL,待测
		TimeOutVal.tv_sec = SELOUT;
		TimeOutVal.tv_usec = 0;
		FD_ZERO(&ReadFds);
		FD_SET(SvrFd, &ReadFds);
		MaxFd = SvrFd;
		AddFds(&ReadFds, &MaxFd);
		ret = select(MaxFd + 1, &ReadFds, NULL, NULL, &TimeOutVal);
		NowTime = time(NULL);
		if (ret > 0) {
			if (FD_ISSET(SvrFd, &ReadFds)) {
				AddServer(SvrFd, NowTime);
			}
			CheckServerMsg(&ReadFds, NowTime);
		}
		else if (ret == 0) {
			CheckConnectionTimeout(NowTime);
		}
		else {
			perror("Select error.\n");
		}
	}
	return (void *)0;
}

/********************************************************************
 *用途	: 添加监听fd
 *参数	: pReadFds指向监听文件集, pMaxFd指向最大文件fd
 *返回值	: 无
********************************************************************/
static void AddFds(fd_set * pReadFds, int * pMaxFd)
{
	server_t * pServer;
	P(SEMWBHD);
	pServer = WBHD.pHead->pNext;
	while (pServer != WBHD.pHead) {
		FD_SET(pServer->fd, pReadFds);
		*pMaxFd = pServer->fd > *pMaxFd ? pServer->fd : *pMaxFd;
		pServer = pServer->pNext;
	}
	V(SEMWBHD);
}

/********************************************************************
 *用途	: 检查连接超时情况,在CONOUT时间内没有通信则超时,删除当前
 *参数	: NowTime当前时间
 *返回值	: 无
********************************************************************/
static void CheckConnectionTimeout(time_t NowTime)
{
	server_t * pServer;
	P(SEMWBHD);
	pServer = WBHD.pHead->pNext;
	while (pServer != WBHD.pHead) {
		if (NowTime - pServer->update > CONOUT) {
			DeleteThis(&pServer);
		}
		pServer = pServer->pNext;
	}
	V(SEMWBHD);
}

/********************************************************************
 *用途	: 删除当前节点,最后一个时直接删除并设置指针为空
 *参数	: ppServer指向当前节点指针的指针
 *返回值	: 无
********************************************************************/
static void DeleteThis(server_t ** ppServer)
{
	server_t * pServerTmp;
	(*ppServer)->pPrev->pNext = (*ppServer)->pNext;
	(*ppServer)->pNext->pPrev = (*ppServer)->pPrev;
	pServerTmp = (*ppServer);
	(*ppServer) = (*ppServer)->pPrev;
	close(pServerTmp->fd);
	free(pServerTmp);
	--(WBHD.num);
}

/********************************************************************
 *用途	: 增加一个服务器连接节点
 *参数	: SvrFd soket接口fd,NowTime当前时间
 *返回值	: 无
********************************************************************/
static void AddServer(int SvrFd, time_t NowTime)
{
	struct sockaddr_in addr;
	socklen_t SocketLen;
	server_t * pServer;
	if (!(pServer = (server_t *)malloc(sizeof(server_t)))) {
		perror("Not enough memory for malloc server_t:");
		exit(1);
	}
	memset(pServer, 0, sizeof(server_t));
	memset(&addr, 0, sizeof(addr));
	SocketLen = sizeof(addr);
	if ((pServer->fd = accept(SvrFd, (struct sockaddr *)&addr, \
												&SocketLen)) < 0) {
		perror("Accept client error:");
		return;
	}
/*  检查服务器地址，不正确断开连接，待修改
	if (argc == 2) {
		if (strcmp(argv[1], (char *)inet_ntoa(addr.sin_addr))) {
			close(pServer->fd);
			free(pServer);
			return;
		}
	}
*/
	pServer->launch = NowTime;
	pServer->update = NowTime;
	pServer->addr = addr;
	P(SEMWBHD);
	pServer->pPrev = WBHD.pHead->pPrev;
	pServer->pNext = WBHD.pHead;
	pServer->pPrev->pNext = pServer;
	WBHD.pHead->pPrev = pServer;
	++(WBHD.num);
	V(SEMWBHD);
}

/********************************************************************
 *用途	: 接收相应的服务器信息并保存到RCSR.pHead
 *参数	: pReadFds指向监听文件集,NowTime当前时间
 *返回值	: 无
********************************************************************/
static void CheckServerMsg(fd_set * pReadFds, time_t NowTime)
{
	server_t * pServer;
	u08 * pBuf;
	int len;
	P(SEMWBHD);
	pServer = WBHD.pHead->pNext;
	while (pServer != WBHD.pHead) {
		if (FD_ISSET(pServer->fd, pReadFds)) {
			pServer->update = NowTime;
			if (!(pBuf = (u08 *)malloc(BUFWEBMAX))) {
				perror("Not enough memory for malloc, recv web:");
				exit(1);
			}
			memset(pBuf, 0, BUFWEBMAX);
			len = read(pServer->fd, pBuf, BUFWEBMAX - 1);
			if (len > 0) {
				SaveServrMsg(pServer, pBuf, len);
				if (THDCOMMSLP) {
					pthread_mutex_lock(&COMMMUTEX);
					pthread_cond_signal(&COMMCOND);
					pthread_mutex_unlock(&COMMMUTEX);
				}
			}
			else if (len == 0) {
				DeleteThis(&pServer);
			}
			else {
				perror("Read web server error:");
			}
		}
		pServer = pServer->pNext;
	}
	V(SEMWBHD);
}

/********************************************************************
 *用途	: 保存接收信息到RCSR.pHead
 *参数	: pServer指向当前服务器节点,pBuf指向接收数据,len接收数据长度
 *返回值	: 无
********************************************************************/
static void SaveServrMsg(server_t * pServer, u08 * pBuf, int len)
{
	recv_t * pRecv;
	if (!(pRecv = (recv_t *)malloc(sizeof(recv_t)))) {
			perror("Not enough memory malloc to save recv web:");
			exit(1);
	}
	pRecv->pBuf = pBuf;
	pRecv->len = len;
	pRecv->time = pServer->update;
	pRecv->fd = pServer->fd;
	pRecv->launch = pServer->launch;
	P(SEMRCSR);
	pRecv->pPrevRecv = RCSR.pHead->pPrevRecv;
	pRecv->pNextRecv = RCSR.pHead;
	pRecv->pPrevRecv->pNextRecv = pRecv;
	RCSR.pHead->pPrevRecv = pRecv;
	++(RCSR.num);
	V(SEMRCSR);
}
/***************************scale***********************************/
