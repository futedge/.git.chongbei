#ifndef _DATA_H
#define _DATA_H

const long BASE_TIME= 1420070400;/* 1970-1-1 0:0:0到2015-1-1 0:0:0
									的秒数 */
const int BUF_MAX	= 64;	// 充电站接收缓存大小
const int BUF_WEB_MAX= 128;	// web端命令接收缓存大小
const int BUF_CMD	= 32;	// web下发命令缓存大小
const int BUF_SQL	= 1024;	// sql命令缓存大小
const int RETRY		= 3;	// 命令失败或无回复重复次数
const int CMD_LIFE	= 127;	// C0 C3初始化命令生命周期
const int SPACE		= 3;	// 两条命令最小间隔时间s
const int COIN_VAILD= 300;	// 定义投币反馈有效间隔，大于此值有效
const int OFF_TIME	= 90;	// 定义离线时长90s
const int SR_PORT	= 8800;	// Server监听端口TCP
const int ST_PORT	= 9000;	// Station监听端口UDP
const int SEL_OUT	= 10;	// select超时时间
const int CON_OUT	= 20;	// 连接超时时间
const int CMD_OUT	= 10;	// 命令超时时间
const int ID_LEN	= 11;	// id长度，用于验证数据有效性
const int CMD_LEN	= 2;	// 命令长度，用于验证数据有效性
const int PORT_LEN	= 2;	// 端口长度，用于验证数据有效性
const int DATA_LEN	= 4;	// 数据长度，用于验证数据有效性
const int ORD_NO_LEN= 13;	// 订单长度，用于验证数据有效性
const int SEM_BASE	= 0x1000;// 信号量key起始值
const int SEM_VAL	= 1;	// 信号量集数量
const int SOK_VAL	= 128;	// 完成连接但还没调用accept的连接数

// 物联网终端传输协议定义
const u08 TP_HEAD 	= 0x8E;
const u08 TP_MARK 	= 0x88;
const u08 TP_FILT 	= 0x8D;
const u08 TP_TAIL 	= 0x8F;
const u08 TP_FT_NM	= 0x20;
const u16 TP_TG_AB	= 0x00AB;
const u16 TP_TG_BB	= 0x00BB;
const u08 TP_ID_LN	= 5;
const u08 TP_TG_LN	= 2;
const u08 TP_LN_LN	= 2;
const u08 TP_HT_LN	= 5;
const u08 DEV_TYPE	= 2;
const u08 LEN_ADD	= 2;

data_t gData;

#endif
/***************************scale***********************************/
