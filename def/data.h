#ifndef _DATA_H
#define _DATA_H

const long BASETIME	= 1420070400;/* 1970-1-1 0:0:0到2015-1-1 0:0:0
									的秒数 */
const int BUFMAX	= 64;	// 充电站接收缓存大小
const int BUFWEBMAX	= 128;	// web端命令接收缓存大小
const int BUFSQL	= 1024;	// sql命令缓存大小
const int RETRY		= 3;	// 命令失败或无回复重复次数
const int CMDLIFE	= 127;	// C0 C3初始化命令生命周期
const int SPACE		= 3;	// 两条命令最小间隔时间s
const int COINVAILD	= 300;	// 定义投币反馈有效间隔，大于此值有效
const int OFFTIME	= 90;	// 定义离线时长90s
const int SRPORT	= 8800;	// Server监听端口TCP
const int STPORT	= 9000;	// Station监听端口UDP
const int SELOUT	= 10;	// select超时时间
const int CONOUT	= 20;	// 连接超时时间
const int CMDOUT	= 10;	// 命令超时时间
const int MSGUP		= 0x888;// 充电站服务器接收，web写
const int MSGDW		= 0x999;// 充电站服务器写，web接收
const int MSGMODE	= 0777;	// 权限设置
const int IDLEN		= 11;	// id长度，用于验证数据有效性
const int CMDLEN	= 2;	// 命令长度，用于验证数据有效性
const int PORTLEN	= 2;	// 端口长度，用于验证数据有效性
const int DATALEN	= 4;	// 数据长度，用于验证数据有效性
const int ORDNOLEN	= 13;	// 订单长度，用于验证数据有效性
const int SEMBASE	= 0x1000;// 信号量key起始值
const int SEMVAL	= 1;	// 信号量集数量
const int SOKVAL	= 128;	// 完成连接但还没调用accept的连接数
const bool TRUE		= 1;	// 条件为真
const bool FALSE	= 0;	// 条件为假

// 物联网终端传输协议定义
const u08 TPHEAD 	= 0x8E;
const u08 TPMARK 	= 0x88;
const u08 TPFILT 	= 0x8D;
const u08 TPTAIL 	= 0x8F;
const u08 TPFTNM	= 0x20;
const u08 TPIDLN	= 5;
const u08 TPTGLN	= 2;
const u08 TPLNLN	= 2;
const u08 TPHTLN	= 5;

data_t gData;

#endif
/***************************scale***********************************/
