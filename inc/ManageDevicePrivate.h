#ifndef _MANAGEDEVICEPRIVATE_H
#define _MANAGEDEVICEPRIVATE_H

typedef enum {
	EMark,
	EXor,
	EEnd,
	EContent
} condition_t;

typedef enum {
	EStart,
	EId,
	ETag,
	ELen,
	EData,
	EChecksum
} step_t;

typedef struct {
	condition_t eCondition;
	step_t eStep;
	u08 count;
	u64 id;
	u08 type;
	u16 tag;
	u16 len;
	u08 * pBuf;
	u08 checksum;
} FSMCondition_t;

typedef enum {
	EStartCP,
	ETagCP,
	EPortCP,
	EUserTypeCP,
	ECmdCP,
	ELenCP,
	EDataCP,
	EChecksumCP
} StepCP_t;

typedef struct {
	StepCP_t eStepCP;
	u08 count;
	// 端口
	u08 port;
	// 用户类型
	u08 UserType;
	// 功能码 A0/A4/A5/C0/C1/C2/C3/C4/C5/F7
	u08 cmd;
	// 数据字节数
	u08 len;
	// 反馈结果
	u08 ret;
	// 数据项缓存指针
	u08 pBuf;
	union {
		// 接收C0
		struct {
			// 网充单价
			u16 NetPrice;
			// 网充单价对应时间
			u16 NetTime;
			// 网充折扣
			u08 NetDiscount;
			// 卡充单价
			u16 CardPrice;
			// 卡充单价对应时间
			u16 CardTime;
			// 投币单价
			u16 CoinPrice;
			// 投币单价对应时间
			u16 CoinTime;
		};
			// C1投币消费总额
			u32 CoinQryTal;
			// C2卡刷消费总额
			u32 CardQryTal;
			// C3电流标准
		struct {
			// 标准电流
			u08 StdCurrent;
			// 最大电流
			u08 MaxCurrent;
		};
			// C5投币消费上报
		struct {
			// 投币单号
			u16 OrderNo;
			// 投币消费总金额
			u32 SumMoney;
			// 投币充电总时长
			u32 TotalTime;
			// 充电完成时间
			u32 EndTime;
			// 存储状态
			u08 CurStatus;
		};
	};
	u08 checksum;
} FSMConditionCP_t;

#endif
/***************************scale***********************************/
