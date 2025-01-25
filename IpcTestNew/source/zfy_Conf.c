/*
 ***************************************************************************************************************************
 ********************************************************************************************************************************
 *			PROGRAM MODULE
 *
 *			$Workfile:		zfy_Conf.c			$
 *			$Revision:		1.0					$
 *			$Author:		LJJ					$
 *			$Date:			2024/12/26			$
 *
 *			
 *
 ********************************************************************************************************************************
 ********************************************************************************************************************************
*/

/*
 **********************************************************************************************************************************
 * DESCRIPTION.
 *
 *	实现设备配置读写功能
 *
*/

/*
 *********************************************************************************************************************************
 * Rev 1.0   2024/12/26   
 * Initial revision.   
*/

/*
 **************************************************************************************************************************************
 * INCLUDE FILE.
 **************************************************************************************************************************************
*/
#include <sys/vfs.h>
#include "zfy_Conf.h"
#include "LogEventServer.h"



/*
 ************************************************************************************************************************************************************************
 * 常数定义
 ************************************************************************************************************************************************************************
*/

#define SYS_CURR_SOFTWARE_VERSION							0xA2220420

#define CURR_CONFIG_DATABASE_ID								0x10090623


/*
 ************************************************************************************************************************************************************************
 类型定义
 ************************************************************************************************************************************************************************
*/
typedef struct tSYSNETCONF
{
	U32     					IPAddr;
	U32    	 					IPMask;
	U32     					GateWay;
	U32							DevType;
	U16							wReserved;
	U8							EthAddr[6];
}SYSNETCONF,*PSYSNETCONF;

typedef struct tSYS_CONFIG_DATABASE
{
	DWORD						dwDatabaseVerify;											
	DWORD						dwDatabaseSize;												
	DWORD						dwDatabaseID;												
	SYSNETCONF					SysNetConfig;																							
	IPC_CONFIG					IpcConfig;	
	PRODUCT_CONFIG				ProdConfig;
	CLOAD_CONFIG				CloadConfig;
	FTP_CONFIG					FtpConfig;
	APN_CONFIG					ApnConfig;
	DWORD						Reserved[512];										
	DWORD						Md5Alignreverse;											
	unsigned char				Md5Check[16];												
}SYS_CONFIG_DATABASE,*PSYS_CONFIG_DATABASE;


/*
 ************************************************************************************************************************************************************************
 * 全局变量
 ************************************************************************************************************************************************************************
*/
static pthread_mutex_t					sLocalMutex=PTHREAD_MUTEX_INITIALIZER;
static BOOL								sIsInitReady=FALSE;
static SYS_CONFIG_DATABASE				sSysConfigDatabase;


/*
 ****************************************************************************************************************************************************
 * 函数定义
 *****************************************************************************************************************************************************
*/
static WORD Comm_MakeCrc(const void *pBuf, WORD Len)
{
	BYTE	*ptr=(BYTE *)pBuf;
	WORD	i, j;
	WORD	wCrc=0xFFFF;
	
	for (i=0; i<Len; i++)
	{
		wCrc	^= (WORD)(*ptr++);
		for (j=0; j<8; j++)
		{
			if (wCrc&0x01)
			{
				wCrc >>= 1; 
				wCrc ^= 0xA001; 
			}
			else
			{
				wCrc >>= 1; 
			}
		}
	}
	
	return	wCrc;
}

static BYTE MakeSum(const void *pBuf,DWORD Len)
{
	BYTE *ptr=(BYTE *)pBuf,Sum=0;

	while(Len--)
	{
		Sum	+= *ptr;
		ptr++;
	}
	
	return Sum;
}

static BOOL ZFY_ConfSaveCurrConfig(void)
{
	FILE	*pFile;
	DWORD	CurrVerify;
	size_t	Size;
	DWORD	i,RetVal;

	pFile=fopen(DEFAULT_SYS_CURR_CONFIG_DATABASE_FILE,"rb+");
	if(NULL==pFile)
		return FALSE;
	sSysConfigDatabase.dwDatabaseVerify=0;
	Size=fwrite(&sSysConfigDatabase,1,sizeof(sSysConfigDatabase),pFile);
	RetVal=fflush(pFile);
	RetVal |= fsync(fileno(pFile));
	fclose(pFile);
	return (Size==sizeof(sSysConfigDatabase) && RetVal==STD_SUCCESS);
}

static void ZFY_ConfDataBaseDefault(PSYS_CONFIG_DATABASE pDataBase)
{
	memset(pDataBase,0,sizeof(sSysConfigDatabase));
	pDataBase->IpcConfig.IpcIp=ntohl(inet_addr(DEFAULT_SYS_IPC_IP));
	pDataBase->IpcConfig.IpcPort=DEFAULT_SYS_IPC_PORT;
	strcpy(pDataBase->IpcConfig.IpcUser,DEFAULT_SYS_IPC_USER);
	strcpy(pDataBase->IpcConfig.IpcPwd,DEFAULT_SYS_IPC_PASSWD);
}

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
extern void ZFY_ConfConfigDataBaseInit(void)
{
	int				i,CancelStatus;
	BOOL			IsReInit=TRUE;
	FILE			*pFile=NULL;
	size_t			Size=0;

	PTHREAD_MUTEX_SAFE_LOCK(sLocalMutex,CancelStatus);
	if(sIsInitReady)
	{
		PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
		return;
	}

	if((pFile=fopen(DEFAULT_SYS_CURR_CONFIG_DATABASE_FILE,"rb"))!=NULL)
	{
		fseek(pFile,0,SEEK_END);
		fseek(pFile,0,SEEK_SET);
		Size=fread(&sSysConfigDatabase,1,sizeof(sSysConfigDatabase),pFile);
		if(Size==sizeof(sSysConfigDatabase) && Size==sSysConfigDatabase.dwDatabaseSize && sSysConfigDatabase.dwDatabaseID==CURR_CONFIG_DATABASE_ID)
		{
			sIsInitReady=TRUE;	
			PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
			fclose(pFile);
			return;
		}
	}

	if(IsReInit)
	{
		ZFY_ConfDataBaseDefault(&sSysConfigDatabase);
	}

	sSysConfigDatabase.dwDatabaseID=CURR_CONFIG_DATABASE_ID;
	sSysConfigDatabase.dwDatabaseSize=sizeof(sSysConfigDatabase);
	sSysConfigDatabase.dwDatabaseVerify=0;
	pFile=fopen(DEFAULT_SYS_CURR_CONFIG_DATABASE_FILE,"wb");
	if(NULL!=pFile)
	{
		fwrite(&sSysConfigDatabase,1,sizeof(sSysConfigDatabase),pFile);
		fflush(pFile);
		fclose(pFile);
	}

	sIsInitReady=TRUE;
	PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
}

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
extern BOOL ZFY_ConfIpcConfig(BOOL IsSet,PIPC_CONFIG pConfig)
{
	int 		CancelStatus;
	BOOL		RetVal;

	if(!sIsInitReady)
		ZFY_ConfConfigDataBaseInit();
	PTHREAD_MUTEX_SAFE_LOCK(sLocalMutex,CancelStatus);
	if(IsSet)
	{
		sSysConfigDatabase.IpcConfig=*pConfig;
		RetVal=ZFY_ConfSaveCurrConfig();
	}
	else
	{
		*pConfig=sSysConfigDatabase.IpcConfig;
		RetVal=TRUE;
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
	return RetVal;
}

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
extern BOOL ZFY_ConfProdConfig(BOOL IsSet,PPRODUCT_CONFIG pConfig)
{
	int 		CancelStatus;
	BOOL		RetVal;

	if(!sIsInitReady)
		ZFY_ConfConfigDataBaseInit();
	PTHREAD_MUTEX_SAFE_LOCK(sLocalMutex,CancelStatus);
	if(IsSet)
	{
		sSysConfigDatabase.ProdConfig=*pConfig;
		RetVal=ZFY_ConfSaveCurrConfig();
	}
	else
	{
		*pConfig=sSysConfigDatabase.ProdConfig;
		RetVal=TRUE;
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
	return RetVal;
}

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
extern BOOL ZFY_ConfCloadConfig(BOOL IsSet,PCLOAD_CONFIG pConfig)
{
	int 		CancelStatus;
	BOOL		RetVal;

	if(!sIsInitReady)
		ZFY_ConfConfigDataBaseInit();
	PTHREAD_MUTEX_SAFE_LOCK(sLocalMutex,CancelStatus);
	if(IsSet)
	{
		sSysConfigDatabase.CloadConfig=*pConfig;
		RetVal=ZFY_ConfSaveCurrConfig();
	}
	else
	{
		*pConfig=sSysConfigDatabase.CloadConfig;
		RetVal=TRUE;
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
	return RetVal;
}

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
extern BOOL ZFY_ConfFtpConfig(BOOL IsSet,PFTP_CONFIG pConfig)
{
	int 		CancelStatus;
	BOOL		RetVal;

	if(!sIsInitReady)
		ZFY_ConfConfigDataBaseInit();
	PTHREAD_MUTEX_SAFE_LOCK(sLocalMutex,CancelStatus);
	if(IsSet)
	{
		sSysConfigDatabase.FtpConfig=*pConfig;
		RetVal=ZFY_ConfSaveCurrConfig();
	}
	else
	{
		*pConfig=sSysConfigDatabase.FtpConfig;
		RetVal=TRUE;
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
	return RetVal;
}

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
extern BOOL ZFY_ConfApnConfig(BOOL IsSet,PAPN_CONFIG pConfig)
{
	int 		CancelStatus;
	BOOL		RetVal;

	if(!sIsInitReady)
		ZFY_ConfConfigDataBaseInit();
	PTHREAD_MUTEX_SAFE_LOCK(sLocalMutex,CancelStatus);
	if(IsSet)
	{
		sSysConfigDatabase.ApnConfig=*pConfig;
		RetVal=ZFY_ConfSaveCurrConfig();
	}
	else
	{
		*pConfig=sSysConfigDatabase.ApnConfig;
		RetVal=TRUE;
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
	return RetVal;
}
