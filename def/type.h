#ifndef _TYPE_H
#define _TYPE_H

#include <arpa/inet.h>	// struct sockaddr_in AF_INET socket...
#include <sys/sem.h>	//Semaphore
#include <sys/types.h>
#include <pthread.h>	// thread
#include <stdlib.h>		// exit atoi
						// !!strtol没有此头文件则返回值宽度4字节
#include <string.h>		// memset strlen
#include <ctype.h>		// isxdigit
#include <errno.h>		// errno
#include <time.h>		// struct tm
#include <unistd.h>		// read() write()
#include <stdio.h>		// printf perror

// 定义常用类型
typedef	unsigned char	u08;
typedef unsigned short	u16;
typedef unsigned int	u32;
typedef unsigned long	u64;
typedef enum {FALSE, TRUE} bool;

// 待处理命令,收到相应反馈或超时后删除
typedef struct tagCmd_t {
	time_t time;		// 命令创建时间
	time_t EndTime;		// 创建时间+充电时间
	server_t * pstSvr;	// 命令对应Server节点
	u08 port;			// 相应端口
	u08 cmd;			// 具体命令
	u08 len;			// 命令字节数
	u08 * pBuf;			// 完整命令包
	char * pOrderNo;	// 相应订单号
	struct tagCmd_t * pstPrevCmd;
	struct tagCmd_t * pstNextCmd;
} cmd_t;

// 跟踪开启后待闭端口,如超时程序关闭
typedef struct {
	time_t time;
	time_t EndTime;
	u08 port;
	u08 cmd;
	char * pOrderNo;	/* 开启反馈成功后用户未使用,端口
						自动关闭,则反馈相应订单作退款处理 */
	struct tagCmd_t * pstPrevChk;
	struct tagCmd_t * pstNextChk;
} chk_t;

// 站端
typedef struct tagStation_t {
	u64 id;				// 站id号,即手机号
	u08 version;		// 站版本
	bool bInSendList;	// 是否在发送队列标志
	u08 status;			/* 站状态,0-OK,1-未初始化,2-无法通信,
						3-不能操作 */
	time_t launch;		// 站第一次上线时间,创建时记录,终生不变
	time_t update;		// 站最后一次更新时间
	u32 ports;			/* 端口状态,1使用中0空闲,
						低到高每位代表一端口,共可表示4*8=32个端口 */
	u08 StdCurrent[2];	// 标准电流值,[0]实际值,[1]待设值,下同
	u08 MaxCurrent[2];	// 最大电流值
	u08 NetPrice[2];	// 网络消费标准
	u16 NetTime[2];		// 网络消费标准对应充电时间
	u08 NetDiscount[2];	// 网络消费折扣
	u08 CardPrice[2];	// 刷卡消费标准
	u16 CardTime[2];	// 刷卡消费标准对应充电时间
	u08 CoinPrice[2];	// 投币消费标准
	u16 CoinTime[2];	// 投币消费标准对应充电时间
	cmd_t * pCmd;		// 待处理命令
	chk_t * pChk;		// 待检查状态端口
	struct sockaddr_in	addr;
	struct tagStation_t * pstPrev;
	struct tagStation_t * pstNext;
} station_t;

// 站端头
typedef struct {
	station_t * pstHead;// 站链表头指针
	station_t ** ppArr;	// 站指针数组,128站设置一个指针
	u32 num;			// 站计数
	int SvrFd;			// station listen socket fd
} devi_t;

// web端
typedef struct tagServer_t {
	int fd;				// web端连接文件描述符
	time_t launch;		// web连接建立时间,创建时记录,终生不变
	time_t update;		// web端最后一次更新时间
	struct sockaddr_in	addr;
	struct tagServer_t * pPrev;
	struct tagServer_t * pNext;
} server_t;

// web端头
typedef struct {
	server_t * pstHead;	// web端链表头指针
	server_t ** ppArr;	// web端指针数组,128站设置一个指针
	u32 num;			// web端计数
	int SvrFd;			// webserver linsten socket fd
} web_t;

// 发送命令列表端头
typedef struct tagSendCmd_t {
	station_t * pstStation;
	struct tagSendCmd_t * pstPrev;
	struct tagSendCmd_t * pstNext;
} SendCmd_t;

// 站端及web端接收数据存储
typedef struct tagRecv_t {
	u08 * pBuf;
	u16 len;
	time_t time;
	union {
		struct {
			server_t * pstSvr;
			time_t launch;
		};
		struct sockaddr_in addr;
	};
	struct tagRecv_t * pstPrev;
	struct tagRecv_t * pstNext;
} recv_t;

// web接收存储头
typedef struct {
	recv_t * pstHead;	// web端链表头指针
	u32 num;			// web端计数
} storage_t;

typedef struct {
	devi_t stDeviceHead;		// 站链表头
	web_t  stWebSvrHead;		// web链表头
	SendCmd_t * stOnCmdHead;	// 单发命令列表端头
	SendCmd_t * stTrCmdHead;	// 跟踪命令列表端头
	storage_t stRecvStation;	// 站信息存储
	storage_t stRecvServer;		// 服务器信息存储
	int SemDvHd;				// 站更新信号量
	int	SemWbHd;				// web更新信号量
	int SemOnCmd;				// 单发命令存储信号量
	int SemTrCmd;				// 跟踪命令存储信号量
	int SemRcSt;				// 站接收信息存储信号量
	int SemRcSr;				// web接收信息存储信号量
	int	SemPort;				// 充电站端口信号量
	bool ThdManComm;			// ManageCommunication线程标记
	bool ThdManDevi;			// 设备管理线程标记
	bool ThdManOnceCmd;			// 起始命令线程运行标记
	bool ThdManTraceCmd;		// 跟踪命令线程运行标记
	pthread_mutex_t CommMutex;	// ManageCommunication线程互斥锁
	pthread_cond_t  CommCond;	// ManageCommunication线程容器
	pthread_mutex_t DeviMutex;	// ManageDevice线程互斥锁
	pthread_cond_t  DeviCond;	// ManageDevice线程容器
	pthread_mutex_t OnCmdMutex;	// OnceCmdList线程互斥锁
	pthread_cond_t  OnCmdCond;	// OncecmdList线程容器
	pthread_mutex_t TrCmdMutex;	// TraceCmdList线程互斥锁
	pthread_cond_t  TrCmdCond;	// TraceCmdList线程容器
} data_t;

#define DVHD		(gData.stDeviceHead)
#define WBHD		(gData.stWebSvrHead)
#define OCHD		(gData.stOnCmdHead)
#define TRHD		(gData.stTrCmdHead)
#define RCST		(gData.stRecvStation)
#define RCSR		(gData.stRecvServer)
#define SEMDVHD		(gData.SemDvHd)
#define SEMWBHD		(gData.SemWbHd)
#define SEMOCCMD	(gData.SemOnCmd)
#define SEMTRCMD	(gData.SemTrCmd)
#define SEMRCST		(gData.SemRcSt)
#define SEMRCSR		(gData.SemRcSr)
#define SEMPORT		(gData.SemPort)
#define THDCOMMSLP	(gData.ThdManComm)
#define THDDEVISLP	(gData.ThdManDevi)
#define THDONCESLP	(gData.ThdManOnceCmd)
#define THDTRACSLP	(gData.ThdManTraceCmd)
#define COMMMUTEX	(gData.CommMutex)
#define COMMCOND	(gData.CommCond)
#define DEVIMUTEX	(gData.DeviMutex)
#define DEVICOND	(gData.DeviCond)
#define ONCEMUTEX	(gData.OnCmdMutex)
#define ONCECOND	(gData.OnCmdCond)
#define TRACEMUTEX	(gData.TrCmdMutex)
#define TRACECOND	(gData.TrCmdCond)

// const常量
extern const long	BASETIME;
extern const int	BUFMAX;
extern const int	BUFWEBMAX;
extern const int	BUFCMD;
extern const int	BUFSQL;
extern const int	RETRY;
extern const int	CMDLIFE;
extern const int	SPACE;
extern const int	COINVAILD;
extern const int	OFFTIME;
extern const int	SRPORT;
extern const int	STPORT;
extern const int	SELOUT;
extern const int	CONOUT;
extern const int	CMDOUT;
extern const int	MSGUP;
extern const int	MSGDW;
extern const int	MSGMODE;
extern const int 	IDLEN;
extern const int	CMDLEN;
extern const int	PORTLEN;
extern const int	DATALEN;
extern const int	ORDNOLEN;
extern const int	SEMBASE;
extern const int	SEMVAL;
extern const int	SOKVAL;
extern const bool	TRUE;
extern const bool	FALSE;

extern const u08 TPHEAD;
extern const u08 TPMARK;
extern const u08 TPFILT;
extern const u08 TPTAIL;
extern const u08 TPFTNM;
extern const u08 TPIDLN;
extern const u08 TPTGLN;
extern const u08 TPLNLN;
extern const u08 TPHTLN;

extern data_t gData;

#endif
/***************************scale***********************************/
