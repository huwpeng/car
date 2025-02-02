#ifndef _ZFY_CONFIG_H_
#define _ZFY_CONFIG_H_

#ifdef	__cplusplus
#if		__cplusplus
extern "C" {
#endif
#endif


#include "Common.h"

#define DEFAULT_IPC_PIC_MAX_NUM							(5)
#define DEFAULT_IPC_REC_MAX_NUM							(3)

#define DEFAULT_SYS_IPC_IP								"192.168.8.108"
#define DEFAULT_SYS_IPC_PORT							80
#define DEFAULT_SYS_IPC_USER							"admin"
#define DEFAULT_SYS_IPC_PASSWD							"123456abc"

#define DEFAULT_SYS_PLATFORM_IP							"192.168.8.11"
#define DEFAULT_SYS_PLATFORM_PORT						1234
#define DEFAULT_SYS_PLATFORM_USER						"admin"
#define DEFAULT_SYS_PLATFORM_PASSWD						"admin"

#define DEFAULT_SYS_FTP_IP								"192.168.8.11"
#define DEFAULT_SYS_FTP_PORT							21
#define DEFAULT_SYS_FTP_USER							"ftp"
#define DEFAULT_SYS_FTP_PASSWD							"ftp"

#define DEFAULT_SYS_APN_NAME							"apn"
#define DEFAULT_SYS_APN_USER							"888888"
#define DEFAULT_SYS_APN_PASSWD							"888888"

#define DEFAULT_SYS_CURR_CONFIG_DATABASE_FILE			"/opt/ConfigDataBase"


typedef struct _IPC_CONFIG
{
	DWORD		AlarmFlag;
	DWORD		IpcIp;
	DWORD		IpcPort;
	char		IpcUser[32];
	char		IpcPwd[32];
	time_t		RecordStart[DEFAULT_IPC_REC_MAX_NUM];
	time_t		RecordEnd[DEFAULT_IPC_REC_MAX_NUM];
	char		CapPath[DEFAULT_IPC_PIC_MAX_NUM][64];
	char		RecordPath[DEFAULT_IPC_REC_MAX_NUM][64];
}IPC_CONFIG,*PIPC_CONFIG;

typedef struct _PRODUCT_CONFIG
{
	char		SoftVer[16];
	char		ProductName[16];
	char		ProductModel[16];
	char		VendorName[16];
	char		DevSerialNum[16];
	char		ProductDate[16];
}PRODUCT_CONFIG,*PPRODUCT_CONFIG;

typedef struct _CLOAD_CONFIG
{
	DWORD		ServerIp;
	DWORD		ServerPort;
	char		LoginUser[32];
	char		LoginPwd[32];
}CLOAD_CONFIG,*PCLOAD_CONFIG;

typedef struct _FTP_CONFIG
{
	DWORD		ServerIp;
	DWORD		ServerPort;
	char		FtpUser[32];
	char		FtpPwd[32];
}FTP_CONFIG,*PFTP_CONFIG;

typedef struct _APN_CONFIG
{
	char		ApnName[32];
	char		ApnUser[32];
	char		ApnPwd[32];
}APN_CONFIG,*PAPN_CONFIG;


/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_ConfConfigDataBaseInit
 *功能描述: 配置参数模块初始化
 *输入描述: 无
 *输出描述: 无
 *返回描述: TRUE/FALSE
 *作者日期: LJJ/2024/12/02
 *全局声明: sLocalMutex,sSysConfigDatabase
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern void ZFY_ConfConfigDataBaseInit(void);

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_ConfIpcConfig
 *功能描述: IPC参数配置
 *输入描述: 设置使能,参数
 *输出描述: 无
 *返回描述: TRUE/FALSE
 *作者日期: LJJ/2024/12/02
 *全局声明: sLocalMutex,sSysConfigDatabase
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_ConfIpcConfig(BOOL IsSet,PIPC_CONFIG pConfig);

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_ConfProdConfig
 *功能描述: 产品参数配置
 *输入描述: 设置使能,参数
 *输出描述: 无
 *返回描述: TRUE/FALSE
 *作者日期: LJJ/2024/12/02
 *全局声明: sLocalMutex,sSysConfigDatabase
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_ConfProdConfig(BOOL IsSet,PPRODUCT_CONFIG pConfig);

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_ConfCloadConfig
 *功能描述: 平台参数配置
 *输入描述: 设置使能,参数
 *输出描述: 无
 *返回描述: TRUE/FALSE
 *作者日期: LJJ/2024/12/02
 *全局声明: sLocalMutex,sSysConfigDatabase
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_ConfCloadConfig(BOOL IsSet,PCLOAD_CONFIG pConfig);

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_ConfFtpConfig
 *功能描述: FTP参数配置
 *输入描述: 设置使能,参数
 *输出描述: 无
 *返回描述: TRUE/FALSE
 *作者日期: LJJ/2024/12/02
 *全局声明: sLocalMutex,sSysConfigDatabase
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_ConfFtpConfig(BOOL IsSet,PFTP_CONFIG pConfig);

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_ConfApnConfig
 *功能描述: APN参数配置
 *输入描述: 设置使能,参数
 *输出描述: 无
 *返回描述: TRUE/FALSE
 *作者日期: LJJ/2024/12/02
 *全局声明: sLocalMutex,sSysConfigDatabase
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_ConfApnConfig(BOOL IsSet,PAPN_CONFIG pConfig);






#ifdef	__cplusplus
#if		__cplusplus
}
#endif
#endif

/******************************************zfy_Conf.h******************************************************************************************************/
#endif
