#ifndef _ZFY_ROLA_H_
#define _ZFY_ROLA_H_

#ifdef	__cplusplus
#if		__cplusplus
extern "C" {
#endif
#endif


#include "Common.h"

extern BOOL ZFY_RolaDevOpen(void);

extern void ZFY_RolaDevClose(void);

extern BOOL ZFY_RolaWriteData(const void *pBuf,DWORD dwSize,BOOL IsAutoOver,const DWORD *pTimeOutMS);

extern BOOL ZFY_RolaReadData(void *pBuf,DWORD *pBufSize,const DWORD *pTimeOutMS);





#ifdef	__cplusplus
#if		__cplusplus
}
#endif
#endif

/******************************************zfy_Rola.h******************************************************************************************************/
#endif
