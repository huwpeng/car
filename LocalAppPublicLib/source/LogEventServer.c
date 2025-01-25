/*
 ***************************************************************************************************************************
 ********************************************************************************************************************************
 *			PROGRAM MODULE
 *
 *			$Workfile:		LogEventServer.c	$
 *			$Revision:		1.0					$
 *			$Author:		LJJ					$
 *			$Date:			2006/10/09			$
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
 *	实现日志事务服务
 *
*/

/*
 *********************************************************************************************************************************
 * Rev 1.0   2006/10/09   
 * Initial revision.   
*/

/*
 **************************************************************************************************************************************
 * INCLUDE FILE.
 **************************************************************************************************************************************
*/

#include <stddef.h>
#include <stdarg.h>
//#include <iconv.h>

#include "sqlite3.h"
#include "LogEventServer.h"


/*
 ***********************************************************************************************************************************************
 * 常数定义
 ************************************************************************************************************************************************
*/


#define PRINTF_LOCAL_BUF_SIZE			2048
#define SYS_LOG_FILE_SIZE_MAX			(1024*1024)
#define SYS_LOG_DB_AUTO_SCAN_TIMER_S	3600
#define DATA_BASE_API_BUSY_TIMEOUT_MS	3000
#define SYS_LOG_DB_FULL_PATH			"/config/syslog.db"


/*
 ******************************************************************************************************************************************************
 类型定义
 ***************************************************************************************************************************************
*/


/*
 ***********************************************************************************************************************************************
 * 函数原型
 ************************************************************************************************************************************************
*/


/*
 ****************************************************************************************************************************************************************
 * 全局变量
 *****************************************************************************************************************************************************************
*/


static int					sLocalFd=STD_INVALID_HANDLE;
static FILE*				sLocalLog=NULL;
static BOOL					sIsAddrReady=FALSE;
static BOOL					sIsStdOut=FALSE;
static BOOL					sIsEnableLogFile=FALSE;
static PLES_CALLBACK		spCallBackFunc=NULL;
static void					*spCallBackContext=NULL;
static unsigned int			sOutLevelLimit=LOG_EVENT_LEVEL_OUT_LIMIT_DEF;
static unsigned int			sSaveLevelLimit=LOG_EVENT_LEVEL_SAVE_LIMIT_DEF;
static unsigned int			sSaveDays=LOG_SAVE_DAYS_DEF;
static pthread_mutex_t		sLocalMutex=PTHREAD_MUTEX_INITIALIZER;
static struct sockaddr_in	sDebugNetAddr;
static char					sSysLogFilePath[1024];
static char					sSysFsuId[128];


/*
 ****************************************************************************************************************************************************
 * 函数定义
 *****************************************************************************************************************************************************
*/


/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_LES_NoPrintf
 *功能描述: 忽略调试输出哑函数
 *输入描述: 标准可变输入
 *输出描述: 无
 *返回描述: 无
 *作者日期: ZCQ/2007/01/26
 *全局声明: 无
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
 */
extern int ZFY_LES_NoPrintf(const char *format, ...)
{
	return 0;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_LES_ConfigDebugServerAddr
 *功能描述: 配置调试接收服务器地址
 *输入描述: 调试接收服务器地址以及端口
 *输出描述: 无
 *返回描述: 无
 *作者日期: ZCQ/2008/03/24
 *全局声明: sLocalMutex,sDebugNetAddr,sIsAddrReady,sLocalFd
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
 */
extern void ZFY_LES_ConfigDebugServerAddr(DWORD dwServerIP,WORD wServerPort)
{
	int CancelStatus;

	PTHREAD_MUTEX_SAFE_LOCK(sLocalMutex,CancelStatus);
	memset(&sDebugNetAddr,0,sizeof(sDebugNetAddr));
	sDebugNetAddr.sin_family=AF_INET;
	sDebugNetAddr.sin_addr.s_addr=htonl(dwServerIP);
	sDebugNetAddr.sin_port=htons(wServerPort);
	sIsAddrReady=TRUE;
	if(sLocalFd!=STD_INVALID_HANDLE)
	{
		close(sLocalFd);
		sLocalFd=STD_INVALID_HANDLE;
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_LES_UdpPrintf
 *功能描述: 调试信息UDP模式格式化输出
 *输入描述: 格式化输出标准可变输入
 *输出描述: 无
 *返回描述: 实际输出字节数
 *作者日期: ZCQ/2013/01/22
 *全局声明: sLocalFd，sLocalMutex,sDebugNetAddr,sIsAddrReady,sSysLogFilePath
 *特殊说明: 自动初始化全局变量；各种输出方式输出优先级：首先回调函数，然后依次是网络输出、日志文件输出、标准输出；同一时间最多只有一种输出方式工作
 ************************************************************************************************************************************************************************       
 */
extern int ZFY_LES_UdpPrintf(const char *format,...)
{
	int		CancelStatus,RetVal=0;
	char	LocalBuf[PRINTF_LOCAL_BUF_SIZE+1]={0};

	if(sLocalFd==STD_INVALID_HANDLE)
	{
		PTHREAD_MUTEX_SAFE_LOCK(sLocalMutex,CancelStatus);
		if(sLocalFd==STD_INVALID_HANDLE)
		{
			int Fd;

			Fd=socket(AF_INET,SOCK_DGRAM,0);
			if(Fd!=STD_INVALID_HANDLE)
			{
				int optval=1;
				
				if(setsockopt(Fd,SOL_SOCKET,SO_BROADCAST,&optval,sizeof(optval))!=STD_FAILURE)
				{
					sLocalFd=Fd;
					if(!sIsAddrReady)
					{
						memset(&sDebugNetAddr,0,sizeof(sDebugNetAddr));
						sDebugNetAddr.sin_family=AF_INET;
						sDebugNetAddr.sin_addr.s_addr=htonl(INADDR_BROADCAST);
						sDebugNetAddr.sin_port=htons(UDP_PRINTF_DEFAULT_PORT);
						sIsAddrReady=TRUE;
					}
				}
				else
				{
					close(Fd);
				}
			}
		}
		PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
	}
	if(sLocalFd!=STD_INVALID_HANDLE)
	{
		char	*pBuf=NULL;
		int		Len;
		va_list	Args;

		pBuf=LocalBuf;
		va_start(Args,format);
		Len=vsnprintf(pBuf,PRINTF_LOCAL_BUF_SIZE,format,Args);
		if(Len>0)
		{
			struct sockaddr_in	RemoteAddr;

			PTHREAD_MUTEX_SAFE_LOCK(sLocalMutex,CancelStatus);
			RemoteAddr=sDebugNetAddr;
			if(NULL!=spCallBackFunc)
				RetVal=(*spCallBackFunc)(pBuf,Len,spCallBackContext);
			else if(sDebugNetAddr.sin_addr.s_addr!=0 && sDebugNetAddr.sin_port!=0)
			{
				if(sLocalFd!=STD_INVALID_HANDLE)
					RetVal=sendto(sLocalFd,pBuf,Len,0,(struct sockaddr *)&RemoteAddr,sizeof(RemoteAddr));
			}
			else if(sSysLogFilePath[0]!=0)
			{
				char	CurrLogFileName[1024];
				char	LastLogFileName[1024];

				memset(CurrLogFileName,0,sizeof(CurrLogFileName));
				memset(LastLogFileName,0,sizeof(LastLogFileName));
				if(sSysLogFilePath[strlen(sSysLogFilePath)-1]=='/')
				{
					snprintf(CurrLogFileName,sizeof(CurrLogFileName)-1,	"%s%s",sSysLogFilePath,"LogFile.log");
					snprintf(LastLogFileName,sizeof(LastLogFileName)-1,"%s%s",sSysLogFilePath,"LogFile_old.log");
				}
				else
				{
					snprintf(CurrLogFileName,sizeof(CurrLogFileName)-1,	"%s/%s",sSysLogFilePath,"LogFile.log");
					snprintf(LastLogFileName,sizeof(LastLogFileName)-1,"%s/%s",sSysLogFilePath,"LogFile_old.log");
				}
				if(sLocalLog==NULL)
					sLocalLog=fopen(CurrLogFileName,"ab+");
				if(sLocalLog!=NULL)
				{
					long	FileSize=ftell(sLocalLog);

					if(FileSize==-1)
					{
						printf("获取当前系统日志文件大小失败...\r\n");
						fclose(sLocalLog);
						sLocalLog=NULL;
					}
					else if(FileSize>SYS_LOG_FILE_SIZE_MAX)
					{
						remove(LastLogFileName);
						RetVal=fwrite(pBuf,1,Len,sLocalLog);
						fflush(sLocalLog);
						fclose(sLocalLog);
						sLocalLog=NULL;
						rename(CurrLogFileName,LastLogFileName);
					}
					else
					{
						RetVal=fwrite(pBuf,1,Len,sLocalLog);
						fflush(sLocalLog);
					}
				}
			}
			else if(sIsStdOut)
			{
				pBuf[Len]=0;
				RetVal=printf("%s",pBuf);
			}
			PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
		}
		va_end(Args);
	}
	return RetVal;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_LES_LogPrintf
 *功能描述: 系统日志格式化输出
 *输入描述: 日志产生属主名、输出级别、标准格式化输出模板、可变输出变量
 *输出描述: 无
 *返回描述:	实际输出字节数
 *作者日期: ZCQ/2015/08/01
 *全局声明: 无
 *特殊说明: 自动初始化全局变量；输出级别值越小级别越高；同时支持网络输出与数据库存储；自动维护持久存储日志生命周期。
 ************************************************************************************************************************************************************************       
 */
extern int ZFY_LES_LogPrintf(const char *pOwnerName,unsigned int Level,const char *pFormat,...)
{
	int		CancelStatus,RetVal=0;
	char	LocalBuf[PRINTF_LOCAL_BUF_SIZE+1];

	if(Level>sOutLevelLimit && Level>sSaveLevelLimit)
		return 0;
	memset(LocalBuf,0,sizeof(LocalBuf));
	if(NULL==pOwnerName)
		pOwnerName=LOG_EVENT_OWNER_NAME_DEF;
	if(sLocalFd==STD_INVALID_HANDLE)
	{
		PTHREAD_MUTEX_SAFE_LOCK(sLocalMutex,CancelStatus);
		if(sLocalFd==STD_INVALID_HANDLE)
		{
			int Fd;

			Fd=socket(AF_INET,SOCK_DGRAM,0);
			if(Fd!=STD_INVALID_HANDLE)
			{
				int optval=1;
				
				if(setsockopt(Fd,SOL_SOCKET,SO_BROADCAST,&optval,sizeof(optval))!=STD_FAILURE)
				{
					sLocalFd=Fd;
					if(!sIsAddrReady)
					{
						memset(&sDebugNetAddr,0,sizeof(sDebugNetAddr));
						sDebugNetAddr.sin_family=AF_INET;
						sDebugNetAddr.sin_addr.s_addr=htonl(INADDR_BROADCAST);
						sDebugNetAddr.sin_port=htons(UDP_PRINTF_DEFAULT_PORT);
						sIsAddrReady=TRUE;
					}
				}
				else
				{
					close(Fd);
				}
			}
		}
		PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
	}
	if(sLocalFd!=STD_INVALID_HANDLE)
	{
		char		*pBuf=NULL;
		time_t		CurrTime;
		struct	tm	SysLocalTime;
		char		TimeStr[128]={0};	
		int			Len,Size;
		va_list		Args;

		pBuf=LocalBuf;
		va_start(Args,pFormat);
		strncpy(pBuf,pOwnerName,PRINTF_LOCAL_BUF_SIZE);
		Size=strlen(pBuf);
		pBuf += Size;
		Size=PRINTF_LOCAL_BUF_SIZE-Size;
		memset(&SysLocalTime,0,sizeof(SysLocalTime));
		CurrTime=time(NULL);
		localtime_r(&CurrTime,&SysLocalTime);
		sprintf(TimeStr,"%.4u-%.2u-%.2u %.2u:%.2u:%.2u",SysLocalTime.tm_year+1900,SysLocalTime.tm_mon+1,SysLocalTime.tm_mday
			,SysLocalTime.tm_hour,SysLocalTime.tm_min,SysLocalTime.tm_sec);
		Len=snprintf(pBuf,Size,"[%d](%s): ",Level,TimeStr);
		pBuf += Len;
		Size -= Len;
		Len=vsnprintf(pBuf,Size,pFormat,Args);
		if(Len>0)
		{
			struct sockaddr_in	RemoteAddr;

			printf("%s\r\n",pBuf);
			PTHREAD_MUTEX_SAFE_LOCK(sLocalMutex,CancelStatus);
			RemoteAddr=sDebugNetAddr;
			if(sDebugNetAddr.sin_addr.s_addr!=0 && sDebugNetAddr.sin_port!=0)
			{
				if(sLocalFd!=STD_INVALID_HANDLE && Level<=sOutLevelLimit)
					RetVal=sendto(sLocalFd,LocalBuf,Len+(pBuf-LocalBuf),0,(struct sockaddr *)&RemoteAddr,sizeof(RemoteAddr));
				if(ntohs(sDebugNetAddr.sin_port)==51111)
					printf("%s",pBuf);
			}
			if(Level<=sSaveLevelLimit)
			{
				if(sIsEnableLogFile && sSysLogFilePath[0]!=0)
				{
					static FILE *pLogFile=NULL;
					static char LastLogFileName[256];
					static char	LogFilePathName[1024];
					char		CurrLogFileName[256]={0};

					snprintf(CurrLogFileName,sizeof(CurrLogFileName)-1,"%.4u%.2u%.2u",SysLocalTime.tm_year+1900,SysLocalTime.tm_mon+1,SysLocalTime.tm_mday);
					if(NULL==pLogFile || LastLogFileName[0]==0)
					{
						if(LastLogFileName[0]==0)
							strcpy(LastLogFileName,CurrLogFileName);
						if(NULL==pLogFile)
						{
							if(sSysLogFilePath[strlen(sSysLogFilePath)-1]=='/')
								snprintf(LogFilePathName,sizeof(LogFilePathName)-1,	"%s%s_%s.log",sSysLogFilePath,sSysFsuId,CurrLogFileName);
							else
								snprintf(LogFilePathName,sizeof(LogFilePathName)-1,	"%s/%s_%s.log",sSysLogFilePath,sSysFsuId,CurrLogFileName);
							if((pLogFile=fopen(LogFilePathName,"ab+"))==NULL)
								pLogFile=fopen(LogFilePathName,"wb+");
							if(NULL!=pLogFile)
								strcpy(LastLogFileName,CurrLogFileName);	
						}
					}
					if(NULL!=pLogFile)
					{
						if(strcmp(LastLogFileName,CurrLogFileName)!=0)
						{
							fflush(pLogFile);
							fsync(fileno(pLogFile));
							fclose(pLogFile);
							pLogFile=NULL;
							if(CurrTime>(time_t)sSaveDays*24*3600)
							{
								int				FileNameLen;
								DIR 			*dir;
								struct dirent 	*s_dir;
								struct  stat 	FileStat;

								if((dir=opendir(sSysLogFilePath))!=NULL)
								{
									while((s_dir=readdir(dir))!=NULL)
									{
										FileNameLen=strlen(s_dir->d_name);
										if(FileNameLen>4 && strcmp(s_dir->d_name+FileNameLen-4,".log")==0)
										{
											if(sSysLogFilePath[strlen(sSysLogFilePath)-1]=='/')
												snprintf(LogFilePathName,sizeof(LogFilePathName)-1,	"%s%s",sSysLogFilePath,s_dir->d_name);
											else
												snprintf(LogFilePathName,sizeof(LogFilePathName)-1,	"%s/%s",sSysLogFilePath,s_dir->d_name);
											stat(LogFilePathName,&FileStat);
											if(!S_ISDIR(FileStat.st_mode) && (time_t)FileStat.st_mtime<CurrTime-(time_t)sSaveDays*24*3600)
												remove(LogFilePathName);
										}
									}
									closedir(dir);
								}
							}
							if(sSysLogFilePath[strlen(sSysLogFilePath)-1]=='/')
								snprintf(LogFilePathName,sizeof(LogFilePathName)-1,	"%s%s_%s.log",sSysLogFilePath,sSysFsuId,CurrLogFileName);
							else
								snprintf(LogFilePathName,sizeof(LogFilePathName)-1,	"%s/%s_%s.log",sSysLogFilePath,sSysFsuId,CurrLogFileName);
							if((pLogFile=fopen(LogFilePathName,"ab+"))==NULL)
								pLogFile=fopen(LogFilePathName,"wb+");
							if(NULL!=pLogFile)
								strcpy(LastLogFileName,CurrLogFileName);
						}
						if(NULL!=pLogFile)
							fprintf(pLogFile,"[%s][%s][%d]:%s",TimeStr,pOwnerName,Level,pBuf);
					}
				}
				else
				{
					
				}
			}
			PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
		}
		va_end(Args);
	}
	return RetVal;	
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_LES_ConfigCallback
 *功能描述: 设置日志输出回调函数
 *输入描述: 标准输出标记、回调函数、回调函数环境
 *输出描述: 可选输出原回调函数环境
 *返回描述: 原回调函数
 *作者日期: ZCQ/2009/05/24
 *全局声明: sLocalMutex,sIsStdOut,spCallBackFunc,spCallBackContext
 *特殊说明: 默认无回调函数且禁用标准输出
 ************************************************************************************************************************************************************************       
 */
extern PLES_CALLBACK ZFY_LES_ConfigCallback(BOOL IsStdOut,PLES_CALLBACK pCallback,void *pContext,void **ppOldContext)
{
	int				CancelStatus;
	PLES_CALLBACK	OldCallback=NULL;
	
	PTHREAD_MUTEX_SAFE_LOCK(sLocalMutex,CancelStatus);
	OldCallback=spCallBackFunc;
	if(NULL!=ppOldContext)
		*ppOldContext=spCallBackContext;
	sIsStdOut=IsStdOut;
	spCallBackFunc=pCallback;
	spCallBackContext=pContext;
	PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
	return OldCallback;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_LES_InitLogPath,ZFY_LES_InitLogPathEx
 *功能描述: 设置设备日志文件存盘路径
 *输入描述: 设备日志文件存盘路径,设备标识FSUID,日志文件使能标记
 *输出描述: 无
 *返回描述: 无
 *作者日期: ZCQ/2016/08/03
 *全局声明: sLocalMutex,sSysLogFilePath,sIsEnableLogFile,sSysFsuId
 *特殊说明: 系统默认路径空
 ************************************************************************************************************************************************************************       
 */
extern void ZFY_LES_InitLogPathEx(const char *pPath,const char *pFsuID,BOOL IsEnableLogFile)
{
	int	CancelStatus;
	
	PTHREAD_MUTEX_SAFE_LOCK(sLocalMutex,CancelStatus);
	if(pPath!=NULL && pPath[0]!=0)
		strncpy(sSysLogFilePath,pPath,sizeof(sSysLogFilePath)-1);
	if(pFsuID!=NULL && pFsuID[0]!=0)
		strncpy(sSysFsuId,pFsuID,sizeof(sSysFsuId)-1);
	sIsEnableLogFile=IsEnableLogFile;
	PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
}
extern void ZFY_LES_InitLogPath(const char *pPath)
{
	ZFY_LES_InitLogPathEx(pPath,NULL,FALSE);
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_LES_SetLogConfig
 *功能描述: 设置日志服务参数
 *输入描述: 日志输出限制级别、日志存储限制级别、日志保存天数
 *输出描述: 无
 *返回描述: 无
 *作者日期: ZCQ/2015/08/01
 *全局声明: 无
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern void ZFY_LES_SetLogConfig(unsigned int OutLevelLimit,unsigned int SaveLevelLimit,unsigned int SaveDays)
{
	int	CancelStatus;

	if(OutLevelLimit>LOG_EVENT_LEVEL_DEBUG)
		OutLevelLimit=LOG_EVENT_LEVEL_DEBUG;
	if(SaveLevelLimit>LOG_EVENT_LEVEL_DEBUG)
		SaveLevelLimit=LOG_EVENT_LEVEL_DEBUG;
	if(SaveDays>LOG_SAVE_DAYS_MAX)
		SaveDays=LOG_SAVE_DAYS_MAX;
	if(SaveDays<LOG_SAVE_DAYS_MIN)
		SaveDays=LOG_SAVE_DAYS_MIN;
	PTHREAD_MUTEX_SAFE_LOCK(sLocalMutex,CancelStatus);
	sOutLevelLimit=OutLevelLimit;
	sSaveLevelLimit=SaveLevelLimit;
	sSaveDays=SaveDays;
	PTHREAD_MUTEX_SAFE_UNLOCK(sLocalMutex,CancelStatus);
}




/*
 ************************************************************************************************************************************************************************     
 *函数名称:
 *功能描述:
 *输入描述:
 *输出描述:
 *返回描述:
 *作者日期:
 *全局声明:
 *特殊说明:
 ************************************************************************************************************************************************************************       
 */




/******************************************LogEventServer.c File End******************************************************************************************/



