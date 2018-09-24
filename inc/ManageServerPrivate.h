#ifndef _MANAGECOMMUNICATIONPRIVATE_H
#define _MANAGECOMMUNICATIONPRIVATE_H

typedef enum {
	EType1 = 3,
	EType2 = 27,
	EType3 = 63
} ValidType_t;

typedef enum {
	EId = 0,
	ECmd,
	EPort,
	EData1,
	EData2,
	EOrdNo,
	EErr
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
	union {
		u08 word;
		typedef struct {
			u08 ValidId		: 1;
			u08 ValidCmd	: 1;
			u08 ValidPort	: 1;
			u08 ValidData1	: 1;
			u08 ValidData2	: 1;
			u08 ValidOrNo	: 1;
			u08 _reserved	: 2;
		} bit;
	};
} FSMCondition_t;

#endif
/***************************scale***********************************/
