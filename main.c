#include "ListenDevice.h"
#include "ListenServer.h"
#include "ManageCommunication.h"
#include "ManageDevice.h"
#include "ManageThread.h"
#include "data.h"

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
	if (!(OCHD.pstHead = (recv_t *)malloc	(sizeof(recv_t)))) {
		perror("Not enough memory malloc for RecvStation.pstHead:");
		exit(1);
	}
	memset(OCHD.pstHead, 0, sizeof(recv_t));
	OCHD.pstHead->pstPrev = OCHD.pstHead;
	OCHD.pstHead->pstNext = OCHD.pstHead;
	// TCHD init
	if (!(TCHD.pstHead = (recv_t *)malloc	(sizeof(recv_t)))) {
		perror("Not enough memory malloc for RecvStation.pstHead:");
		exit(1);
	}
	memset(TCHD.pstHead, 0, sizeof(recv_t));
	TCHD.pstHead->pstPrev = TCHD.pstHead;
	TCHD.pstHead->pstNext = TCHD.pstHead;
	// RCST init
	if (!(RCST.pstHead = (recv_t *)malloc	(sizeof(recv_t)))) {
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
	SEMRCST = MakeSemid(SEMBASE);
	SetSemidVal(SEMRCST, SEMVAL);
	SEMRCSR = MakeSemid(SEMBASE);
	SetSemidVal(SEMRCSR, SEMVAL);
	SEMDVHD = MakeSemid(SEMBASE);
	SetSemidVal(SEMDVHD, SEMVAL);
	SEMWBHD = MakeSemid(SEMBASE);
	SetSemidVal(SEMWBHD, SEMVAL);
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
}

int main(int argc, char ** argv)
{
	pthread_t ThdLstDev, ThdLstSvr, ThdComm, ThdDev, ThdMan;
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
	// 创建线程管理子线程
	if (pthread_create(&ThdMan, &ThdAttr, ManageThread, NULL)) {
		perror("Create ThdMan error:");
		exit(1);
	}
	pthread_attr_destroy(&ThdAttr);
	
//	DVHD.num = 44;
//	printf("1:%d\n", DVHD.num);
	while (1){
		sleep(10);
	}
	return 0;
}
/***************************scale***********************************/
