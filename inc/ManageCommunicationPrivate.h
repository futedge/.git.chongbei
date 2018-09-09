#ifndef _MANAGECOMMUNICATIONPRIVATE_H
#define _MANAGECOMMUNICATIONPRIVATE_H

typedef enum {
	eId = 0,
	eCmd,
	ePort,
	eData1,
	eData2,
	eOrdNo,
	eErr
} steep_t;

typedef struct {
	steep_t emStep;
	u08 count;
	u64 id;
	u08 cmd;
	u08 port;
	u16 data1;
	u16 data2;
	char * pBuf;
	bool bAnalyseStatus;
} FSMCondition_t;

#endif
/***************************scale***********************************/
