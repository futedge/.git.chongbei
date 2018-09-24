#ifndef _MANAGEDEVICEPRIVATE_H
#define _MANAGEDEVICEPRIVATE_H

typedef enum {
	EMark,
	EXor,
	EEnd,
	EContent,
	EError
} condition_t;

typedef enum {
	EId,
	ETag,
	ELen,
	EData,
	EChecksum
} steep_t;

typedef struct {
	condition_t eCondition;
	steep_t eStep;
	int count;
	u64 id;
	u08 type;
	u16 tag;
	u16 len;
	u08 * pBuf;
	u08 checksum;
} FSMCondition_t;

#endif
/***************************scale***********************************/
