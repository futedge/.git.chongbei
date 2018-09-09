#ifndef _MANAGEDEVICEPRIVATE_H
#define _MANAGEDEVICEPRIVATE_H

typedef enum {
	eMark,
	eXor,
	eEnd,
	eContent
} condition_t;

typedef enum {
	eError,
	eID,
	eTag,
	eLen,
	eData,
	eChecksum
} steep_t;
/*
typedef enum {
	eContinue,
	eErrTPMark,
	eErrChksum,
	eProcDev
} status_t;
*/
typedef struct {
	condition_t condition;
	steep_t step;
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
