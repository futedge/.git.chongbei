#include "ListenDevice.h"
#include "ListenServer.h"
#include "ManageCommunication.h"
#include "ManageDevice.h"
#include "ManageOnceCmd.h"
#include "data.h"
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
void init(void)
{
	memset(&gData, 0, sizeof(data_t));
	// DVHD init
	if (!(DVHD.pstHead = (station_t *)malloc(sizeof(station_t)))) {
		perror("Not enough memory malloc for RecvServer.pstHead:");
		exit(1);
	}
	memset(DVHD.pstHead, 0, sizeof(station_t));
	DVHD.pstHead->pstPrev = DVHD.pstHead;
	DVHD.pstHead->pstNext = DVHD.pstHead;
	// WBHD init
	if (!(WBHD.pstHead = (server_t *)malloc(sizeof(server_t)))) {
		perror("Not enough memory malloc for RecvServer.pstHead:");
		exit(1);
	}
	memset(WBHD.pstHead, 0, sizeof(server_t));
	WBHD.pstHead->pstPrev = WBHD.pstHead;
	WBHD.pstHead->pstNext = WBHD.pstHead;
	// OCHD init
	if (!(OCHD = (SendCmd_t *)malloc(sizeof(SendCmd_t)))) {
		perror("Not enough memory malloc for OnSendCmd:");
		exit(1);
	}
	memset(OCHD, 0, sizeof(SendCmd_t));
	OCHD->pstPrev = OCHD;
	OCHD->pstNext = OCHD;
	// TCHD init
	if (!(TCHD = (SendCmd_t *)malloc(sizeof(SendCmd_t)))) {
		perror("Not enough memory malloc for TrSendCmd:");
		exit(1);
	}
	memset(TCHD, 0, sizeof(SendCmd_t));
	TCHD->pstPrev = TCHD;
	TCHD->pstNext = TCHD;
	// RCST init
	if (!(RCST.pstHead = (recv_t *)malloc(sizeof(recv_t)))) {
		perror("Not enough memory malloc for RecvStation.pstHead:");
		exit(1);
	}
	memset(RCST.pstHead, 0, sizeof(recv_t));
	RCST.pstHead->pstPrev = RCST.pstHead;
	RCST.pstHead->pstNext = RCST.pstHead;
	// RCSR init
	if (!(RCSR.pstHead = (recv_t *)malloc(sizeof(recv_t)))) {
		perror("Not enough memory malloc for RecvServer.pstHead:");
		exit(1);
	}
	memset(RCSR.pstHead, 0, sizeof(recv_t));
	RCSR.pstHead->pstPrev = RCSR.pstHead;
	RCSR.pstHead->pstNext = RCSR.pstHead;
	// set semid
	SEMDVHD = MakeSemid(SEMBASE);
	SetSemidVal(SEMDVHD, SEMVAL);
	SEMWBHD = MakeSemid(SEMBASE);
	SetSemidVal(SEMWBHD, SEMVAL);
	SEMOCCMD = MakeSemid(SEMBASE);
	SetSemidVal(SEMOCCMD, SEMVAL);
	SEMTRCMD = MakeSemid(SEMBASE);
	SetSemidVal(SEMTRCMD, SEMVAL);
	SEMRCST = MakeSemid(SEMBASE);
	SetSemidVal(SEMRCST, SEMVAL);
	SEMRCSR = MakeSemid(SEMBASE);
	SetSemidVal(SEMRCSR, SEMVAL);
	SEMPORT = MakeSemid(SEMBASE);
	SetSemidVal(SEMPORT, SEMVAL);
	if (pthread_mutex_init(&COMMMUTEX, NULL)) {
		perror("mutex init error:");
		exit(1);
	} 
    if (pthread_cond_init(&COMMCOND, NULL)) {
		perror("cond init error:");
		exit(1);
	}
	if (pthread_mutex_init(&DEVIMUTEX, NULL)) {
		perror("mutex init error:");
		exit(1);
	} 
    if (pthread_cond_init(&DEVICOND, NULL)) {
		perror("cond init error:");
		exit(1);
	}
	if (pthread_mutex_init(&ONCEMUTEX, NULL)) {
		perror("mutex init error:");
		exit(1);
	} 
    if (pthread_cond_init(&ONCECOND, NULL)) {
		perror("cond init error:");
		exit(1);
	}
	if (pthread_mutex_init(&TRACEMUTEX, NULL)) {
		perror("mutex init error:");
		exit(1);
	} 
    if (pthread_cond_init(&TRACECOND, NULL)) {
		perror("cond init error:");
		exit(1);
	}
}
/********************************************************************
 *用途	: 
 *参数	: 
 *返回值: 
********************************************************************/
int main(int argc, char ** argv)
{
	pthread_t ThdLstDev, ThdLstSvr, ThdComm;
	pthread_t	ThdDev, ThdOnCmd, ThdTrCmd;
	pthread_attr_t ThdAttr;
	
	if (argc > 2) {
		printf("Too many arguments.\n");
		exit(0);
	}
	
	init();
	
	memset(&ThdAttr, 0, sizeof(ThdAttr));
	// 初始化线程属性
	pthread_attr_init(&ThdAttr);
	// 设置子线程分离属性
	pthread_attr_setdetachstate(&ThdAttr, PTHREAD_CREATE_DETACHED);
	// 创建设备监听子线程
	if (pthread_create(&ThdLstDev, &ThdAttr, ListenDevice, NULL)) {
		perror("Create ThdLstDev error:");
		exit(1);
	}
	// 创建服务器监听子线程
	if (pthread_create(&ThdLstSvr, &ThdAttr, ListenServer, NULL)) {
		perror("Create ThdLstSvr error:");
		exit(1);
	}
	// 创建通信管理子线程
	if (pthread_create(&ThdComm, &ThdAttr, \
									  ManageCommunication, NULL)) {
		perror("Create ThdComm error:");
		exit(1);
	}
	// 创建设备管理子线程
	if (pthread_create(&ThdDev, &ThdAttr, ManageDevice, NULL)) {
		perror("Create ThdDev error:");
		exit(1);
	}
	// 创建单发命令子线程
	if (pthread_create(&ThdOnCmd, &ThdAttr, ManageOnceCmd, NULL)) {
		perror("Create ThdOnceCmd error:");
		exit(1);
	}
	// 创建跟踪命令子线程
	if (pthread_create(&ThdTrCmd, &ThdAttr, ManageTraceCmd, NULL)) {
		perror("Create ThdTraceCmd error:");
		exit(1);
	}
	pthread_attr_destroy(&ThdAttr);
	// 检查
	while (1){
		sleep(10);
	}
	return 0;
}
/***************************scale***********************************/