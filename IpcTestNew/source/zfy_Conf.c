
#include <sys/vfs.h>
#include "zfy_Conf.h"
#include "LogEventServer.h"


#define SYS_CURR_SOFTWARE_VERSION							0xA2220420

#define CURR_CONFIG_DATABASE_ID								0x10090623



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
	DWORD							dwDatabaseVerify;											
	DWORD							dwDatabaseSize;												
	DWORD							dwDatabaseID;												
	SYSNETCONF						SysNetConfig;																							
	IPC_CONFIG						IpcConfig;													
	DWORD							Reserved[512];										
	DWORD							Md5Alignreverse;											
	unsigned char					Md5Check[16];												
}SYS_CONFIG_DATABASE,*PSYS_CONFIG_DATABASE;



static pthread_mutex_t					sLocalMutex=PTHREAD_MUTEX_INITIALIZER;
static BOOL								sIsInitReady=FALSE;
static SYS_CONFIG_DATABASE				sSysConfigDatabase;



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