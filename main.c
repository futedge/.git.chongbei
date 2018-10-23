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
	// DV_HD init
	if (!(DV_HD.pstHead = (station_t *)malloc(sizeof(station_t)))) {
		perror("Not enough memory malloc for RecvServer.pstHead:");
		exit(1);
	}
	memset(DV_HD.pstHead, 0, sizeof(station_t));
	DV_HD.pstHead->pstPrev = DV_HD.pstHead;
	DV_HD.pstHead->pstNext = DV_HD.pstHead;
	// WB_HD init
	if (!(WB_HD.pstHead = (server_t *)malloc(sizeof(server_t)))) {
		perror("Not enough memory malloc for RecvServer.pstHead:");
		exit(1);
	}
	memset(WB_HD.pstHead, 0, sizeof(server_t));
	WB_HD.pstHead->pstPrev = WB_HD.pstHead;
	WB_HD.pstHead->pstNext = WB_HD.pstHead;
	// ON_CM_HD init
	if (!(ON_CM_HD = (SendCmd_t *)malloc(sizeof(SendCmd_t)))) {
		perror("Not enough memory malloc for OnSendCmd:");
		exit(1);
	}
	memset(ON_CM_HD, 0, sizeof(SendCmd_t));
	ON_CM_HD->pstPrev = ON_CM_HD;
	ON_CM_HD->pstNext = ON_CM_HD;
	// TCHD init
	if (!(TCHD = (SendCmd_t *)malloc(sizeof(SendCmd_t)))) {
		perror("Not enough memory malloc for TrSendCmd:");
		exit(1);
	}
	memset(TCHD, 0, sizeof(SendCmd_t));
	TCHD->pstPrev = TCHD;
	TCHD->pstNext = TCHD;
	// RC_ST init
	if (!(RC_ST.pstHead = (recv_t *)malloc(sizeof(recv_t)))) {
		perror("Not enough memory malloc for RecvStation.pstHead:");
		exit(1);
	}
	memset(RC_ST.pstHead, 0, sizeof(recv_t));
	RC_ST.pstHead->pstPrev = RC_ST.pstHead;
	RC_ST.pstHead->pstNext = RC_ST.pstHead;
	// RC_SR init
	if (!(RC_SR.pstHead = (recv_t *)malloc(sizeof(recv_t)))) {
		perror("Not enough memory malloc for RecvServer.pstHead:");
		exit(1);
	}
	memset(RC_SR.pstHead, 0, sizeof(recv_t));
	RC_SR.pstHead->pstPrev = RC_SR.pstHead;
	RC_SR.pstHead->pstNext = RC_SR.pstHead;
	// set semid
	SEM_DV_HD = MakeSemid(SEM_BASE);
	SetSemidVal(SEM_DV_HD, SEM_VAL);
	SEM_WB_HD = MakeSemid(SEM_BASE);
	SetSemidVal(SEM_WB_HD, SEM_VAL);
	SEM_ON_CM = MakeSemid(SEM_BASE);
	SetSemidVal(SEM_ON_CM, SEM_VAL);
	SEM_TR_CM = MakeSemid(SEM_BASE);
	SetSemidVal(SEM_TR_CM, SEM_VAL);
	SEM_RC_ST = MakeSemid(SEM_BASE);
	SetSemidVal(SEM_RC_ST, SEM_VAL);
	SEM_RC_SR = MakeSemid(SEM_BASE);
	SetSemidVal(SEM_RC_SR, SEM_VAL);
	SEM_PORT = MakeSemid(SEM_BASE);
	SetSemidVal(SEM_PORT, SEM_VAL);
	if (pthread_mutex_init(&COMM_MUTEX, NULL)) {
		perror("mutex init error:");
		exit(1);
	} 
    if (pthread_cond_init(&COMM_COND, NULL)) {
		perror("cond init error:");
		exit(1);
	}
	if (pthread_mutex_init(&DEVI_MUTEX, NULL)) {
		perror("mutex init error:");
		exit(1);
	} 
    if (pthread_cond_init(&DEVI_COND, NULL)) {
		perror("cond init error:");
		exit(1);
	}
	if (pthread_mutex_init(&ONCE_MUTEX, NULL)) {
		perror("mutex init error:");
		exit(1);
	} 
    if (pthread_cond_init(&ONCE_COND, NULL)) {
		perror("cond init error:");
		exit(1);
	}
	if (pthread_mutex_init(&TRAC_MUTEX, NULL)) {
		perror("mutex init error:");
		exit(1);
	} 
    if (pthread_cond_init(&TRAC_COND, NULL)) {
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
	// 创建设备管理子线程
	if (pthread_create(&ThdDev, &ThdAttr, ManageDevice, NULL)) {
		perror("Create ThdDev error:");
		exit(1);
	}
	// 创建通信管理子线程
	if (pthread_create(&ThdComm, &ThdAttr, \
									  ManageServer, NULL)) {
		perror("Create ThdComm error:");
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