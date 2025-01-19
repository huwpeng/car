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
	
}IPC_CONFIG,*PIPC_CONFIG;




extern void ZFY_ConfConfigDataBaseInit(void);


extern BOOL ZFY_ConfIpcConfig(BOOL IsSet,PIPC_CONFIG pConfig);












#ifdef	__cplusplus
#if		__cplusplus
}
#endif
#endif

/******************************************zfy_Conf.h******************************************************************************************************/
#endif
