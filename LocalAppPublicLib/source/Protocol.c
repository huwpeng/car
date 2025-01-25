/*
 ***************************************************************************************************************************
 ********************************************************************************************************************************
 *			PROGRAM MODULE
 *
 *			$Workfile:		Protocol.c			$
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
 *	实现兼容的VS2008协议
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

#include "Protocol.h"
#include "NetServer.h"
#include "LogEventServer.h"



/*
 ***********************************************************************************************************************************************
 * 常数定义
 ************************************************************************************************************************************************
*/


#define BYTE_ORDER_LITTLE_ENDIAN						1
#define BYTE_ORDER_BIG_ENDIAN							2
#define SYS_CURR_BYTE_ORDER								BYTE_ORDER_LITTLE_ENDIAN		

#define SWAP_DWORD(x)									x=JSYA_SwapDword(x)
#define SWAP_WORD(x)									x=JSYA_SwapWord(x)

#define PROTOCOL_COMM_QUEUE_SIZE						128
#define PROTOCOL_USER_TOTAL								(PROTOCOL_MAX_USER_NUM+3)
#define PROTOCOL_QUEUE_NODE_SIZE						PROTOCOL_PACKET_MAX_SIZE


#define PROTOCOL_SERVER_MAX_REPORT_TIME					30

#define	PROTOCOL_API_LOG_NAME							"PROTOCOL_API"
#define	PROTOCOL_API_LOG(level,format,...)				ZFY_LES_LogPrintf(PROTOCOL_API_LOG_NAME,level,format,##__VA_ARGS__)


/*
 ******************************************************************************************************************************************************
 类型定义
 ***************************************************************************************************************************************
*/

typedef struct tPROTOCOL_COMM_QUEUE_NODE
{
	void	*pBuf;
	DWORD	Len;
	DWORD	Owner;
	DWORD	Option;
	DWORD	dwReserved;
	WORD	wReserved;
	BYTE	bReserved1;
	BYTE	bReserved2;
}PROTOCOL_COMM_QUEUE_NODE,*PPROTOCOL_COMM_QUEUE_NODE;
typedef struct tPROTOCOL_USER_STATUS
{
	BOOL	IsEnable;
	DWORD	Tail;
	DWORD	Count;
	DWORD	OverCount;
}PROTOCOL_USER_STATUS,*PPROTOCOL_USER_STATUS;
typedef struct tPROTOCOL_COMM_QUEUE
{
	DWORD						Head;
	DWORD						ActiveUserCount;
	DWORD						OverCount;
	BOOL						IsCondInited;
	BOOL						IsReady;
	pthread_cond_t				ReadyCond;
	pthread_cond_t				WriteCond;
	PROTOCOL_USER_STATUS		UserTab[PROTOCOL_USER_TOTAL];
	PROTOCOL_COMM_QUEUE_NODE	Queue[PROTOCOL_COMM_QUEUE_SIZE];
}PROTOCOL_COMM_QUEUE,*PPROTOCOL_COMM_QUEUE;


typedef struct tPROTOCOL_SERVER
{
	BOOL		IsInitReady;
	BOOL		IsReqQuit;
	pthread_t	OwnerThread;
	pthread_t	ManagerThread;
	__pid_t		OwnerPID;
}PROTOCOL_SERVER,*PPROTOCOL_SERVER;


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


static pthread_mutex_t			sProtocolQueueMutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t			sProtocolMutex=PTHREAD_MUTEX_INITIALIZER;
static PROTOCOL_COMM_QUEUE		sProtocolQueue[PROTOCOL_COMM_QUEUE_MAX_NUM];
static PROTOCOL_SERVER			sProtocolServer;
static DWORD					sProtocolQueueCurrMaxNum=PROTOCOL_COMM_QUEUE_DEFAULT_NUM;

/*
 ****************************************************************************************************************************************************
 * 函数定义
 *****************************************************************************************************************************************************
*/


/*
 ************************************************************************************************************************************************************************     
 *函数名称: ProtocolQueueUnInit
 *功能描述: 协议队列反初始化
 *输入描述: 锁定标志
 *输出描述: 无
 *返回描述: 无
 *作者日期: ZCQ/2007/08/09
 *全局声明: sProtocolQueue,sProtocolQueueMutex
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
 */
static void ProtocolQueueUnInit(BOOL IsLock)
{
	int i,j,OldStatus;

	if(IsLock)PTHREAD_MUTEX_SAFE_LOCK(sProtocolQueueMutex,OldStatus);
	for(i=0;i<PROTOCOL_COMM_QUEUE_MAX_NUM;i++)
	{
		if(sProtocolQueue[i].IsCondInited)
		{
			if(IsLock)
			{
				pthread_cond_broadcast(&sProtocolQueue[i].ReadyCond);
				pthread_cond_broadcast(&sProtocolQueue[i].WriteCond);
			}
			pthread_cond_destroy(&sProtocolQueue[i].ReadyCond);
			pthread_cond_destroy(&sProtocolQueue[i].WriteCond);
			sProtocolQueue[i].IsCondInited=FALSE;
		}
		for(j=0;j<PROTOCOL_COMM_QUEUE_SIZE;j++)
		{
			if(NULL!=sProtocolQueue[i].Queue[j].pBuf)
			{
				free(sProtocolQueue[i].Queue[j].pBuf);
				sProtocolQueue[i].Queue[j].pBuf=NULL;
			}
		}
		sProtocolQueue[i].IsReady=FALSE;
	}
	if(IsLock)PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,OldStatus);
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ProtocolQueueInit
 *功能描述: 协议队列初始化
 *输入描述: 无
 *输出描述: 无
 *返回描述: 成功(TRUE)/失败(FALSE)
 *作者日期: ZCQ/2007/08/09
 *全局声明: sProtocolQueue,sProtocolQueueMutex,sProtocolQueueCurrMaxNum
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
 */
static BOOL ProtocolQueueInit(void)
{
	int i,j,OldStatus;
	
	PTHREAD_MUTEX_SAFE_LOCK(sProtocolQueueMutex,OldStatus);
	memset(&sProtocolQueue,0,sizeof(sProtocolQueue));
	for(i=0;i<(int)sProtocolQueueCurrMaxNum;i++)
	{
		pthread_cond_init(&sProtocolQueue[i].ReadyCond,NULL);
		pthread_cond_init(&sProtocolQueue[i].WriteCond,NULL);
		sProtocolQueue[i].IsCondInited=TRUE;
		for(j=0;j<PROTOCOL_COMM_QUEUE_SIZE;j++)
		{
			sProtocolQueue[i].Queue[j].pBuf=malloc(PROTOCOL_QUEUE_NODE_SIZE);
			if(NULL==sProtocolQueue[i].Queue[j].pBuf)
			{
				ProtocolQueueUnInit(FALSE);
				PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,OldStatus);
				return FALSE;
			}
		}
		sProtocolQueue[i].IsReady=TRUE;
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,OldStatus);
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ProtocolServerThread
 *功能描述: 协议队列状态监视服务线程
 *输入描述: 无
 *输出描述: 无
 *返回描述: 无
 *作者日期: ZCQ/2007/02/02
 *全局声明: sProtocolQueue
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
 */
static void *ProtocolServerThread(void *pArg)
{
	int		i,ReportTime;

	for(ReportTime=0;;)
	{
		if(sProtocolServer.IsReqQuit)
		{
			sProtocolServer.IsReqQuit=FALSE;
			break;
		}
		pthread_testcancel();


		sleep(1);
		if(ReportTime++>=PROTOCOL_SERVER_MAX_REPORT_TIME)
		{
			ReportTime=0;
			for(i=0;i<PROTOCOL_COMM_QUEUE_MAX_NUM;i++)
			{
				if(sProtocolQueue[i].ActiveUserCount>0)
				{
					PROTOCOL_API_LOG(LOG_EVENT_LEVEL_INFO,"系统协议处理队列当前状态：QID=%d,UC=%u,OV=%u...\r\n",i,(unsigned int)sProtocolQueue[i].ActiveUserCount
						,(unsigned int)sProtocolQueue[i].OverCount);
				}
			}
		}
	}

	return NULL;
}








/*
 ************************************************************************************************************************************************************************     
 *函数名称: JSYA_ProtocolServerInit
 *功能描述: 协议服务初始化
 *输入描述: 协议队列数目
 *输出描述: 无
 *返回描述: 成功(TRUE)/失败(FALSE)
 *作者日期: ZCQ/2007/08/09
 *全局声明: sProtocolServer,sProtocolMutex,sProtocolQueueCurrMaxNum
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
 */
extern BOOL JSYA_ProtocolServerInit(DWORD CurrProtocolQueueNum)
{
	int					OldStatus;
	pthread_attr_t		ThreadAttr;
	struct sched_param	ThreadSchedParam;

	PTHREAD_MUTEX_SAFE_LOCK(sProtocolMutex,OldStatus);
	if(!sProtocolServer.IsInitReady)
	{
		sProtocolServer.OwnerPID=getpid();
		sProtocolServer.OwnerThread=pthread_self();
		sProtocolServer.IsReqQuit=FALSE;
		sProtocolQueueCurrMaxNum=CurrProtocolQueueNum;
		if(sProtocolQueueCurrMaxNum<PROTOCOL_COMM_QUEUE_DEFAULT_NUM)
			sProtocolQueueCurrMaxNum=PROTOCOL_COMM_QUEUE_DEFAULT_NUM;
		if(sProtocolQueueCurrMaxNum>PROTOCOL_COMM_QUEUE_MAX_NUM)
			sProtocolQueueCurrMaxNum=PROTOCOL_COMM_QUEUE_MAX_NUM;
		if(!ProtocolQueueInit())
		{
			PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
			PROTOCOL_API_LOG(LOG_EVENT_LEVEL_ERR,"系统协议缓冲队列初始化失败...\r\n");
			return FALSE;
		}

		pthread_attr_init(&ThreadAttr);
		pthread_attr_setdetachstate(&ThreadAttr,PTHREAD_CREATE_JOINABLE);
		pthread_attr_setinheritsched(&ThreadAttr,PTHREAD_EXPLICIT_SCHED);
		pthread_attr_setschedpolicy(&ThreadAttr,SCHED_OTHER);
		ThreadSchedParam.sched_priority=PTHREAD_DEFAULT_SCHED_PRIORITY+SYS_APP_SCHED_PRIORITY
			+APP_NORMAL_SCHED_PRIORITY+SYNC_APP_SCHED_PRIORITY+BACKGROUND_APP_SCHED_PRIORITY+CMD_SCHED_PRIORITY;
		pthread_attr_setschedparam(&ThreadAttr,&ThreadSchedParam);
		if(pthread_create(&sProtocolServer.ManagerThread,&ThreadAttr,ProtocolServerThread,NULL)!=STD_SUCCESS)
		{
			ProtocolQueueUnInit(TRUE);
			PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
			pthread_attr_destroy(&ThreadAttr);
			PROTOCOL_API_LOG(LOG_EVENT_LEVEL_DEBUG,"系统协议服务线程创建失败(%s)...\r\n",strerror(errno));
			return FALSE;
		}
		pthread_attr_destroy(&ThreadAttr);
		sProtocolServer.IsInitReady=TRUE;
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: JSYA_ProtocolQueueWrite
 *功能描述: 协议队列输入
 *输入描述: 队列ID、数据及其大小、所有者标识、自动溢出标志、阻塞超时毫秒数、可选输入
 *输出描述: 无
 *返回描述: 成功(TRUE)/失败(FALSE)
 *作者日期: ZCQ/2007/08/09
 *全局声明: sProtocolServer,sProtocolMutex,sProtocolQueue,sProtocolQueueMutex,sProtocolQueueCurrMaxNum
 *特殊说明: 若IsAutoOver=TRUE,则忽略pTimeOutMS;pTimeOutMS为NULL表示无限阻塞
 *			，*pTimeOutMS为0表示不阻塞,阻塞超时返回失败；阻塞时与所有活动用户同步；
			可选输入目前暂时忽略,请设置NULL;
 ************************************************************************************************************************************************************************       
 */
extern BOOL JSYA_ProtocolQueueWrite(DWORD QueueID,const void *pBuf,DWORD dwSize,DWORD OwnerID,BOOL IsAutoOver,const DWORD *pTimeOutMS,const void *pOption)
{
	int						i,OldStatus,CancelStatus;
	DWORD					MaxCount;
	struct timespec			DestTime;
	PPROTOCOL_COMM_QUEUE	pQueue;


	if(NULL==pBuf || dwSize==0)
		return FALSE;
	
	if(QueueID>=sProtocolQueueCurrMaxNum || dwSize>PROTOCOL_QUEUE_NODE_SIZE)
		return FALSE;
	
	if(NULL!=pTimeOutMS && *pTimeOutMS!=0)
	{
		clock_gettime(CLOCK_REALTIME,&DestTime);
		DestTime.tv_nsec += (*pTimeOutMS%1000)*1000000;
		DestTime.tv_sec += (*pTimeOutMS/1000);
		if(DestTime.tv_nsec>=1000000000)
		{
			DestTime.tv_nsec -= 1000000000;
			DestTime.tv_sec++;
		}
	}
	
	for(;;)
	{
		PTHREAD_MUTEX_SAFE_LOCK(sProtocolMutex,OldStatus);
		if(!sProtocolServer.IsInitReady)
		{
			PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
			return FALSE;
		}
		PTHREAD_MUTEX_SAFE_LOCK(sProtocolQueueMutex,CancelStatus);
		pQueue=&sProtocolQueue[QueueID];
		if(!pQueue->IsReady)
		{
			PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,CancelStatus);
			PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
			return FALSE;
		}
		for(i=0,MaxCount=0;i<PROTOCOL_USER_TOTAL;i++)
		{
			if(pQueue->UserTab[i].IsEnable && pQueue->UserTab[i].Count>MaxCount)
				MaxCount=pQueue->UserTab[i].Count;
		}
		if(MaxCount+1>=PROTOCOL_COMM_QUEUE_SIZE)
		{
			if(IsAutoOver)
			{
				memcpy(pQueue->Queue[pQueue->Head].pBuf,pBuf,dwSize);
				pQueue->Queue[pQueue->Head].Len=dwSize;
				pQueue->Queue[pQueue->Head].Owner=OwnerID;

				pQueue->Head=(pQueue->Head+1)%PROTOCOL_COMM_QUEUE_SIZE;
				for(i=0;i<PROTOCOL_USER_TOTAL;i++)
				{
					if(pQueue->UserTab[i].IsEnable)
					{
						pQueue->UserTab[i].Count++;
						if(pQueue->UserTab[i].Tail==pQueue->Head)
						{
							pQueue->UserTab[i].Count=0;
							pQueue->UserTab[i].OverCount++;
							pQueue->OverCount++;
						}
					}
				}
				pthread_cond_broadcast(&pQueue->ReadyCond);
				break;
			}
			else if(NULL==pTimeOutMS)
			{
				PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,CancelStatus);
				if(pthread_cond_wait(&pQueue->WriteCond,&sProtocolQueueMutex)!=STD_SUCCESS)
				{
					PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,OldStatus);
					
					return FALSE;
				}
				PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,OldStatus);
				continue;
			}
			else if(*pTimeOutMS==0)
			{
				PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,CancelStatus);
				PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
				
				return FALSE;
			}
			else
			{
				PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,CancelStatus);
				if(pthread_cond_timedwait(&pQueue->WriteCond,&sProtocolQueueMutex,&DestTime)!=STD_SUCCESS)
				{
					PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,OldStatus);
					
					return FALSE;
				}
				
				PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,OldStatus);
				continue;
			}
		}
		else
		{
			memcpy(pQueue->Queue[pQueue->Head].pBuf,pBuf,dwSize);
			pQueue->Queue[pQueue->Head].Len=dwSize;
			pQueue->Queue[pQueue->Head].Owner=OwnerID;

			pQueue->Head=(pQueue->Head+1)%PROTOCOL_COMM_QUEUE_SIZE;
			for(i=0;i<PROTOCOL_USER_TOTAL;i++)
			{
				if(pQueue->UserTab[i].IsEnable)
				{
					pQueue->UserTab[i].Count++;
					if(pQueue->UserTab[i].Tail==pQueue->Head)
					{
						pQueue->UserTab[i].Count=0;
						pQueue->UserTab[i].OverCount++;
						pQueue->OverCount++;
					}
				}
			}
			pthread_cond_broadcast(&pQueue->ReadyCond);
			break;
		}
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,CancelStatus);
	PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: JSYA_ProtocolQueueRead
 *功能描述: 协议队列读数据
 *输入描述: 队列ID、用户ID、数据输出缓存区及其大小、阻塞等待超时毫秒数、可选输出缓冲
 *输出描述: 当前数据及其长度
 *返回描述: 成功(TRUE)/失败(FALSE)
 *作者日期: ZCQ/2007/08/09
 *全局声明: sProtocolServer,sProtocolMutex,sProtocolQueue,sProtocolQueueMutex,sProtocolQueueCurrMaxNum
 *特殊说明: pTimeOutMS为NULL表示无限阻塞，*pTimeOutMS为0表示不阻塞,超时成功返回码流长度0；
 *			可选输出目前暂时忽略,请设置NULL;
 ************************************************************************************************************************************************************************       
 */
extern BOOL JSYA_ProtocolQueueRead(DWORD QueueID,DWORD UserIndex,void *pBuf,DWORD *pBufSize,const DWORD *pTimeOutMS,void *pOption)
{
	int						Ret,OldStatus,CancelStatus;
	PPROTOCOL_COMM_QUEUE	pQueue=NULL;
	struct timespec			DestTime;

	if(QueueID>=sProtocolQueueCurrMaxNum || UserIndex>=PROTOCOL_USER_TOTAL || NULL==pBuf || NULL==pBufSize)
		return FALSE;
	if(NULL!=pTimeOutMS && *pTimeOutMS!=0)
	{
		clock_gettime(CLOCK_REALTIME,&DestTime);
		DestTime.tv_nsec += (*pTimeOutMS%1000)*1000000;
		DestTime.tv_sec += (*pTimeOutMS/1000);
		if(DestTime.tv_nsec>=1000000000)
		{
			DestTime.tv_nsec -= 1000000000;
			DestTime.tv_sec++;
		}
	}
	
	for(;;)
	{
		PTHREAD_MUTEX_SAFE_LOCK(sProtocolMutex,OldStatus);
		if(!sProtocolServer.IsInitReady)
		{
			PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
			return FALSE;
		}
		PTHREAD_MUTEX_SAFE_LOCK(sProtocolQueueMutex,CancelStatus);
		pQueue=&sProtocolQueue[QueueID];
		if(!pQueue->IsReady || !pQueue->UserTab[UserIndex].IsEnable)
		{
			PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,CancelStatus);
			PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
			return FALSE;
		}
		if(pQueue->Head==pQueue->UserTab[UserIndex].Tail)
		{
			if(NULL==pTimeOutMS)
			{
				PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,CancelStatus);
				if(pthread_cond_wait(&pQueue->ReadyCond,&sProtocolQueueMutex)!=STD_SUCCESS)
				{
					PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,OldStatus);
					return FALSE;
				}
				PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,OldStatus);
				continue;
			}
			else if(*pTimeOutMS==0)
			{
				*pBufSize=0;
				break;
			}
			else
			{
				PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,CancelStatus);
				if((Ret=pthread_cond_timedwait(&pQueue->ReadyCond,&sProtocolQueueMutex,&DestTime))!=STD_SUCCESS)
				{
					PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,OldStatus);
					if(Ret==ETIMEDOUT)
					{
						*pBufSize=0;
						return TRUE;
					}
					return FALSE;
				}
				PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,OldStatus);
				continue;
			}
		}
		else
		{
			if(pQueue->Queue[pQueue->UserTab[UserIndex].Tail].Owner==PROTOCOL_OWNER_ALL
				|| pQueue->Queue[pQueue->UserTab[UserIndex].Tail].Owner==UserIndex)
			{
				if(pQueue->Queue[pQueue->UserTab[UserIndex].Tail].Len>*pBufSize)
				{
					PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,CancelStatus);
					PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
					return FALSE;
				}
				memcpy(pBuf,pQueue->Queue[pQueue->UserTab[UserIndex].Tail].pBuf
					,pQueue->Queue[pQueue->UserTab[UserIndex].Tail].Len);
				*pBufSize=pQueue->Queue[pQueue->UserTab[UserIndex].Tail].Len;

				pQueue->UserTab[UserIndex].Tail=(pQueue->UserTab[UserIndex].Tail+1)%PROTOCOL_COMM_QUEUE_SIZE;
				pQueue->UserTab[UserIndex].Count--;
				break;
			}
			else
			{
				pQueue->UserTab[UserIndex].Tail=(pQueue->UserTab[UserIndex].Tail+1)%PROTOCOL_COMM_QUEUE_SIZE;
				pQueue->UserTab[UserIndex].Count--;
				PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,CancelStatus);
				PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
				continue;
			}
		}
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,CancelStatus);
	PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: JSYA_ProtocolQueueUserCtrl
 *功能描述: 协议队列用户活动控制
 *输入描述: 队列ID、用户ID、活动标记
 *输出描述: 无
 *返回描述: 成功(TRUE)/失败(FALSE)
 *作者日期: ZCQ/2007/08/09
 *全局声明: sProtocolServer,sProtocolMutex,sProtocolQueue,sProtocolQueueMutex,sProtocolQueueCurrMaxNum
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
 */
extern BOOL JSYA_ProtocolQueueUserCtrl(DWORD QueueID,DWORD UserIndex,BOOL IsWork)
{
	int						OldStatus,CancelStatus;
	PPROTOCOL_COMM_QUEUE	pQueue=NULL;

	if(QueueID>=sProtocolQueueCurrMaxNum || UserIndex>=PROTOCOL_USER_TOTAL)
		return FALSE;
	PTHREAD_MUTEX_SAFE_LOCK(sProtocolMutex,OldStatus);
	if(!sProtocolServer.IsInitReady)
	{
		PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
		return FALSE;
	}
	PTHREAD_MUTEX_SAFE_LOCK(sProtocolQueueMutex,CancelStatus);
	pQueue=&sProtocolQueue[QueueID];
	if(!pQueue->IsReady)
	{
		PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,CancelStatus);
		PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
		return FALSE;
	}

	if(IsWork)
	{
		if(!pQueue->UserTab[UserIndex].IsEnable)
		{
			pQueue->UserTab[UserIndex].Tail=pQueue->Head;
			pQueue->UserTab[UserIndex].Count=0;
			pQueue->UserTab[UserIndex].IsEnable=TRUE;
			pQueue->ActiveUserCount++;
		}
	}
	else
	{
		if(pQueue->UserTab[UserIndex].IsEnable)
		{
			pQueue->UserTab[UserIndex].IsEnable=FALSE;
			pQueue->UserTab[UserIndex].Tail=pQueue->Head;
			pQueue->UserTab[UserIndex].Count=0;
			pQueue->ActiveUserCount--;
		}
	}

	PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolQueueMutex,CancelStatus);
	PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
	return TRUE;	
}








/*
 ************************************************************************************************************************************************************************     
 *函数名称: JSYA_SwapMmnetHeader
 *功能描述: 根据协议数据基本类型变换协议MMNET包头字节序
 *输入描述: 应用协议MMNET包头指针,协议命令头是否来自网络
 *输出描述: 匹配协议要求的字节序应用协议MMNET包头
 *返回描述: 成功(TRUE)/失败(FALSE)
 *作者日期: ZCQ/2007/02/12
 *全局声明: 无
 *特殊说明: 仅仅检查协议数据类型、长度、标识三个基本字段
 ************************************************************************************************************************************************************************       
 */
extern U32 JSYA_SwapDword(U32 val)
{
	return ((val<<24)&0xFF000000)|((val<<8)&0xFF0000)| 
		   ((val>>8)&0xFF00)|((val>>24)&0xFF);
}
extern U16 JSYA_SwapWord(U16 val)
{
	return ((val>>8)&0xFF)|((val<<8)&0xFF00);
}
extern BOOL JSYA_SwapMmnetHeader(PMMNET_HEADER pHead,BOOL IsFromNet)
{
#if SYS_CURR_BYTE_ORDER==BYTE_ORDER_LITTLE_ENDIAN
	if(pHead->nType==MMNET_I_VIDEO || pHead->nType==MMNET_I_AUDIO || pHead->nType==MMNET_I_DATA)
	{
		if(pHead->dwLength<PROTOCOL_PACKET_MIN_SIZE || pHead->dwLength>PROTOCOL_PACKET_MAX_SIZE)
			return FALSE;
		return pHead->CheckSum==TCLASS_CHECK_WORD;
	}
	if(pHead->nType!=MMNET_M_VIDEO && pHead->nType!=MMNET_M_AUDIO && pHead->nType!=MMNET_M_DATA)
		return FALSE;
#elif SYS_CURR_BYTE_ORDER==BYTE_ORDER_BIG_ENDIAN
	if(pHead->nType==MMNET_M_VIDEO || pHead->nType==MMNET_M_AUDIO || pHead->nType==MMNET_M_DATA)
	{
		if(pHead->dwLength<PROTOCOL_PACKET_MIN_SIZE || pHead->dwLength>PROTOCOL_PACKET_MAX_SIZE)
			return FALSE;
		return pHead->CheckSum==TCLASS_CHECK_WORD;
	}
	if(pHead->nType!=MMNET_I_VIDEO && pHead->nType!=MMNET_I_AUDIO && pHead->nType!=MMNET_I_DATA)
		return FALSE;
#else
#error Must Define System Current Byte Order!
#endif
	if(IsFromNet)
	{
		pHead->dwLength=SwapDword(pHead->dwLength);
		pHead->CheckSum=SwapWord(pHead->CheckSum);
	}
	if(pHead->dwLength<PROTOCOL_PACKET_MIN_SIZE || pHead->dwLength>PROTOCOL_PACKET_MAX_SIZE
		|| pHead->CheckSum!=TCLASS_CHECK_WORD)
			return FALSE;
	if(!IsFromNet)
	{
		pHead->dwLength=SwapDword(pHead->dwLength);
		pHead->CheckSum=SwapWord(pHead->CheckSum);
	}
	pHead->DestIP=SwapDword(pHead->DestIP);
	pHead->dwDevID=SwapDword(pHead->dwDevID);
	pHead->dwFormat=SwapDword(pHead->dwFormat);
	pHead->dwOpCode=SwapDword(pHead->dwOpCode);
	pHead->dwReserved=SwapDword(pHead->dwReserved);
	pHead->dwSeqNum=SwapDword(pHead->dwSeqNum);
	pHead->SrcIP=SwapDword(pHead->SrcIP);
	pHead->wReserved1=SwapWord(pHead->wReserved1);
	pHead->wReserved2=SwapWord(pHead->wReserved2);
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: JSYA_ProtocolSwapBodyByteOrder
 *功能描述: 处理协议命令体字节序转换
 *输入描述: 转换前协议命令、严格转换标志、协议命令体是否来自网络
 *输出描述: 转换后协议命令
 *返回描述: 成功(TRUE)/失败(FALSE)
 *作者日期: ZCQ/2014/09/23
 *全局声明: 无
 *特殊说明: 调用前必须保证协议头部字节序与系统当前本地字节序一致
 ************************************************************************************************************************************************************************       
 */
extern BOOL JSYA_ProtocolSwapBodyByteOrder(PCMD_PACKET pCmd,BOOL IsStrict,BOOL IsFromNet)
{
#if SYS_CURR_BYTE_ORDER==BYTE_ORDER_LITTLE_ENDIAN
	if(pCmd->Header.nType==MMNET_I_VIDEO || pCmd->Header.nType==MMNET_I_AUDIO
		|| pCmd->Header.nType==MMNET_I_DATA)
			return TRUE;
#endif
#if SYS_CURR_BYTE_ORDER==BYTE_ORDER_BIG_ENDIAN
	if(pCmd->Header.nType==MMNET_M_VIDEO || pCmd->Header.nType==MMNET_M_AUDIO
		|| pCmd->Header.nType==MMNET_M_DATA)
			return TRUE;
#endif
#if BYTE_ORDER_LITTLE_ENDIAN==BYTE_ORDER_BIG_ENDIAN
#error Must Define System Current Byte Order!
#endif

	if(pCmd->Header.nType==MMNET_I_VIDEO)
	{
		return TRUE;
	}
	else if(pCmd->Header.nType==MMNET_M_VIDEO)
	{
		return TRUE;
	}
	else if(pCmd->Header.nType==MMNET_I_AUDIO)
	{
		return TRUE;
	}
	else if(pCmd->Header.nType==MMNET_M_AUDIO)
	{
		return TRUE;
	}
	else if(pCmd->Header.nType==MMNET_I_DATA || pCmd->Header.nType==MMNET_M_DATA)
	{
		DWORD	i,j,CurrCmdLen;

		CurrCmdLen=pCmd->Header.dwLength-sizeof(pCmd->Header);
		switch(pCmd->Header.dwOpCode)
		{
		case COMM_STARTVIDEO:
		case COMM_APP_VIDEO_CONFIG:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.VideoConf))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.VideoConf.BitRate);
			SWAP_DWORD(pCmd->COMMAND.VideoConf.Format);
			SWAP_DWORD(pCmd->COMMAND.VideoConf.NetMode);
			break;
		case COMM_STOPVIDEO:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.StopVideo))
				return FALSE;
			break;
		case COMM_SETHW_SA7113:
			return FALSE;
			break;
		case COMM_SETHW_W99200:
			return FALSE;
			break;
		case COMM_GETHW_SA7113:
			return FALSE;
			break;
		case COMM_GETHW_W99200:
			break;
		case COMM_RECORD_GET_STATUS:
			if(!IsStrict)return TRUE;
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.RecordRunStatus))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.RecordRunStatus.dwDelFileCount);
			SWAP_DWORD(pCmd->COMMAND.RecordRunStatus.dwMediaErrCount);
			SWAP_DWORD(pCmd->COMMAND.RecordRunStatus.dwMediaLeftCapacityMB);
			SWAP_DWORD(pCmd->COMMAND.RecordRunStatus.dwMediaTotalCapacityMB);
			SWAP_DWORD(pCmd->COMMAND.RecordRunStatus.dwQueueOverCount);
			SWAP_DWORD(pCmd->COMMAND.RecordRunStatus.dwReserved);
			SWAP_DWORD(pCmd->COMMAND.RecordRunStatus.MediaIsValid);
			SWAP_WORD(pCmd->COMMAND.RecordRunStatus.wReserved);
			for(i=0;i<sizeof(pCmd->COMMAND.RecordRunStatus.ChTab)/sizeof(pCmd->COMMAND.RecordRunStatus.ChTab[0]);i++)
			{
				SWAP_DWORD(pCmd->COMMAND.RecordRunStatus.ChTab[i].dwCurrMode);
				SWAP_DWORD(pCmd->COMMAND.RecordRunStatus.ChTab[i].dwFileCount);
				SWAP_DWORD(pCmd->COMMAND.RecordRunStatus.ChTab[i].dwFileErrCount);
				SWAP_DWORD(pCmd->COMMAND.RecordRunStatus.ChTab[i].dwReserved);
				SWAP_DWORD(pCmd->COMMAND.RecordRunStatus.ChTab[i].dwStreamErrCount);
				SWAP_DWORD(pCmd->COMMAND.RecordRunStatus.ChTab[i].IsRecording);
				SWAP_DWORD(pCmd->COMMAND.RecordRunStatus.ChTab[i].IsStarted);
			}
			break;
		case COMM_GETHW_VIDEO_FORMAT:
			if(!IsStrict)return TRUE;
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.HwVideoFormat))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.HwVideoFormat.dwBitRate);
			SWAP_DWORD(pCmd->COMMAND.HwVideoFormat.dwFormat);
			SWAP_DWORD(pCmd->COMMAND.HwVideoFormat.dwGopSize);
			SWAP_WORD(pCmd->COMMAND.HwVideoFormat.wFrameRate);
			SWAP_WORD(pCmd->COMMAND.HwVideoFormat.wPolicy);
			SWAP_WORD(pCmd->COMMAND.HwVideoFormat.wQuality);
			SWAP_WORD(pCmd->COMMAND.HwVideoFormat.wReserved);
			break;
		case COMM_REQ_KEY_FRAME:
			break;
		case COMM_RECORD_RUN_CTRL:
			break;
		case COMM_RECORD_DOWNLOAD_LIST_REQ:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.RecordDownLoadListReq))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.RecordDownLoadListReq.m_dwChNo);
			SWAP_DWORD(pCmd->COMMAND.RecordDownLoadListReq.m_dwCmdSeqNo);
			SWAP_DWORD(pCmd->COMMAND.RecordDownLoadListReq.m_dwEndGmtTime);
			SWAP_DWORD(pCmd->COMMAND.RecordDownLoadListReq.m_dwEventMask);
			SWAP_DWORD(pCmd->COMMAND.RecordDownLoadListReq.m_dwFileNumOffset);
			SWAP_DWORD(pCmd->COMMAND.RecordDownLoadListReq.m_dwStartGmtTime);
			break;
		case COMM_RECORD_DOWNLOAD_FILE_REQ:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.RecordDownLoadFileReq))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.RecordDownLoadFileReq.m_dwChNo);
			SWAP_DWORD(pCmd->COMMAND.RecordDownLoadFileReq.m_dwCmdSeqNo);
			SWAP_DWORD(pCmd->COMMAND.RecordDownLoadFileReq.m_dwFileByteOffset);
			break;
		case COMM_RECORD_DOWNLOAD_LIST_ACK:
		case COMM_RECORD_DOWNLOAD_FILE_ACK:
			break;
		case COMM_FIRMWARE_UPDATE_REQ:
			SWAP_WORD(pCmd->COMMAND.FirmwareUpdateReq.m_wFtpServerPort);
			SWAP_WORD(pCmd->COMMAND.FirmwareUpdateReq.m_wReserved);
			break;
		case COMM_FIRMWARE_UPDATE_ACK:
			SWAP_WORD(pCmd->COMMAND.FirmwareUpdateAck.m_RetCode);
			SWAP_WORD(pCmd->COMMAND.FirmwareUpdateAck.m_wReserved);
			break;
		case COMM_CONFIG_DATABASE_DOWNLOAD_REQ:
			SWAP_DWORD(pCmd->COMMAND.DatabaseDownLoadReq.m_dwFileOffset);
			SWAP_DWORD(pCmd->COMMAND.DatabaseDownLoadReq.m_dwReserved);
			break;
		case COMM_CONFIG_DATABASE_DOWNLOAD_ACK:
			SWAP_DWORD(pCmd->COMMAND.DatabaseDownLoadAck.m_dwFileOffset);
			SWAP_DWORD(pCmd->COMMAND.DatabaseDownLoadAck.m_dwReserved);
			SWAP_DWORD(pCmd->COMMAND.DatabaseDownLoadAck.m_dwFileSize);
			SWAP_DWORD(pCmd->COMMAND.DatabaseDownLoadAck.m_dwDataLen);
			break;
		case COMM_CONFIG_DATABASE_UPLOAD_REQ:
			SWAP_DWORD(pCmd->COMMAND.DatabaseUploadReq.m_dwFileOffset);
			SWAP_DWORD(pCmd->COMMAND.DatabaseUploadReq.m_dwReserved);
			SWAP_DWORD(pCmd->COMMAND.DatabaseUploadReq.m_dwFileSize);
			SWAP_DWORD(pCmd->COMMAND.DatabaseUploadReq.m_dwDataLen);
			break;
		case COMM_CONFIG_DATABASE_UPLOAD_ACK:
			SWAP_DWORD(pCmd->COMMAND.DatabaseUploadAck.m_dwFileOffset);
			SWAP_DWORD(pCmd->COMMAND.DatabaseUploadAck.m_dwReserved);
			SWAP_DWORD(pCmd->COMMAND.DatabaseUploadAck.m_dwFileSize);
			SWAP_DWORD(pCmd->COMMAND.DatabaseUploadAck.m_dwDataLen);
			break;
		case COMM_REQ_CAPTURE_PICTURE:
		case COMM_ACK_CAPTURE_PICTURE:
			break;
		case COMM_CAPTURE_PICTURE_SAVE:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.CaptureSaveCmd))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.CaptureSaveCmd.dwChannelNo);
			SWAP_DWORD(pCmd->COMMAND.CaptureSaveCmd.dwQuality);
			SWAP_DWORD(pCmd->COMMAND.CaptureSaveCmd.IsBmpFormat);
			break;
		case COMM_GETHW_ETHADDR:
		case COMM_SETHW_ETHADDR:
			break;
		case COMM_ADDRSERVER_AUTH_NOTIFY:
			break;
		case COMM_ADDRSERVER_AUTH_REQ:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.AddrServerAuthReq))
				return FALSE;
			break;
		case COMM_ADDRSERVER_AUTH_ACK:
			break;
		case COMM_ADDRSERVER_LOGIN_REQ:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.AddrServerLoginReq))
				return FALSE;
			break;
		case COMM_ADDRSERVER_LOGIN_ACK:
			break;
		case COMM_GETIP:
			if(!IsStrict)return TRUE;
		case COMM_SETIP:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.IPConfCmd) && CurrCmdLen!=sizeof(pCmd->COMMAND.IPConfCmdEx))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.IPConfCmd.GateWay);
			SWAP_DWORD(pCmd->COMMAND.IPConfCmd.IPAddr);
			SWAP_DWORD(pCmd->COMMAND.IPConfCmd.IPMask);
			SWAP_DWORD(pCmd->COMMAND.IPConfCmd.LiveTime);
			SWAP_DWORD(pCmd->COMMAND.IPConfCmd.Version);
			break;
		case COMM_RESET:
			break;
		case COMM_RESET_CONFIG:
			break;
		case COMM_USER_LOGIN:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.UserLogin))
				return FALSE;
			break;
		case COMM_USER_LOGINEX:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.UserLoginEx))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.UserLoginEx.IPAddr);
			SWAP_DWORD(pCmd->COMMAND.UserLoginEx.UserID);
			break;
		case COMM_GET_USER_LOGIN_INFO:
			if(!IsStrict)return TRUE;
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.UserLoginTable))
				return FALSE;
			if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.UserLoginTable.CurrUserNum);
			if(pCmd->COMMAND.UserLoginTable.CurrUserNum>MAX_CLIENT_NUM)
				pCmd->COMMAND.UserLoginTable.CurrUserNum=MAX_CLIENT_NUM;
			for(i=0;i<pCmd->COMMAND.UserLoginTable.CurrUserNum;i++)
			{
				SWAP_DWORD(pCmd->COMMAND.UserLoginTable.Table[i].IPAddr);
				SWAP_DWORD(pCmd->COMMAND.UserLoginTable.Table[i].Time);
				SWAP_DWORD(pCmd->COMMAND.UserLoginTable.Table[i].UserID);
			}
			if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.UserLoginTable.CurrUserNum);
			break;
		case COMM_GET_VIDEO_MULTICAST:
			if(!IsStrict)return TRUE;
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.VideoMultiCast))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.VideoMultiCast.MultiCastIP);
			SWAP_WORD(pCmd->COMMAND.VideoMultiCast.MultiCastPort);
			SWAP_WORD(pCmd->COMMAND.VideoMultiCast.wReseved);
			break;
		case COMM_SET_WEB_SERVER_CONFIG:
		case COMM_GET_WEB_SERVER_CONFIG:
			if(!IsStrict && pCmd->Header.dwOpCode==COMM_GET_WEB_SERVER_CONFIG)
				return TRUE;
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.DevWebServerConfig))
				return FALSE;
			SWAP_WORD(pCmd->COMMAND.DevWebServerConfig.wRecordReserved);
			SWAP_WORD(pCmd->COMMAND.DevWebServerConfig.wFtpMaxRateKBps);
			SWAP_WORD(pCmd->COMMAND.DevWebServerConfig.wFtpPort);
			SWAP_WORD(pCmd->COMMAND.DevWebServerConfig.wDefaultGopSize);
			SWAP_DWORD(pCmd->COMMAND.DevWebServerConfig.dwReserved);
			for(i=0;i<sizeof(pCmd->COMMAND.DevWebServerConfig.VideoChSet)/sizeof(pCmd->COMMAND.DevWebServerConfig.VideoChSet[0]);i++)
			{
				SWAP_WORD(pCmd->COMMAND.DevWebServerConfig.VideoChSet[i].CtrlBaudrate);
				SWAP_WORD(pCmd->COMMAND.DevWebServerConfig.VideoChSet[i].VideoBitRate);
			}
			for(i=0;i<sizeof(pCmd->COMMAND.DevWebServerConfig.RecordChSet)/sizeof(pCmd->COMMAND.DevWebServerConfig.RecordChSet[0]);i++)
			{
				SWAP_DWORD(pCmd->COMMAND.DevWebServerConfig.RecordChSet[i].dwDayMask);
				SWAP_DWORD(pCmd->COMMAND.DevWebServerConfig.RecordChSet[i].dwEventMask);
				SWAP_DWORD(pCmd->COMMAND.DevWebServerConfig.RecordChSet[i].dwRecordType);
				SWAP_DWORD(pCmd->COMMAND.DevWebServerConfig.RecordChSet[i].dwReserved);
				for(j=0;j<sizeof(pCmd->COMMAND.DevWebServerConfig.RecordChSet[i].TimeTab)/sizeof(pCmd->COMMAND.DevWebServerConfig.RecordChSet[i].TimeTab[0]);j++)
				{
					SWAP_WORD(pCmd->COMMAND.DevWebServerConfig.RecordChSet[i].TimeTab[j].wEndTime);
					SWAP_WORD(pCmd->COMMAND.DevWebServerConfig.RecordChSet[i].TimeTab[j].wStartTime);
				}
			}
			break;
		case COMM_SET_FIRE_SECURITY_CONF:
		case COMM_GET_FIRE_SECURITY_CONF:
			if(!IsStrict && pCmd->Header.dwOpCode==COMM_GET_FIRE_SECURITY_CONF)
				return TRUE;
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.FireSecurityConf))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.FireSecurityConf.dwFireSecurityIsEnable);
			SWAP_DWORD(pCmd->COMMAND.FireSecurityConf.dwRemoteServerIP);
			SWAP_DWORD(pCmd->COMMAND.FireSecurityConf.dwFtpServerIP);
			SWAP_DWORD(pCmd->COMMAND.FireSecurityConf.dwReserved);
			SWAP_WORD(pCmd->COMMAND.FireSecurityConf.wRemoteServerPort);
			SWAP_WORD(pCmd->COMMAND.FireSecurityConf.wFtpServerPort);
			SWAP_WORD(pCmd->COMMAND.FireSecurityConf.wPowerOnProtectTimerS);
			SWAP_WORD(pCmd->COMMAND.FireSecurityConf.wAutoProtectTimerS);
			SWAP_WORD(pCmd->COMMAND.FireSecurityConf.wHandProtectTiemrS);
			SWAP_WORD(pCmd->COMMAND.FireSecurityConf.wOutOfDoorEarlyWarnReportTimerS);
			SWAP_WORD(pCmd->COMMAND.FireSecurityConf.wAlarmReportTimerS);
			SWAP_WORD(pCmd->COMMAND.FireSecurityConf.wServiceProvider);
			SWAP_WORD(pCmd->COMMAND.FireSecurityConf.wCardOpenDoorFailLockTimerS);
			SWAP_WORD(pCmd->COMMAND.FireSecurityConf.wDoorShockSensitivityLevel);
			SWAP_WORD(pCmd->COMMAND.FireSecurityConf.wOutOfDoorInfraredRange);
			SWAP_WORD(pCmd->COMMAND.FireSecurityConf.wPicFileSaveDays);
			SWAP_WORD(pCmd->COMMAND.FireSecurityConf.wReserved);
			SWAP_DWORD(*(DWORD *)&pCmd->COMMAND.FireSecurityConf.FlammableGasLv1Threshold);
			SWAP_DWORD(*(DWORD *)&pCmd->COMMAND.FireSecurityConf.FlammableGasLv2Threshold);
			SWAP_DWORD(*(DWORD *)&pCmd->COMMAND.FireSecurityConf.PowerFailuerThreshold);
			SWAP_DWORD(*(DWORD *)&pCmd->COMMAND.FireSecurityConf.TemperatureHighThreshold);
			SWAP_DWORD(*(DWORD *)&pCmd->COMMAND.FireSecurityConf.HumidityThreshold);
			SWAP_DWORD(*(DWORD *)&pCmd->COMMAND.FireSecurityConf.fFlammableGasConstantA);
			SWAP_DWORD(*(DWORD *)&pCmd->COMMAND.FireSecurityConf.fFlammableGasConstantB);
			SWAP_DWORD(*(DWORD *)&pCmd->COMMAND.FireSecurityConf.fTemperatureConstantA);
			SWAP_DWORD(*(DWORD *)&pCmd->COMMAND.FireSecurityConf.fTemperatureConstantB);
			SWAP_DWORD(*(DWORD *)&pCmd->COMMAND.FireSecurityConf.fHumidityConstantA);
			SWAP_DWORD(*(DWORD *)&pCmd->COMMAND.FireSecurityConf.fHumidityConstantB);
			break;
		case COMM_GET_RUN_STATUS_INFO:
			if(!IsStrict)return TRUE;
			break;
		case COMM_GET_RUN_STATUS_INFOEX:
			if(!IsStrict)return TRUE;
			if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.dwMsgVersion);
			if(pCmd->COMMAND.DevRunStatus.dwMsgVersion==DEV_RUN_STATUS_MSG_VER1)
			{
				DWORD	i;

				if(CurrCmdLen!=sizeof(pCmd->COMMAND.DevRunStatus.dwMsgVersion)
					+sizeof(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1))
						return FALSE;

				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.CurrUserCount);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.CurrUserCount;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.UserTable[i].dwAudioIP);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.UserTable[i].dwUserID);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.UserTable[i].dwUserIP);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.UserTable[i].dwUserLevel);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.UserTable[i].dwVideoIP);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.UserTable[i].wAudioID);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.UserTable[i].wAudioPort);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.UserTable[i].wCom1BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.UserTable[i].wCom2BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.UserTable[i].wVideoID);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.UserTable[i].wVideoPort);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.CurrUserCount);
				
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoDecCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoDecCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoDecChBitRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoDecChFrameRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoDecChIsLive[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoDecChLostCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoDecChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoDecChRebootCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoDecCurrChNum);
				
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoEncCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoEncCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoEncChBitRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoEncChFrameRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoEncChIsLive[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoEncChLostCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoEncChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoEncChRebootCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwVideoEncCurrChNum);

				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwAudioDecCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwAudioDecCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwAudioDecChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwAudioDecLostCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwAudioDecCurrChNum);
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwAudioEncCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwAudioEncCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwAudioEncChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwAudioEncChLostCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwAudioEncCurrChNum);
				
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwSerCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwSerCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwSerErrorCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwSerRxCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwSerRxOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwSerTxCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwSerTxOverCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwSerCurrChNum);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwDataDiCurrVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwDataDoCurrVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwDataWarnDiStdVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwDataWarnMask);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwDataWarnStatus);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwEthCollisions);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwEthRxDropped);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwEthRxErrors);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwEthRxMulticast);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwEthRxPackets);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwEthTxDropped);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwEthTxErrors);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwEthTxPackets);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwDevType);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwDevIP);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwFirmwareVer);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwSysRunTime);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwSysRealTime);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwCpuOccupancy);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver1.dwWebServerRebootCount);
			}
			else if(pCmd->COMMAND.DevRunStatus.dwMsgVersion==DEV_RUN_STATUS_MSG_VER2)
			{
				DWORD	i;

				if(CurrCmdLen!=sizeof(pCmd->COMMAND.DevRunStatus.dwMsgVersion)
					+sizeof(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2))
						return FALSE;

				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.CurrUserCount);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.CurrUserCount;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.UserTable[i].dwAudioIP);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.UserTable[i].dwUserID);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.UserTable[i].dwUserIP);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.UserTable[i].dwUserLevel);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.UserTable[i].dwVideoIP);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.UserTable[i].wAudioID);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.UserTable[i].wAudioPort);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.UserTable[i].wCom1BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.UserTable[i].wCom2BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.UserTable[i].wCom3BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.UserTable[i].wCom4BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.UserTable[i].wVideoID);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.UserTable[i].wVideoPort);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.CurrUserCount);
				
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoDecCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoDecCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoDecChBitRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoDecChFrameRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoDecChIsLive[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoDecChLostCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoDecChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoDecChRebootCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoDecCurrChNum);
				
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoEncCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoEncCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoEncChBitRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoEncChFrameRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoEncChIsLive[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoEncChLostCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoEncChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoEncChRebootCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwVideoEncCurrChNum);

				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwAudioDecCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwAudioDecCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwAudioDecChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwAudioDecLostCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwAudioDecCurrChNum);
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwAudioEncCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwAudioEncCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwAudioEncChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwAudioEncChLostCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwAudioEncCurrChNum);
				
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwSerCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwSerCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwSerErrorCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwSerRxCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwSerRxOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwSerTxCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwSerTxOverCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwSerCurrChNum);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwDataDiCurrVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwDataDoCurrVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwDataWarnDiStdVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwDataWarnMask);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwDataWarnStatus);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwEthCollisions);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwEthRxDropped);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwEthRxErrors);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwEthRxMulticast);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwEthRxPackets);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwEthTxDropped);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwEthTxErrors);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwEthTxPackets);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwDevType);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwDevIP);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwFirmwareVer);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwSysRunTime);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwSysRealTime);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwCpuOccupancy);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.dwWebServerRebootCount);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciDevStatus.dwDmaSendTimeoutCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciDevStatus.dwMsgSendTimeoutCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciDevStatus.dwRecvCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciDevStatus.dwRecvOutOfOrderCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciDevStatus.dwRecvRepeatCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciDevStatus.dwRecvValidCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciDevStatus.dwSendFailCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciDevStatus.dwSendReadyCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciDevStatus.dwTrySendCount);
				
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciHostStatus.dwDmaSendTimeoutCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciHostStatus.dwMsgSendTimeoutCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciHostStatus.dwRecvCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciHostStatus.dwRecvOutOfOrderCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciHostStatus.dwRecvRepeatCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciHostStatus.dwRecvValidCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciHostStatus.dwSendFailCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciHostStatus.dwSendReadyCount);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver2.PciHostStatus.dwTrySendCount);
			}
			else if(pCmd->COMMAND.DevRunStatus.dwMsgVersion==DEV_RUN_STATUS_MSG_VER3)
			{
				DWORD	i;

				if(CurrCmdLen!=sizeof(pCmd->COMMAND.DevRunStatus.dwMsgVersion)
					+sizeof(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3))
						return FALSE;

				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.CurrUserCount);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.CurrUserCount;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].dwAudioIP);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].dwUserID);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].dwUserIP);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].dwUserLevel);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].dwVideoIP);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].wAudioID);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].wAudioPort);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].wCom1BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].wCom2BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].wCom3BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].wCom4BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].wCom5BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].wCom6BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].wCom7BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].wVideoID);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.UserTable[i].wVideoPort);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.CurrUserCount);
				
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwVideoEncCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwVideoEncCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwVideoEncChBitRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwVideoEncChFrameRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwVideoEncChIsLive[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwVideoEncChLostCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwVideoEncChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwVideoEncChRebootCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwVideoEncCurrChNum);

				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwAudioDecCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwAudioDecCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwAudioDecChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwAudioDecLostCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwAudioDecCurrChNum);
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwAudioEncCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwAudioEncCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwAudioEncChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwAudioEncChLostCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwAudioEncCurrChNum);
				
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwSerCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwSerCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwSerErrorCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwSerRxCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwSerRxOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwSerTxCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwSerTxOverCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwSerCurrChNum);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwDataDiCurrVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwDataDoCurrVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwDataWarnDiStdVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwDataWarnMask);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwDataWarnStatus);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwEthCollisions);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwEthRxDropped);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwEthRxErrors);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwEthRxMulticast);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwEthRxPackets);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwEthTxDropped);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwEthTxErrors);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwEthTxPackets);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwDevType);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwDevIP);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwFirmwareVer);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwSysRunTime);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwSysRealTime);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwCpuOccupancy);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver3.dwWebServerRebootCount);
			}
			else if(pCmd->COMMAND.DevRunStatus.dwMsgVersion==DEV_RUN_STATUS_MSG_VER4)
			{
				DWORD	i;

				if(CurrCmdLen!=sizeof(pCmd->COMMAND.DevRunStatus.dwMsgVersion)
					+sizeof(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4))
						return FALSE;

				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.CurrUserCount);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.CurrUserCount;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].dwAudioIP);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].dwUserID);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].dwUserIP);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].dwUserLevel);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].dwVideoIP);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].wAudioID);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].wAudioPort);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].wCom1BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].wCom2BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].wCom3BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].wCom4BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].wCom5BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].wCom6BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].wCom7BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].wVideoID);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.UserTable[i].wVideoPort);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.CurrUserCount);
				
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwVideoEncCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwVideoEncCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwVideoEncChBitRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwVideoEncChFrameRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwVideoEncChIsLive[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwVideoEncChLostCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwVideoEncChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwVideoEncChRebootCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwVideoEncCurrChNum);

				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwAudioDecCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwAudioDecCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwAudioDecChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwAudioDecLostCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwAudioDecCurrChNum);
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwAudioEncCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwAudioEncCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwAudioEncChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwAudioEncChLostCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwAudioEncCurrChNum);
				
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwSerCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwSerCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwSerErrorCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwSerRxCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwSerRxOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwSerTxCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwSerTxOverCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwSerCurrChNum);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwDataDiCurrVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwDataDoCurrVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwDataWarnDiStdVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwDataWarnMask);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwDataWarnStatus);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwEthCollisions);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwEthRxDropped);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwEthRxErrors);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwEthRxMulticast);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwEthRxPackets);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwEthTxDropped);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwEthTxErrors);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwEthTxPackets);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwDevType);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwDevIP);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwFirmwareVer);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwSysRunTime);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwSysRealTime);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwCpuOccupancy);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver4.dwWebServerRebootCount);
			}
			else if(pCmd->COMMAND.DevRunStatus.dwMsgVersion==DEV_RUN_STATUS_MSG_VER6)
			{
				DWORD	i;

				if(CurrCmdLen!=sizeof(pCmd->COMMAND.DevRunStatus.dwMsgVersion)
					+sizeof(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6))
						return FALSE;

				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.CurrUserCount);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.CurrUserCount;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].dwAudioIP);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].dwUserID);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].dwUserIP);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].dwUserLevel);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].dwVideoIP);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].wAudioID);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].wAudioPort);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].wCom1BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].wCom2BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].wCom3BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].wCom4BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].wCom5BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].wCom6BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].wCom7BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].wVideoID);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.UserTable[i].wVideoPort);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.CurrUserCount);
				
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwVideoEncCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwVideoEncCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwVideoEncChBitRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwVideoEncChFrameRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwVideoEncChIsLive[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwVideoEncChLostCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwVideoEncChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwVideoEncChRebootCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwVideoEncCurrChNum);

				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwAudioDecCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwAudioDecCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwAudioDecChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwAudioDecLostCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwAudioDecCurrChNum);
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwAudioEncCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwAudioEncCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwAudioEncChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwAudioEncChLostCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwAudioEncCurrChNum);
				
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwSerCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwSerCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwSerErrorCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwSerRxCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwSerRxOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwSerTxCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwSerTxOverCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwSerCurrChNum);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwDataDiCurrVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwDataDoCurrVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwDataWarnDiStdVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwDataWarnMask);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwDataWarnStatus);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwEthCollisions);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwEthRxDropped);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwEthRxErrors);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwEthRxMulticast);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwEthRxPackets);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwEthTxDropped);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwEthTxErrors);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwEthTxPackets);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwDevType);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwDevIP);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwFirmwareVer);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwPcmFirmwareVer);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwSysRunTime);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwSysRealTime);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwCpuOccupancy);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver6.dwWebServerRebootCount);
			}
			else if(pCmd->COMMAND.DevRunStatus.dwMsgVersion==DEV_RUN_STATUS_MSG_VER7)
			{
				DWORD	i;

				if(CurrCmdLen!=sizeof(pCmd->COMMAND.DevRunStatus.dwMsgVersion)
					+sizeof(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7))
						return FALSE;

				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.CurrUserCount);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.CurrUserCount;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].dwAudioIP);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].dwUserID);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].dwUserIP);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].dwUserLevel);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].dwVideoIP);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].wAudioID);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].wAudioPort);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].wCom1BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].wCom2BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].wCom3BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].wCom4BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].wCom5BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].wCom6BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].wCom7BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].wCom8BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].wCom9BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].wVideoID);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.UserTable[i].wVideoPort);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.CurrUserCount);
				
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwVideoEncCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwVideoEncCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwVideoEncChBitRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwVideoEncChFrameRate[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwVideoEncChIsLive[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwVideoEncChLostCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwVideoEncChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwVideoEncChRebootCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwVideoEncCurrChNum);

				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwAudioDecCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwAudioDecCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwAudioDecChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwAudioDecLostCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwAudioDecCurrChNum);
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwAudioEncCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwAudioEncCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwAudioEncChOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwAudioEncChLostCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwAudioEncCurrChNum);
				
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwSerCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwSerCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwSerErrorCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwSerRxCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwSerRxOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwSerTxCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwSerTxOverCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwSerCurrChNum);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwDataDiCurrVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwDataDoCurrVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwDataWarnDiStdVal);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwDataWarnMask);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwDataWarnStatus);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwEthCollisions);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwEthRxDropped);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwEthRxErrors);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwEthRxMulticast);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwEthRxPackets);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwEthTxDropped);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwEthTxErrors);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwEthTxPackets);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwDevType);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwDevIP);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwFirmwareVer);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwPcmFirmwareVer);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwSysRunTime);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwSysRealTime);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver7.dwCpuOccupancy);
			}
			else if(pCmd->COMMAND.DevRunStatus.dwMsgVersion==DEV_RUN_STATUS_MSG_VER8)
			{
				DWORD	i;
				
				if(CurrCmdLen!=sizeof(pCmd->COMMAND.DevRunStatus.dwMsgVersion)
					+sizeof(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8))
					return FALSE;
				
				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.CurrUserCount);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.CurrUserCount;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].dwAudioIP);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].dwUserID);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].dwUserIP);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].dwUserLevel);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].dwVideoIP);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].wAudioID);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].wAudioPort);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].wCom1BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].wCom2BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].wCom3BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].wCom4BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].wCom5BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].wCom6BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].wCom7BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].wCom8BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].wCom9BaudRate);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].wVideoID);
					SWAP_WORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.UserTable[i].wVideoPort);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.CurrUserCount);

				if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwSerCurrChNum);
				for(i=0;i<pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwSerCurrChNum;i++)
				{
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwSerErrorCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwSerRxCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwSerRxOverCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwSerTxCount[i]);
					SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwSerTxOverCount[i]);
				}
				if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwSerCurrChNum);

				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwEthCollisions);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwEthRxDropped);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwEthRxErrors);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwEthRxMulticast);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwEthRxPackets);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwEthTxDropped);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwEthTxErrors);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwEthTxPackets);
				
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwDevType);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwDevIP);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwFirmwareVer);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwPcmFirmwareVer);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwSysRunTime);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwSysRealTime);
				SWAP_DWORD(pCmd->COMMAND.DevRunStatus.MsgBody.Ver8.dwCpuOccupancy);
			}
			else
				return FALSE;
			if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.DevRunStatus.dwMsgVersion);
			break;
		case COMM_PUT_SERDATA:
		case COMM_GET_SERDATA:
			break;
		case COMM_OPEN_SER:
		case COMM_CLOSE_SER:
		case COMM_SETHW_SER:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.SerialConf))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.SerialConf.BaudRate);
			SWAP_DWORD(pCmd->COMMAND.SerialConf.DataBits);
			SWAP_DWORD(pCmd->COMMAND.SerialConf.parity);
			SWAP_DWORD(pCmd->COMMAND.SerialConf.Status);
			SWAP_DWORD(pCmd->COMMAND.SerialConf.StopBits);
			SWAP_DWORD(pCmd->COMMAND.SerialConf.unit);
			SWAP_DWORD(pCmd->COMMAND.SerialConf.WorkMode);
			break;
		case COMM_WARN_DI_LINKAGE_DO:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.WarnLinkage))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.WarnLinkage.WarnEvent);
			break;
		case COMM_GETHW_SER:
			if(!IsStrict)return TRUE;
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.SerialInfo))
				return FALSE;
			if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.SerialInfo.UnitNum);
			if(pCmd->COMMAND.SerialInfo.UnitNum>MAX_SERIAL_NUM)
				pCmd->COMMAND.SerialInfo.UnitNum=MAX_SERIAL_NUM;
			for(i=0;i<pCmd->COMMAND.SerialInfo.UnitNum;i++)
			{
				SWAP_DWORD(pCmd->COMMAND.SerialInfo.pParam[i].BaudRate);
				SWAP_DWORD(pCmd->COMMAND.SerialInfo.pParam[i].DataBits);
				SWAP_DWORD(pCmd->COMMAND.SerialInfo.pParam[i].parity);
				SWAP_DWORD(pCmd->COMMAND.SerialInfo.pParam[i].Status);
				SWAP_DWORD(pCmd->COMMAND.SerialInfo.pParam[i].StopBits);
				SWAP_DWORD(pCmd->COMMAND.SerialInfo.pParam[i].unit);
				SWAP_DWORD(pCmd->COMMAND.SerialInfo.pParam[i].WorkMode);
			}
			if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.SerialInfo.UnitNum);
			break;
		case COMM_GETHW_SER_EX:
			if(!IsStrict)return TRUE;
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.SerialInfoEx))
				return FALSE;
			if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.SerialInfoEx.UnitNum);
			if(pCmd->COMMAND.SerialInfoEx.UnitNum>MAX_SERIAL_NUM_EX)
				pCmd->COMMAND.SerialInfoEx.UnitNum=MAX_SERIAL_NUM_EX;
			for(i=0;i<pCmd->COMMAND.SerialInfoEx.UnitNum;i++)
			{
				SWAP_DWORD(pCmd->COMMAND.SerialInfoEx.pParam[i].BaudRate);
				SWAP_DWORD(pCmd->COMMAND.SerialInfoEx.pParam[i].DataBits);
				SWAP_DWORD(pCmd->COMMAND.SerialInfoEx.pParam[i].parity);
				SWAP_DWORD(pCmd->COMMAND.SerialInfoEx.pParam[i].Status);
				SWAP_DWORD(pCmd->COMMAND.SerialInfoEx.pParam[i].StopBits);
				SWAP_DWORD(pCmd->COMMAND.SerialInfoEx.pParam[i].unit);
				SWAP_DWORD(pCmd->COMMAND.SerialInfoEx.pParam[i].WorkMode);
			}
			if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.SerialInfoEx.UnitNum);
			break;
		case COMM_QUERY_USER_INFO:
			return FALSE;
			break;
		case COMM_MODIFY_USER_INFO:
			return FALSE;
			break;
		case COMM_VALIDATE_USER_INFO:
			return FALSE;
			break;
		case COMM_READ_COMMON_BUF:
			return FALSE;
			break;
		case COMM_WRITE_COMMON_BUF:
			return FALSE;
			break;
		case COMM_READ_AVSU_CONF:
			return FALSE;
			break;
		case COMM_WRITE_AVSU_CONF:
			return FALSE;
			break;
		case COMM_CONFIG_E1_CHANNEL:
			return FALSE;
			break;
		case COMM_GET_E1CHANNEL_CON:
			return FALSE;
			break;
		case COMM_GET_REMOTE_NETMODE:
			if(!IsStrict)return TRUE;
		case COMM_SET_REMOTE_NETMODE:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.RemoteNetModeCmd))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.RemoteNetModeCmd.AudioMode);
			SWAP_DWORD(pCmd->COMMAND.RemoteNetModeCmd.NetMode);
			break;
		case COMM_START_AUDIO:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.StartAudio))
				return FALSE;
			break;
		case COMM_STOP_AUDIO:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.StopAudio))
				return FALSE;
			break;
		case COMM_START_TALK_BACK:
			break;
		case COMM_STOP_TALK_BACK:
			break;
		case COMM_REQ_VIDEO_PLAY:
			break;
		case COMM_ACK_VIDEO_PLAY:
			if(!IsStrict)return TRUE;
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.VideoPlayAck))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.VideoPlayAck.dwAudioFormat);
			SWAP_DWORD(pCmd->COMMAND.VideoPlayAck.dwEncoderIP);
			SWAP_DWORD(pCmd->COMMAND.VideoPlayAck.dwVideoFormat);
			SWAP_DWORD(pCmd->COMMAND.VideoPlayAck.dwVideoKbps);
			SWAP_WORD(pCmd->COMMAND.VideoPlayAck.wAudioChNo);
			SWAP_WORD(pCmd->COMMAND.VideoPlayAck.wVideoChNo);
			SWAP_WORD(pCmd->COMMAND.VideoPlayAck.wVideoFps);
			SWAP_WORD(pCmd->COMMAND.VideoPlayAck.wVideoGopSize);
			SWAP_WORD(pCmd->COMMAND.VideoPlayAck.wVideoQuality);
			break;
		case COMM_GET_DEVNETID_LIST:
			return FALSE;
			break;
		case COMM_GETIPEX:
		case COMM_GETIPNEW:
		case COMM_GETHW_ETHADDREX:
			if(!IsStrict)return TRUE;
			if(pCmd->Header.dwOpCode == COMM_GETIPNEW)
			{
				if(CurrCmdLen!=sizeof(pCmd->COMMAND.IPConfCmdNew))
					return FALSE;
			}
			else
			{
				if(CurrCmdLen!=sizeof(pCmd->COMMAND.IPConfCmdEx))
					return FALSE;
			}
			SWAP_DWORD(pCmd->COMMAND.IPConfCmdEx.DevType);
			SWAP_DWORD(pCmd->COMMAND.IPConfCmdEx.GateWay);
			SWAP_DWORD(pCmd->COMMAND.IPConfCmdEx.IPAddr);
			SWAP_DWORD(pCmd->COMMAND.IPConfCmdEx.IPMask);
			SWAP_DWORD(pCmd->COMMAND.IPConfCmdEx.LiveTime);
			SWAP_DWORD(pCmd->COMMAND.IPConfCmdEx.Version);
			break;
		case COMM_GET_PCM_DEV_PORT_CONF_TAB:
			if(!IsStrict)return TRUE;
			for(i=0;i<pCmd->COMMAND.PcmPortTab.m_PortConfNum;i++)
			{
				SWAP_WORD(pCmd->COMMAND.PcmPortTab.m_wReserved);
			}
			break;
		case COMM_GET_PCM_DEV_CLASS_TAB:
			if(!IsStrict)return TRUE;
			for(i=0;i<pCmd->COMMAND.PcmClassTab.m_DevClassNum;i++)
			{
				SWAP_WORD(pCmd->COMMAND.PcmClassTab.m_wReserved);
				SWAP_WORD(pCmd->COMMAND.PcmClassTab.m_DevClassTab[i].m_wReserved);
			}
			break;
		case COMM_GET_PCM_DEV_TYPE_TAB:
			if(!IsStrict)return TRUE;
			for(i=0;i<pCmd->COMMAND.PcmTypeTab.m_DevTypeNum;i++)
			{
				SWAP_WORD(pCmd->COMMAND.PcmTypeTab.m_wReserved);
				SWAP_WORD(pCmd->COMMAND.PcmTypeTab.m_DevTypeTab[i].m_wDevType);
				SWAP_DWORD(pCmd->COMMAND.PcmTypeTab.m_DevTypeTab[i].m_OptionPollTimerMS);
				SWAP_DWORD(pCmd->COMMAND.PcmTypeTab.m_DevTypeTab[i].m_OptionPollTimeoutMS);
			}
			break;
		case COMM_GET_PCM_DEV_INSTANCE_TAB:
			if(!IsStrict)return TRUE;
			SWAP_WORD(pCmd->COMMAND.PcmDevInstTab.m_wReserved);
			for(i=0;i<pCmd->COMMAND.PcmDevInstTab.m_DevInstanceNum;i++)
			{
				SWAP_WORD(pCmd->COMMAND.PcmDevInstTab.m_DevInstanceTab[i].m_wDevType);
				SWAP_DWORD(pCmd->COMMAND.PcmDevInstTab.m_DevInstanceTab[i].m_OptionPollTimerMS);
				SWAP_DWORD(pCmd->COMMAND.PcmDevInstTab.m_DevInstanceTab[i].m_OptionPollTimeoutMS);
			}
			break;
		case COMM_SET_PCM_DEV_INSTANCE_TAB:
			SWAP_WORD(pCmd->COMMAND.PcmDevInstTab.m_wReserved);
			for(i=0;i<pCmd->COMMAND.PcmDevInstTab.m_DevInstanceNum;i++)
			{
				SWAP_WORD(pCmd->COMMAND.PcmDevInstTab.m_DevInstanceTab[i].m_wDevType);
				SWAP_DWORD(pCmd->COMMAND.PcmDevInstTab.m_DevInstanceTab[i].m_OptionPollTimerMS);
				SWAP_DWORD(pCmd->COMMAND.PcmDevInstTab.m_DevInstanceTab[i].m_OptionPollTimeoutMS);
			}
			break;
		case COMM_DYNAMIC_SWITCH_AIM:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.DynamicSwitchAimCmd))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.DynamicSwitchAimCmd.DestDevIPID);
			break;
		case COMM_TXD_TRANSPARENCY_CTRL:
			return FALSE;
			break;
		case COMM_REMOTE_PROGLOAD:
			return FALSE;
			break;
		case COMM_REMOTE_PROGLOAD_ACK:
			return FALSE;
			break;
		case COMM_REMOTE_VIDEO_RESET:
			return FALSE;
			break;
		case COMM_GET_LOCAL_SERCON:
			if(!IsStrict)return TRUE;
		case COMM_SET_LOCAL_SERCON:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.SerConfigCmd))
				return FALSE;
			if(IsFromNet)
			{
				SWAP_WORD(pCmd->COMMAND.SerConfigCmd.LocalSerNum);
				SWAP_WORD(pCmd->COMMAND.SerConfigCmd.RemoteSerNum);
			}
			if(pCmd->COMMAND.SerConfigCmd.LocalSerNum>MAX_SERIAL_NUM)
				pCmd->COMMAND.SerConfigCmd.LocalSerNum=MAX_SERIAL_NUM;
			if(pCmd->COMMAND.SerConfigCmd.RemoteSerNum>MAX_SERIAL_NUM)
				pCmd->COMMAND.SerConfigCmd.RemoteSerNum=MAX_SERIAL_NUM;
			for(i=0;i<pCmd->COMMAND.SerConfigCmd.LocalSerNum;i++)
			{
				SWAP_WORD(pCmd->COMMAND.SerConfigCmd.LocalSerTable[i].BaudRate);
				SWAP_WORD(pCmd->COMMAND.SerConfigCmd.LocalSerTable[i].WorkMode);
			}
			for(i=0;i<pCmd->COMMAND.SerConfigCmd.RemoteSerNum;i++)
			{
				SWAP_WORD(pCmd->COMMAND.SerConfigCmd.RemoteSerTable[i].BaudRate);
				SWAP_WORD(pCmd->COMMAND.SerConfigCmd.RemoteSerTable[i].WorkMode);
			}
			if(!IsFromNet)
			{
				SWAP_WORD(pCmd->COMMAND.SerConfigCmd.LocalSerNum);
				SWAP_WORD(pCmd->COMMAND.SerConfigCmd.RemoteSerNum);
			}
			break;
		case COMM_GET_LOCAL_SERCON_EX:
			if(!IsStrict)return TRUE;
		case COMM_SET_LOCAL_SERCON_EX:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.SerConfigCmdEx))
				return FALSE;
			if(IsFromNet)
			{
				SWAP_WORD(pCmd->COMMAND.SerConfigCmdEx.LocalSerNumEx);
				SWAP_WORD(pCmd->COMMAND.SerConfigCmdEx.RemoteSerNumEx);
			}
			if(pCmd->COMMAND.SerConfigCmdEx.LocalSerNumEx>MAX_SERIAL_NUM_EX)
				pCmd->COMMAND.SerConfigCmdEx.LocalSerNumEx=MAX_SERIAL_NUM_EX;
			if(pCmd->COMMAND.SerConfigCmdEx.RemoteSerNumEx>MAX_SERIAL_NUM_EX)
				pCmd->COMMAND.SerConfigCmdEx.RemoteSerNumEx=MAX_SERIAL_NUM_EX;
			for(i=0;i<pCmd->COMMAND.SerConfigCmdEx.LocalSerNumEx;i++)
			{
				SWAP_DWORD(pCmd->COMMAND.SerConfigCmdEx.LocalSerTableEx[i].BaudRate);
				SWAP_WORD(pCmd->COMMAND.SerConfigCmdEx.LocalSerTableEx[i].WorkMode);
				SWAP_WORD(pCmd->COMMAND.SerConfigCmdEx.LocalSerTableEx[i].wReserved);
				SWAP_WORD(pCmd->COMMAND.SerConfigCmdEx.LocalSerTableEx[i].ResponseTime);
				SWAP_WORD(pCmd->COMMAND.SerConfigCmdEx.LocalSerTableEx[i].RecvTimeout);
			}
			for(i=0;i<pCmd->COMMAND.SerConfigCmdEx.RemoteSerNumEx;i++)
			{
				SWAP_DWORD(pCmd->COMMAND.SerConfigCmdEx.RemoteSerTableEx[i].BaudRate);
				SWAP_WORD(pCmd->COMMAND.SerConfigCmdEx.RemoteSerTableEx[i].WorkMode);
				SWAP_WORD(pCmd->COMMAND.SerConfigCmdEx.RemoteSerTableEx[i].wReserved);
				SWAP_WORD(pCmd->COMMAND.SerConfigCmdEx.RemoteSerTableEx[i].ResponseTime);
				SWAP_WORD(pCmd->COMMAND.SerConfigCmdEx.RemoteSerTableEx[i].RecvTimeout);
			}
			if(!IsFromNet)
			{
				SWAP_WORD(pCmd->COMMAND.SerConfigCmdEx.LocalSerNumEx);
				SWAP_WORD(pCmd->COMMAND.SerConfigCmdEx.RemoteSerNumEx);
			}
			break;
		case COMM_GET_LOCAL_NETMODE:
			if(!IsStrict)return TRUE;
		case COMM_SET_LOCAL_NETMODE:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.NetModeConfigCmd))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.NetModeConfigCmd.DestDevIPID);
			SWAP_DWORD(pCmd->COMMAND.NetModeConfigCmd.NetType);
			break;
		case COMM_GET_LOCAL_USER:
			if(!IsStrict)return TRUE;
		case COMM_SET_LOCAL_USER:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.LocalUserConCmd))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.LocalUserConCmd.UserLevel);
			break;
		case COMM_GET_LOCAL_VIDEOAUDIO:
			if(!IsStrict)return TRUE;
		case COMM_SET_LOCAL_VIDEOAUDIO:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.LocalAudioVideo))
				return FALSE;
			SWAP_DWORD(pCmd->COMMAND.LocalAudioVideo.AudioMode);
			SWAP_DWORD(pCmd->COMMAND.LocalAudioVideo.VideoBitRate);
			SWAP_DWORD(pCmd->COMMAND.LocalAudioVideo.VideoFormat);
			SWAP_DWORD(pCmd->COMMAND.LocalAudioVideo.VideoQuality);
			break;
		case COMM_GET_LOCAL_SERTIME:
			if(!IsStrict)return TRUE;
		case COMM_SET_LOCAL_SERTIME:
			if(CurrCmdLen!=sizeof(pCmd->COMMAND.SerTimeConfigCmd))
				return FALSE;
			if(IsFromNet)SWAP_DWORD(pCmd->COMMAND.SerTimeConfigCmd.SerNum);
			if(pCmd->COMMAND.SerTimeConfigCmd.SerNum>MAX_SERIAL_NUM)
				pCmd->COMMAND.SerTimeConfigCmd.SerNum=MAX_SERIAL_NUM;
			for(i=0;i<pCmd->COMMAND.SerTimeConfigCmd.SerNum;i++)
			{
				SWAP_WORD(pCmd->COMMAND.SerTimeConfigCmd.SerTimeTable[i].RecvTimeout);
				SWAP_WORD(pCmd->COMMAND.SerTimeConfigCmd.SerTimeTable[i].ResponseTime);
			}
			if(!IsFromNet)SWAP_DWORD(pCmd->COMMAND.SerTimeConfigCmd.SerNum);
			break;
		case COMM_PCM_GEN_GET_DEV_DATA_DESC_REQ:
			SWAP_WORD(pCmd->COMMAND.PcmGetDataDescReq.m_wReserved);
			break;
		case COMM_PCM_GEN_GET_DEV_DATA_DESC_ACK:
			SWAP_WORD(pCmd->COMMAND.PcmGetDataDescAck.m_wReserved);
			SWAP_DWORD(pCmd->COMMAND.PcmGetDataDescAck.m_DataDesc.m_ReadOnlyNodeNum);
			SWAP_DWORD(pCmd->COMMAND.PcmGetDataDescAck.m_DataDesc.m_ReadWriteNodeNum);
			break;
		case COMM_PCM_GEN_GET_DEV_RO_DATA_REQ:
			SWAP_WORD(pCmd->COMMAND.PcmGetRoDataReq.m_wReserved);
			SWAP_DWORD(pCmd->COMMAND.PcmGetRoDataReq.m_ReqNodeNum);
			SWAP_DWORD(pCmd->COMMAND.PcmGetRoDataReq.m_StartNodeNo);
			break;
		case COMM_PCM_GEN_GET_DEV_RO_DATA_ACK:
			SWAP_DWORD(pCmd->COMMAND.PcmGetRoDataAck.m_AckNodeNum);
			SWAP_DWORD(pCmd->COMMAND.PcmGetRoDataAck.m_NodeDataLen);
			SWAP_DWORD(pCmd->COMMAND.PcmGetRoDataAck.m_StartNodeNo);
			break;
		case COMM_PCM_GEN_GET_DEV_RW_DATA_REQ:
			SWAP_WORD(pCmd->COMMAND.PcmGetRwDataReq.m_wReserved);
			SWAP_DWORD(pCmd->COMMAND.PcmGetRwDataReq.m_ReqNodeNum);
			SWAP_DWORD(pCmd->COMMAND.PcmGetRwDataReq.m_StartNodeNo);
			break;
		case COMM_PCM_GEN_GET_DEV_RW_DATA_ACK:
			SWAP_DWORD(pCmd->COMMAND.PcmGetRwDataAck.m_AckNodeNum);
			SWAP_DWORD(pCmd->COMMAND.PcmGetRwDataAck.m_NodeDataLen);
			SWAP_DWORD(pCmd->COMMAND.PcmGetRwDataAck.m_StartNodeNo);
			break;
		case COMM_PCM_GEN_SET_DEV_RW_DATA_REQ:
			SWAP_WORD(pCmd->COMMAND.PcmSetRwDataReq.m_wReserved);
			SWAP_DWORD(pCmd->COMMAND.PcmSetRwDataReq.m_NodeDataLen);
			SWAP_DWORD(pCmd->COMMAND.PcmSetRwDataReq.m_ReqNodeNum);
			SWAP_DWORD(pCmd->COMMAND.PcmSetRwDataReq.m_StartNodeNo);
			break;
		case COMM_PCM_GEN_SET_DEV_RW_DATA_ACK:
			SWAP_WORD(pCmd->COMMAND.PcmSetRwDataAck.m_wReserved);
			SWAP_DWORD(pCmd->COMMAND.PcmSetRwDataAck.m_AckNodeNum);
			SWAP_DWORD(pCmd->COMMAND.PcmSetRwDataAck.m_StartNodeNo);
			break;
		default:
			PROTOCOL_API_LOG(LOG_EVENT_LEVEL_WARNING,"系统当前不能识别或者不支持协议命令码(0x%X)...\r\n",(unsigned int)pCmd->Header.dwOpCode);
			return FALSE;
		}
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: JSYA_ProtocolRecvCmd
 *功能描述: 从网络接收协议命令报文
 *输入描述: 网络接口句柄、命令缓冲区、模式标记、可选远程过滤地址以及结果
 *输出描述: 可选输出远程冲突标记
 *返回描述:	成功(TRUE)/失败(FALSE)
 *作者日期: ZCQ/2007/11/15
 *全局声明: 无
 *特殊说明: 仅仅处理协议头部字节序
 ************************************************************************************************************************************************************************       
 */
extern BOOL JSYA_ProtocolRecvCmd(int Fd,PCMD_PACKET pCmd,BOOL IsStreamMode,struct sockaddr_in *pRemoteAddr,BOOL *pConflict)
{
	U32		Len;

	if(IsStreamMode)
	{
		if(!RecvStream(Fd,pCmd,sizeof(MMNET_HEADER)))
		{
			return FALSE;
		}
		if(!SwapMmnetHeader(&pCmd->Header,TRUE))
		{
			PROTOCOL_API_LOG(LOG_EVENT_LEVEL_ERR,"系统收到协议头部无效的数据包(0x%X,0x%X,0x%X)...\n\r"
				,pCmd->Header.nType,pCmd->Header.CheckSum,(unsigned int)pCmd->Header.dwLength);
			return FALSE;
		}
		Len=pCmd->Header.dwLength-sizeof(MMNET_HEADER);
		if(Len>0)
		{
			if(!RecvStream(Fd,(U8 *)pCmd+sizeof(MMNET_HEADER),Len))
			{
				return FALSE;
			}
		}
	}
	else
	{
		Len=PROTOCOL_PACKET_MAX_SIZE;
		if(!RecvUdpFrom(Fd,pCmd,&Len,pRemoteAddr,pConflict))
			return FALSE;
		if(Len<PROTOCOL_PACKET_MIN_SIZE)
		{
			PROTOCOL_API_LOG(LOG_EVENT_LEVEL_ERR,"系统收到无效的协议数据包(0x%X)...\n\r",(unsigned int)Len);
			return FALSE;
		}
		if(!SwapMmnetHeader(&pCmd->Header,TRUE))
		{
			PROTOCOL_API_LOG(LOG_EVENT_LEVEL_ERR,"系统收到协议头部无效的数据包(0x%X,0x%X,0x%X)...\n\r"
				,pCmd->Header.nType,pCmd->Header.CheckSum,(unsigned int)pCmd->Header.dwLength);
			return FALSE;
		}
		if(Len!=pCmd->Header.dwLength)
		{
			PROTOCOL_API_LOG(LOG_EVENT_LEVEL_ERR,"系统收到无效的协议数据包(%u,%u)...\n\r"
				,(unsigned int)Len,(unsigned int)pCmd->Header.dwLength);
			return FALSE;
		}
	}
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: JSYA_ProtocolMakeCmd,ProtocolMakeCmdEx,ProtocolMakeCmdNew
 *功能描述: 根据协议构造命令报文并自动根据参数设置选择直接网络发送或直接PCI发送或放入命令队列延迟发送
 *输入描述: 基本命令协议头参数、命令内容及其长度、可选扩展协议头参数、PCI模式标记
 *输出描述: 可选输出协议报文
 *返回描述: 成功(TRUE)/失败(FALSE)
 *作者日期: ZCQ/2013/01/22
 *全局声明: 无
 *特殊说明: 协议头重叠参数基本命令协议头优先，目前实现不支持PCI模式
 ************************************************************************************************************************************************************************       
 */
extern BOOL JSYA_ProtocolMakeCmdNew(const PCMD_MAKE_HEADER pCmdHeader,void *pCmdBody,DWORD BodyLen,PCMD_PACKET pCmdBuf,PMMNET_HEADER pCmdHeaderEx,BOOL IsPciModeSend)
{
	PCMD_PACKET pCmd=NULL;
	CMD_PACKET	LocalCmdBuf;

	pCmd=NULL==pCmdBuf?&LocalCmdBuf:pCmdBuf;
	memset(&pCmd->Header,0,sizeof(pCmd->Header));
	pCmd->Header.CheckSum=TCLASS_CHECK_WORD;
	pCmd->Header.DestIP=pCmdHeader->dwDestIP;
	pCmd->Header.dwDevID=pCmdHeader->dwCmdObjID;
	pCmd->Header.dwFormat=pCmdHeader->dwFormat;
	pCmd->Header.dwLength=sizeof(pCmd->Header)+BodyLen;
	pCmd->Header.dwOpCode=pCmdHeader->dwCmdOpCode;
	pCmd->Header.nType=MMNET_M_DATA;
	pCmd->Header.SrcIP=pCmdHeader->dwSrcIP;
	if(NULL!=pCmdHeaderEx)
	{
		pCmd->Header.bReserved=pCmdHeaderEx->bReserved;
		pCmd->Header.dwReserved=pCmdHeaderEx->dwReserved;
		pCmd->Header.dwSeqNum=pCmdHeaderEx->dwSeqNum;
		pCmd->Header.wReserved1=pCmdHeaderEx->wReserved1;
		pCmd->Header.wReserved2=pCmdHeaderEx->wReserved2;
	}
	if(pCmd->Header.dwLength>PROTOCOL_PACKET_MAX_SIZE)
		return FALSE;
	if(BodyLen>0)
		memcpy(&pCmd->COMMAND,pCmdBody,BodyLen);
	if(!ProtocolSwapBodyByteOrder(pCmd,BodyLen>0,FALSE))
	{
		PROTOCOL_API_LOG(LOG_EVENT_LEVEL_WARNING,"协议命令无效或者系统当前不支持(0x%X)...\r\n",pCmd->Header.dwOpCode);
		return FALSE;
	}
	if(!SwapMmnetHeader(&pCmd->Header,FALSE))
	{
		PROTOCOL_API_LOG(LOG_EVENT_LEVEL_ERR,"系统本地构造的协议命令协议头部无效...\r\n");
		return FALSE;
	}

	if(pCmdHeader->IsNetSend)
	{
		if(!SendStream(pCmdHeader->SendTcpFd,pCmd,sizeof(pCmd->Header)+BodyLen))
			return FALSE;
	}
	else
	{
		if(!ProtocolQueueWrite(pCmdHeader->ProtocolQueueID,pCmd,sizeof(pCmd->Header)+BodyLen
			,pCmdHeader->dwUserID,TRUE,NULL,NULL))
		{
			PROTOCOL_API_LOG(LOG_EVENT_LEVEL_ERR,"系统本地协议报文缓冲队列写入失败...\r\n");
			return FALSE;
		}
	}
	return TRUE;
}
extern BOOL JSYA_ProtocolMakeCmdEx(const PCMD_MAKE_HEADER pCmdHeader,void *pCmdBody,DWORD BodyLen,PCMD_PACKET pCmdBuf,PMMNET_HEADER pCmdHeaderEx)
{
	return JSYA_ProtocolMakeCmdNew(pCmdHeader,pCmdBody,BodyLen,pCmdBuf,pCmdHeaderEx,FALSE);
}
extern BOOL JSYA_ProtocolMakeCmd(const PCMD_MAKE_HEADER pCmdHeader,void *pCmdBody,DWORD BodyLen,PCMD_PACKET pCmdBuf)
{
	return JSYA_ProtocolMakeCmdNew(pCmdHeader,pCmdBody,BodyLen,pCmdBuf,NULL,FALSE);
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: JSYA_ProtocolServerGetRunStatus
 *功能描述: 获取协议服务器运行状态
 *输入描述: 无
 *输出描述: 协议服务器运行状态
 *返回描述: 成功(TRUE)/失败(FALSE)
 *作者日期: ZCQ/2007/12/12
 *全局声明: sProtocolMutex,sProtocolServer,sProtocolQueue
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
 */
extern BOOL JSYA_ProtocolServerGetRunStatus(PPROTOCOL_SERVER_PUBLIC_STATUS pStatus)
{
	int		OldStatus;
	DWORD	i;
	
	if(NULL==pStatus)
		return FALSE;
	PTHREAD_MUTEX_SAFE_LOCK(sProtocolMutex,OldStatus);
	if(!sProtocolServer.IsInitReady)
	{
		PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
		return FALSE;
	}
	for(i=0;i<sizeof(pStatus->QueueOverCount)/sizeof(pStatus->QueueOverCount[0]);i++)
		pStatus->QueueOverCount[i]=sProtocolQueue[i].OverCount;
	PTHREAD_MUTEX_SAFE_UNLOCK(sProtocolMutex,OldStatus);
	return TRUE;
}




/******************************************Protocol.c File End******************************************************************************************/
