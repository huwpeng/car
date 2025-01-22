#ifndef _ZFY_TCP_SERVER_H_
#define _ZFY_TCP_SERVER_H_

#ifdef	__cplusplus
#if		__cplusplus
extern "C" {
#endif
#endif


#include "Common.h"

typedef struct _CLOAD_PLAT_CONF
{
	const char				*strCloadServerIP;
	unsigned short int		CloadServerPort;
	const char				*strLoginUser;
	const char				*strLoginPwd;
	const char				*strDevIMEI;
	const char				*strDevIMSI;
}CLOAD_PLAT_CONF,*PCLOAD_CONF_CONF;

extern BOOL ZFY_CloadServerStatus(BOOL *pIsAlive, BOOL *pIsLogin);

extern BOOL ZFY_CloadServerOpen(PCLOAD_CONF_CONF pConf);

extern void ZFY_CloadServerClose(void);


#ifdef	__cplusplus
#if		__cplusplus
}
#endif
#endif

/****************************************************************************zfy_Platform.h******************************************************************************************************/
#endif