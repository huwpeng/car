#ifndef _ZFY_CONFIG_H_
#define _ZFY_CONFIG_H_

#ifdef	__cplusplus
#if		__cplusplus
extern "C" {
#endif
#endif


#include "Common.h"



#define DEFAULT_SYS_IPC_IP								"192.168.8.108"
#define DEFAULT_SYS_IPC_USER							"admin"
#define DEFAULT_SYS_IPC_PASSWD							"123456abc"
#define DEFAULT_SYS_IPC_PORT							80



#define DEFAULT_SYS_CURR_CONFIG_DATABASE_FILE			"/opt/ConfigDataBase"


typedef struct _IPC_CONFIG
{
	DWORD		IpcIp;
	DWORD		IpcPort;
	char		IpcUser[32];
	char		IpcPwd[32];
	char		CapPath[5][64];
	char		RecordPath[3][64];
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

extern void ZFY_ConfConfigDataBaseInit(void);

extern BOOL ZFY_ConfIpcConfig(BOOL IsSet,PIPC_CONFIG pConfig);

extern BOOL ZFY_ConfProdConfig(BOOL IsSet,PPRODUCT_CONFIG pConfig);

extern BOOL ZFY_ConfCloadConfig(BOOL IsSet,PCLOAD_CONFIG pConfig);

extern BOOL ZFY_ConfFtpConfig(BOOL IsSet,PFTP_CONFIG pConfig);

extern BOOL ZFY_ConfApnConfig(BOOL IsSet,PAPN_CONFIG pConfig);






#ifdef	__cplusplus
#if		__cplusplus
}
#endif
#endif

/******************************************zfy_Conf.h******************************************************************************************************/
#endif
