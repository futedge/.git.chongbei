#ifndef _MANAGECOMMUNICATIONPRIVATE_H
#define _MANAGECOMMUNICATIONPRIVATE_H

typedef enum {
	E_type1 = 3,
	E_type2 = 27,
	E_type3 = 63
} ValidType_t;

typedef enum {
	E_id = 0,
	E_cmd,
	E_port,
	E_data1,
	E_data2,
	E_OrdNo,
	E_err
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
} FSM_Condition_t;

#endif
/***************************scale***********************************/
