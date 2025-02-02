/*
 ***************************************************************************************************************************
 ********************************************************************************************************************************
 *			PROGRAM MODULE
 *
 *			$Workfile:		zfy_Platform.c		$
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
 *	实现平台通讯功能
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
#include <stdarg.h>
#include <math.h>
#include <linux/serial.h>
#include "zfy_Platform.h"
#include "zfy_Rola.h"
#include "zfy_Conf.h"
#include "ZFY_Ipc.h"
#include "NetServer.h"
#include "LogEventServer.h"
#include "aes.h"


/*
 ************************************************************************************************************************************************************************
 * 常数定义
 ************************************************************************************************************************************************************************
*/
#define CLOAD_SERVER_USER_NAME						"admin"
#define CLOAD_SERVER_USER_PWD						"admin"
#define CLOAD_SERVER_PORT_STD						1234

#define HOST_IP_IS_VALID(ip)						((ip)!=INADDR_ANY && (ip)!=INADDR_NONE && (ip)!=INADDR_LOOPBACK && (IN_CLASSA(ip) || IN_CLASSB(ip) || IN_CLASSC(ip)))

#define CLOAD_TCP_QUEUE_NODE_SIZE					1560
#define CLOAD_TCP_SEND_QUEUE_SIZE					32
#define CLOAD_TCP_RECV_QUEUE_SIZE					32


/*
 ************************************************************************************************************************************************************************
 类型定义
 ************************************************************************************************************************************************************************
*/
typedef struct _CLOAD_TCP_QUEUE_NODE
{
	DWORD						m_Len;
	BYTE						m_Buf[CLOAD_TCP_QUEUE_NODE_SIZE];
}CLOAD_TCP_QUEUE_NODE,*PCLOAD_TCP_QUEUE_NODE;

typedef struct _CLOAD_TCP_SEND_QUEUE
{
	DWORD						m_CurrCount;
	DWORD						m_OverCount;
	DWORD						m_Head;
	DWORD						m_Tail;
	CLOAD_TCP_QUEUE_NODE		m_Queue[CLOAD_TCP_SEND_QUEUE_SIZE];
}CLOAD_TCP_SEND_QUEUE,*PCLOAD_TCP_SEND_QUEUE;

typedef struct _CLOAD_TCP_RECV_QUEUE
{
	DWORD						m_CurrCount;
	DWORD						m_OverCount;
	DWORD						m_Head;
	DWORD						m_Tail;
	CLOAD_TCP_QUEUE_NODE		m_Queue[CLOAD_TCP_RECV_QUEUE_SIZE];
}CLOAD_TCP_RECV_QUEUE,*PCLOAD_TCP_RECV_QUEUE;

typedef struct _CLOAD_TCP_SERVER
{
	BOOL						m_IsReady;
	BOOL						m_IsReqLocalQuit;
	BOOL						m_IsReqProcQuit;
	int							m_LocalFd;
	pthread_t					m_LocalThreadID;
	pthread_t					m_ProcThreadID;
	CLOAD_TCP_RECV_QUEUE		m_LocalRecvQueue;
	CLOAD_TCP_SEND_QUEUE		m_LocalSendQueue;
	BOOL						m_IsCloadConnected;
	BOOL						m_IsCloadLogin;
	BOOL						m_IsCloadSendData;
	BOOL						m_IsCloadAlarm;
	BOOL						m_IsCloadAlarmAck;
	DWORD						m_CloadLastHeartAckTime;
	char						m_CloadServerIp[16];
	DWORD						m_CloadServerPort;
	char						m_CloadServerUser[64];
	char						m_CloadServerPwd[64];
	char						m_DevIMEI[16];
	char						m_DevIMSI[16];
	DWORD						m_CloadDataLostCount;
}CLOAD_TCP_SERVER,*PCLOAD_TCP_SERVER;


/*
 ************************************************************************************************************************************************************************
 * 全局变量
 ************************************************************************************************************************************************************************
*/
static CLOAD_TCP_SERVER			sCloadTcpServer;
static pthread_mutex_t			sCloadTcpServerMutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t			sCloadTcpQueueMutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t			sCloadTcpWorkMutex=PTHREAD_MUTEX_INITIALIZER;


/*
 ****************************************************************************************************************************************************
 * 函数定义
 *****************************************************************************************************************************************************
*/
extern DWORD ConfigServerGetSysRunTime(BOOL IsInMs)
{
	static pthread_mutex_t	sSysRunTimeMutex=PTHREAD_MUTEX_INITIALIZER;
	static FILE				*pUptimeFile=NULL;
	int						OldStatus;
	DWORD					dwSysRunTime=0;
	double					RealSysRunTime=0;

	PTHREAD_MUTEX_SAFE_LOCK(sSysRunTimeMutex,OldStatus);
	if(NULL==pUptimeFile)
		pUptimeFile=fopen("/proc/uptime","r");
	if(NULL!=pUptimeFile)
	{
		fseek(pUptimeFile,0,SEEK_SET);
		fscanf(pUptimeFile,"%lf",&RealSysRunTime);
		fclose(pUptimeFile);
		dwSysRunTime=(DWORD)(IsInMs?RealSysRunTime*1000:RealSysRunTime);
	}
	pUptimeFile=NULL;
	PTHREAD_MUTEX_SAFE_UNLOCK(sSysRunTimeMutex,OldStatus);
	return dwSysRunTime;
}

static BOOL CloadTcpSendQueuePush(PCLOAD_TCP_SEND_QUEUE pQueue,const PCLOAD_TCP_QUEUE_NODE pNode)
{
	int		OldStatus;

	if(NULL==pQueue || NULL==pNode || pNode->m_Len==0)
		return FALSE;
	PTHREAD_MUTEX_SAFE_LOCK(sCloadTcpQueueMutex,OldStatus);
	if((pQueue->m_Head+1)%CLOAD_TCP_SEND_QUEUE_SIZE==pQueue->m_Tail)
	{
		pQueue->m_OverCount++;
		PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpQueueMutex,OldStatus);
		return FALSE;
	}
	pQueue->m_Queue[pQueue->m_Head]=*pNode;
	pQueue->m_CurrCount++;
	pQueue->m_Head=(pQueue->m_Head+1)%CLOAD_TCP_SEND_QUEUE_SIZE;
	PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpQueueMutex,OldStatus);
	return TRUE;
}

static BOOL CloadTcpSendQueuePopup(PCLOAD_TCP_SEND_QUEUE pQueue,PCLOAD_TCP_QUEUE_NODE pNode)
{
	int		OldStatus;

	if(NULL==pQueue || NULL==pNode)
		return FALSE;
	PTHREAD_MUTEX_SAFE_LOCK(sCloadTcpQueueMutex,OldStatus);
	if(pQueue->m_Tail==pQueue->m_Head)
	{
		PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpQueueMutex,OldStatus);
		return FALSE;
	}
	*pNode=pQueue->m_Queue[pQueue->m_Tail];
	pQueue->m_Tail=(pQueue->m_Tail+1)%CLOAD_TCP_SEND_QUEUE_SIZE;
	PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpQueueMutex,OldStatus);
	return TRUE;
}

static BOOL CloadTcpRecvQueuePush(PCLOAD_TCP_RECV_QUEUE pQueue,const PCLOAD_TCP_QUEUE_NODE pNode)
{
	int		OldStatus;

	if(NULL==pQueue || NULL==pNode || pNode->m_Len==0)
		return FALSE;
	PTHREAD_MUTEX_SAFE_LOCK(sCloadTcpQueueMutex,OldStatus);
	if((pQueue->m_Head+1)%CLOAD_TCP_RECV_QUEUE_SIZE==pQueue->m_Tail)
	{
		pQueue->m_OverCount++;
		PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpQueueMutex,OldStatus);
		return FALSE;
	}
	pQueue->m_Queue[pQueue->m_Head]=*pNode;
	pQueue->m_CurrCount++;
	pQueue->m_Head=(pQueue->m_Head+1)%CLOAD_TCP_RECV_QUEUE_SIZE;
	PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpQueueMutex,OldStatus);
	return TRUE;
}

static BOOL CloadTcpRecvQueuePopup(PCLOAD_TCP_RECV_QUEUE pQueue,PCLOAD_TCP_QUEUE_NODE pNode)
{
	int		OldStatus;

	if(NULL==pQueue || NULL==pNode)
		return FALSE;
	PTHREAD_MUTEX_SAFE_LOCK(sCloadTcpQueueMutex,OldStatus);
	if(pQueue->m_Tail==pQueue->m_Head)
	{
		PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpQueueMutex,OldStatus);
		return FALSE;
	}
	*pNode=pQueue->m_Queue[pQueue->m_Tail];
	pQueue->m_Tail=(pQueue->m_Tail+1)%CLOAD_TCP_RECV_QUEUE_SIZE;
	PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpQueueMutex,OldStatus);
	return TRUE;
}

static WORD CRC_16_S_CCIT_FALSE(const BYTE *di, DWORD len)
{
    WORD crc_poly = 0x1021;  //X^16+X^12+X^5+1 total 16 effective bits without X^16. Computed total data shall be compensated 16-bit '0' before CRC computing.
	DWORD clen = len+2;
	BYTE cdata[clen];
	
	memcpy(cdata, di, len); cdata[len]=0; cdata[len+1]=0;
	cdata[0] ^= 0xff; cdata[1] ^= 0xff;

	WORD data_t = (((WORD)cdata[0]) << 8) + cdata[1]; //CRC register

    for (uint32_t i = 2; i < clen; i++)
    {
        for (BYTE j = 0; j <= 7; j++)
        {
            if(data_t&0x8000)
            	data_t = ( (data_t<<1) | ( (cdata[i]>>(7-j))&0x01) ) ^ crc_poly;
            else
            	data_t = ( (data_t<<1) | ( (cdata[i]>>(7-j))&0x01) ) ;
        }
    }
    return data_t;
}

static BOOL PProtocolAnalyse1F(const BYTE *pBuf,DWORD *pLen,BOOL *pIsCutted)
{
	DWORD	DataLen;
	WORD  U16Temp;
	WORD  crc16;

	if(pBuf!=NULL && pLen!=NULL && pIsCutted!=NULL)
	{
		if(pBuf[0]==0x1f)		
		{
			if(*pLen<16)
			{
				*pIsCutted=TRUE;
				return TRUE;
			}
			else
			{
				DataLen=pBuf[12];
				printf("-P--1--DataLen=%d----\r\n",DataLen);
				DataLen=DataLen<<8;
				printf("-P-2---DataLen=%d----\r\n",DataLen);
				DataLen+=pBuf[13];
				printf("-P--3--DataLen=%d----\r\n",DataLen);
				if(*pLen<(DWORD)(DataLen+16))	
				{
					*pIsCutted=TRUE;
					return TRUE;
				}
				else
				{
					crc16 = pBuf[14+DataLen]<<8 | pBuf[15+DataLen];
					U16Temp = CRC_16_S_CCIT_FALSE(pBuf,14+DataLen);
					printf("--P--crc16=0x%x---U16Temp=0x%x----\r\n",crc16,U16Temp);
					if((pBuf[16+DataLen]==0xF1) && (crc16 == U16Temp))
					{
						*pLen=DataLen+16;
						*pIsCutted=FALSE;
						return TRUE;
					}
				}
			}
		}
	}

	if(pIsCutted!=NULL)
		*pIsCutted=FALSE;
	return FALSE;
}

static BOOL CloadParseRecvData(const BYTE *pBuf,const DWORD Len)
{
	static BYTE				RecvBuf[2048];
	static DWORD			RecvLen=0;
	DWORD					i,pos,TempLen,ComDataLen,OverPushLen;
	BOOL					IsCutted=FALSE;
	int						j=0,OldStatus;
	BOOL					Ret=FALSE;
	DWORD					CurrSysRunTime=0;
	

	if((Len+RecvLen)<=sizeof(RecvBuf))
	{
		memcpy(&RecvBuf[RecvLen],pBuf,Len);
		RecvLen += Len;
	}
	else
	{
		sCloadTcpServer.m_CloadDataLostCount += Len+RecvLen-sizeof(RecvBuf);
		memcpy(&RecvBuf[RecvLen],pBuf,sizeof(RecvBuf)-RecvLen);
		RecvLen=sizeof(RecvBuf);
	}
	for(pos=0;pos<RecvLen;)
	{
		TempLen=RecvLen-pos;
		if(PProtocolAnalyse1F(&RecvBuf[pos],&TempLen,&IsCutted))
		{
			if(IsCutted)
			{
				if(pos>0)
				{
					memcpy(RecvBuf,&RecvBuf[pos],RecvLen-pos);
					RecvLen -= pos;
				}
				return FALSE;
			}
			else
			{
				struct AES_ctx ctx;
				int num,j=0;
				DWORD datalen = 0;
				BYTE key[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};
				
				PTHREAD_MUTEX_SAFE_LOCK(sCloadTcpWorkMutex,OldStatus);
				
				datalen = RecvBuf[pos+12]<<8 |RecvBuf[pos+13];
				AES_init_ctx(&ctx, key);
				printf("-------datalen=%d-------\r\n",datalen);
				num = datalen/16;
				for(j=0;j<num;j++)
				{
					AES_ECB_decrypt(&ctx, &RecvBuf[pos+j*16+14]);
				}
				//if(RecvBuf[pos+1]==0xff)	//addr
				{
					printf("-----type=0x%x------\r\n",RecvBuf[pos+11]);
					switch(RecvBuf[pos+11])
					{
					case 0x7A:
						{
							BYTE SendBuf[256]={0};
							WORD Count=0,MsgLen=16;
							BYTE DevMsgId=0x0A;
							BYTE MsgBody[16]={0};
							WORD U16Temp=0;
							WORD U16CRC=0;
							
							SendBuf[0]=0x1F;
							SendBuf[9]=(Count<<8)&0xff;
							SendBuf[10]=Count&0xff;
							SendBuf[11]=DevMsgId;
							SendBuf[12]=(MsgLen<<8)&0xff;;
							SendBuf[13]=MsgLen&0xff;;
							AES_init_ctx(&ctx, key);
							num = MsgLen/16;
							for(j=0;j<num;j++)
							{
								AES_ECB_encrypt(&ctx, &MsgBody[j*16]);
							}
							memcpy(&SendBuf[14],MsgBody,sizeof(MsgBody));
							U16CRC = CRC_16_S_CCIT_FALSE(SendBuf,14+sizeof(MsgBody));
							SendBuf[14+sizeof(MsgBody)]=U16CRC>>8;
							SendBuf[15+sizeof(MsgBody)]=U16CRC&0xff;
							SendBuf[16+sizeof(MsgBody)]=0xF1;
							
							if(sCloadTcpServer.m_IsCloadConnected)
							{
								CLOAD_TCP_QUEUE_NODE	SendNode;

								memset(&SendNode,0,sizeof(SendNode));
								memcpy(SendNode.m_Buf,SendBuf,17+sizeof(MsgBody));
								SendNode.m_Len=17+sizeof(MsgBody);
								CloadTcpSendQueuePush(&sCloadTcpServer.m_LocalSendQueue,&SendNode);
							}
						}
						break;
					case 0x7B:
						sCloadTcpServer.m_IsCloadLogin=TRUE;
						sCloadTcpServer.m_IsCloadAlarm=TRUE;
						sCloadTcpServer.m_IsCloadSendData=TRUE;
						break;
					case 0x7C:
						break;
					default:
						ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_NOTICE,"***protocol(Cmd=0x%X)...\r\n",RecvBuf[pos+3]);
						break;
					}
				}
				//else
				//	ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_NOTICE,"***unspported protocol(Addr=0x%X)...\r\n",RecvBuf[i+1]);
				PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpWorkMutex,OldStatus);
				
				pos += TempLen;
			}
			
		}
		else
		{
			pos++;
			sCloadTcpServer.m_CloadDataLostCount++;
			Ret=FALSE;
		}
	}
	
	RecvLen=0;
	return Ret;
}

static void *CloadTcpProcThread(void *pArg)
{
	PCLOAD_TCP_SERVER			pServer=(PCLOAD_TCP_SERVER)pArg;
	BOOL						IsRecv=FALSE,IsSend=FALSE;
	struct in_addr				CloatServerIP;
	CLOAD_TCP_QUEUE_NODE		RecvNode;
	int 						i=0,j=0,OldStatus,tempLen=1024;
	DWORD						QueryCount=0,OutLen=1024,AlarmTimeOut=0;
	DWORD						CurrLoginTime=0,LastLoginTime=0,CurrSendDataTime=0,LastSendDataTime=0,CurrSendAlarmTime=0,LastSendAlarmTime=0;
	DWORD						CurrHeartTime=0,LastHeartTime=0,CurrPreLoginTime=0,LastPreLoginTime=0;
	char						strIMSIcode[256]={0};
	char						strIMEI[128]={0};
	char						strManufacturer[128]={0};
	char						strProductModel[128]={0};
	char						strOperType[128]={0};
	struct AES_ctx 				ctx;
	int 						num;
	DWORD 						datalen = 0;
	BYTE key[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};	
	
	ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_INFO,"******CLOAD-TCP thread(PID=%d,PTID=%u)...\r\n",getpid(),pthread_self());
	sleep(1);
	
	for(;;)
	{
		if(pServer->m_IsReqProcQuit)
		{
			pServer->m_IsReqProcQuit=FALSE;
			break;
		}
		pthread_testcancel();

		if(pServer->m_IsCloadConnected)
		{
			
			{
				DWORD	CurrHCheckTime=0;
				
				GET_CURRENT_SYS_TIME(CurrHCheckTime);
				if(CurrHCheckTime-sCloadTcpServer.m_CloadLastHeartAckTime>300)
				{
					PTHREAD_MUTEX_SAFE_LOCK(sCloadTcpWorkMutex,OldStatus);
					sCloadTcpServer.m_IsCloadLogin=FALSE;
					PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpWorkMutex,OldStatus);
					ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_CRIT,"heartbeat timeout...\r\n");
					sCloadTcpServer.m_CloadLastHeartAckTime=CurrHCheckTime;
				}
			}
			if(!pServer->m_IsCloadLogin)
			{
				memset(&RecvNode,0,sizeof(RecvNode));
				if(RecvNode.m_Len==0)
				{
					if(!CloadTcpRecvQueuePopup(&pServer->m_LocalRecvQueue,&RecvNode))
						RecvNode.m_Len=0;
				}
				if(RecvNode.m_Len!=0)
				{
					ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_INFO,"CLOAD-TCP recv=%d-%x-%x-%x-%x----%s---\r\n",RecvNode.m_Len,RecvNode.m_Buf[0],RecvNode.m_Buf[1],RecvNode.m_Buf[2],RecvNode.m_Buf[3],&RecvNode.m_Buf[3]);
					CloadParseRecvData(RecvNode.m_Buf,RecvNode.m_Len);
					RecvNode.m_Len=0;
				}
				GET_CURRENT_SYS_TIME(CurrLoginTime);
				if(CurrLoginTime-LastLoginTime>180)
				{
					if(pServer->m_IsCloadConnected)
					{
						BYTE SendBuf[256]={0};
						WORD Count=0,MsgLen=16;
						BYTE DevMsgId=0x0A;
						BYTE MsgBody[16]={0};
						WORD U16Temp=0;
						WORD U16CRC=0;
						
						SendBuf[0]=0x1F;
						memset(&SendBuf[1],0xff,8);
						SendBuf[9]=(Count<<8)&0xff;
						SendBuf[10]=Count&0xff;
						SendBuf[11]=DevMsgId;
						SendBuf[12]=(MsgLen<<8)&0xff;;
						SendBuf[13]=MsgLen&0xff;;
						AES_init_ctx(&ctx, key);
						num = MsgLen/16;
						for(j=0;j<num;j++)
						{
							AES_ECB_encrypt(&ctx, &MsgBody[j*16]);
						}
						memcpy(&SendBuf[14],MsgBody,sizeof(MsgBody));
						U16CRC = CRC_16_S_CCIT_FALSE(SendBuf,14+sizeof(MsgBody));
						SendBuf[14+sizeof(MsgBody)]=U16CRC>>8;
						SendBuf[15+sizeof(MsgBody)]=U16CRC&0xff;
						SendBuf[16+sizeof(MsgBody)]=0xF1;
						
						if(sCloadTcpServer.m_IsCloadConnected)
						{
							CLOAD_TCP_QUEUE_NODE SendNode;

							memset(&SendNode,0,sizeof(SendNode));
							memcpy(SendNode.m_Buf,SendBuf,17+sizeof(MsgBody));
							SendNode.m_Len=17+sizeof(MsgBody);
							CloadTcpSendQueuePush(&sCloadTcpServer.m_LocalSendQueue,&SendNode);
						}
					}
					
					GET_CURRENT_SYS_TIME(LastLoginTime);
					LastSendDataTime=0;
				}
			}
			GET_CURRENT_SYS_TIME(CurrLoginTime);
			GET_CURRENT_SYS_TIME(CurrHeartTime);
			GET_CURRENT_SYS_TIME(CurrSendAlarmTime);
			
			if(pServer->m_IsCloadSendData)
			{
				GET_CURRENT_SYS_TIME(CurrSendDataTime);
				if(CurrSendDataTime-LastSendDataTime>(LastSendDataTime?15*60:300))
				{
					
					GET_CURRENT_SYS_TIME(LastSendDataTime);
				}
			}

			
			PTHREAD_MUTEX_SAFE_LOCK(sCloadTcpWorkMutex,OldStatus);
			if(pServer->m_IsCloadAlarmAck)
			{
				
			}
			PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpWorkMutex,OldStatus);
			memset(&RecvNode,0,sizeof(RecvNode));
			if(RecvNode.m_Len==0)
			{
				if(!CloadTcpRecvQueuePopup(&pServer->m_LocalRecvQueue,&RecvNode))
					RecvNode.m_Len=0;
			}
			if(RecvNode.m_Len!=0)
			{
				ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_INFO,"CLOAD-TCP recv=%d-%x-%x-%x-%x----%s---\r\n",RecvNode.m_Len,RecvNode.m_Buf[0],RecvNode.m_Buf[1],RecvNode.m_Buf[2],RecvNode.m_Buf[3],&RecvNode.m_Buf[3]);
				CloadParseRecvData(RecvNode.m_Buf,RecvNode.m_Len);
				RecvNode.m_Len=0;
			}
		}
		
		MSLEEP(100);
	}
}

static void *CloadTcpNetThread(void *pArg)
{
	PCLOAD_TCP_SERVER			pServer=(PCLOAD_TCP_SERVER)pArg;
	int							i,ReadyFdNum;
	DWORD						TimeoutMs;
	DWORD						CurrRunTime;
	struct sockaddr_in			RemoteAddr;
	fd_set						ReadSet,WriteSet;
	CLOAD_TCP_QUEUE_NODE		SendNode;

	ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_INFO,"******CLOAD-TCP net thread(PID=%d,PTID=%u)...\r\n",getpid(),pthread_self());
	sleep(10);
	memset(&RemoteAddr,0,sizeof(RemoteAddr));
	RemoteAddr.sin_family=AF_INET;
	RemoteAddr.sin_addr.s_addr=inet_addr(sCloadTcpServer.m_CloadServerIp);
	RemoteAddr.sin_port=htons(sCloadTcpServer.m_CloadServerPort);
	memset(&SendNode,0,sizeof(SendNode));
	TimeoutMs=(1000+GET_SYS_SCHED_CLOCK_TICK_FREQUENCY-1)/GET_SYS_SCHED_CLOCK_TICK_FREQUENCY;
	
	for(;;)
	{
		if(pServer->m_IsReqLocalQuit)
		{
			pServer->m_IsReqLocalQuit=FALSE;
			break;
		}
		pthread_testcancel();
		sleep(1);
		
		if(pServer->m_LocalFd==SOCKET_INVALID_FD)
			pServer->m_LocalFd=CreateSocket(SOCKET_TYPE_TCP,SOCKET_TCP_MODE_CLIENT,NULL,&RemoteAddr,INVALID_DEV_NET_ID);
		if(pServer->m_LocalFd==SOCKET_INVALID_FD)
		{
			ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_ERR,"CLOAD-TCP create socket failed...\r\n");
			sleep(15);
			continue;
		}
		ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_NOTICE,"CLOAD-TCP socket connected...\r\n");
		pServer->m_IsCloadConnected=TRUE;
		
		for(;;)
		{
			if(pServer->m_IsReqLocalQuit)
				break;
			pthread_testcancel();

			GET_CURRENT_SYS_TIME(CurrRunTime);
			
			FD_ZERO(&ReadSet);
			FD_ZERO(&WriteSet);

			if(pServer->m_LocalFd!=SOCKET_INVALID_FD)
			{
				FD_SET(pServer->m_LocalFd,&ReadSet);
				FD_SET(pServer->m_LocalFd,&WriteSet);
			}
			if(!WaitForSocket(pServer->m_LocalFd,&ReadSet,&WriteSet,&TimeoutMs,&ReadyFdNum))
			{
				MSLEEP(TimeoutMs);
				continue;
			}
			if(ReadyFdNum==0)
			{
				MSLEEP(TimeoutMs);
				continue;
			}
		
			if(pServer->m_LocalFd!=SOCKET_INVALID_FD)
			{
				if(FD_ISSET(pServer->m_LocalFd,&WriteSet))
				{
					if(SendNode.m_Len==0)
					{
						if(!CloadTcpSendQueuePopup(&pServer->m_LocalSendQueue,&SendNode))
							SendNode.m_Len=0;
					}
					if(SendNode.m_Len!=0)
					{
						if(!ZFY_NET_SendStream(pServer->m_LocalFd,SendNode.m_Buf,SendNode.m_Len))
						{
							ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_NOTICE,"CLOAD-TCP send failed...\r\n");
							KillSocket(&pServer->m_LocalFd);
							pServer->m_LocalFd=SOCKET_INVALID_FD;
							pServer->m_IsCloadConnected=FALSE;
							SendNode.m_Len=0;
							break;
						}
						SendNode.m_Len=0;
					}
				}
				if(FD_ISSET(pServer->m_LocalFd,&ReadSet))
				{
					int							InLen=0;
					BYTE						RecvBuf[1460];
					DWORD						RecvLen=1460,steplen=1024,curpos=0;
					CLOAD_TCP_QUEUE_NODE		RecvNode;

					RecvNode.m_Len=sizeof(RecvNode.m_Buf);
					if(ioctl(pServer->m_LocalFd,FIONREAD,(char *)&InLen)!=STD_SUCCESS)
					{
						ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_ERR,"CLOAD-TCP ioctl(%s) failed...\r\n",strerror(errno));
						KillSocket(&pServer->m_LocalFd);
						pServer->m_LocalFd=SOCKET_INVALID_FD;
						pServer->m_IsCloadConnected=FALSE;
						break;
					}
					if(InLen==0)
					{
						ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_NOTICE,"CLOAD-TCP len==0...\r\n");
						KillSocket(&pServer->m_LocalFd);
						pServer->m_LocalFd=SOCKET_INVALID_FD;
						pServer->m_IsCloadConnected=FALSE;
						break;
					}
					if((DWORD)InLen<RecvLen)
						RecvLen=InLen;
					if(!ZFY_NET_RecvStream(pServer->m_LocalFd,RecvBuf,RecvLen))
					{
						ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_NOTICE,"CLOAD-TCP recv failed...\r\n");
						KillSocket(&pServer->m_LocalFd);
						pServer->m_LocalFd=SOCKET_INVALID_FD;
						pServer->m_IsCloadConnected=FALSE;
						break;
					}
					ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_CRIT,"***CLOAD-TCP recv len=%d...\r\n",RecvLen);
					while(RecvLen)
					{
						if(RecvLen>steplen)
						{
							memset(&RecvNode,0,sizeof(RecvNode));
							memcpy(RecvNode.m_Buf,&RecvBuf[curpos],steplen);
							RecvNode.m_Len=steplen;
							curpos+=steplen;
							RecvLen-=steplen;
							CloadTcpRecvQueuePush(&pServer->m_LocalRecvQueue,&RecvNode);
						}
						else
						{
							memset(&RecvNode,0,sizeof(RecvNode));
							memcpy(RecvNode.m_Buf,&RecvBuf[curpos],RecvLen);
							RecvNode.m_Len=RecvLen;
							RecvLen=0;
							CloadTcpRecvQueuePush(&pServer->m_LocalRecvQueue,&RecvNode);
						}
					}	
				}
			}
			MSLEEP(10);
		}
	}

	return NULL;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_CloadServerStatus
 *功能描述: 平台协议模块状态查询
 *输入描述: 无
 *输出描述: 无
 *返回描述: TRUE/FALSE
 *作者日期: LJJ/2024/12/02
 *全局声明: sCloadTcpServer
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_CloadServerStatus(BOOL *pIsAlive, BOOL *pIsLogin)
{
	if(pIsAlive!=NULL)
		*pIsAlive=sCloadTcpServer.m_IsCloadConnected;
	if(pIsLogin!=NULL)
		*pIsLogin=sCloadTcpServer.m_IsCloadLogin;
	if(sCloadTcpServer.m_IsReady)
		return TRUE;
	return FALSE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_CloadServerOpen
 *功能描述: 平台协议模块打开
 *输入描述: 配置参数
 *输出描述: 无
 *返回描述: TRUE/FALSE
 *作者日期: LJJ/2024/12/02
 *全局声明: sCloadTcpServerMutex,sCloadTcpServer
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_CloadServerOpen(PCLOAD_CONF_CONF pConf)
{
	int					i,ret,OldStatus;
	struct in_addr		CloadServerIP;
	
	printf("-------ZFY_CloadServerOpen------\r\n");
	PTHREAD_MUTEX_SAFE_LOCK(sCloadTcpServerMutex,OldStatus);
	if(sCloadTcpServer.m_IsReady)
	{
		ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_NOTICE,"CLOAD-TCP is ready...\r\n");
		PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpServerMutex,OldStatus);
		return TRUE;
	}
	memset(&sCloadTcpServer,0,sizeof(sCloadTcpServer));
	sCloadTcpServer.m_IsReqLocalQuit=FALSE;
	sCloadTcpServer.m_LocalFd=SOCKET_INVALID_FD;
	sCloadTcpServer.m_LocalThreadID=INVALID_PTHREAD_ID;
	sCloadTcpServer.m_ProcThreadID=INVALID_PTHREAD_ID;
	
	if(NULL==pConf || NULL==pConf->strCloadServerIP)
	{
		ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_NOTICE,"CLOAD-TCP param is err...\r\n");
		PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpServerMutex,OldStatus);
		return FALSE;
	}
	if(NULL==pConf->strLoginUser)
		pConf->strLoginUser=CLOAD_SERVER_USER_NAME;
	if(NULL==pConf->strLoginPwd)
		pConf->strLoginPwd=CLOAD_SERVER_USER_PWD;
	
	sCloadTcpServer.m_CloadServerPort=pConf->CloadServerPort==0?CLOAD_SERVER_PORT_STD:pConf->CloadServerPort;
	strncpy(sCloadTcpServer.m_CloadServerIp,pConf->strCloadServerIP,sizeof(sCloadTcpServer.m_CloadServerIp)-1);
	strncpy(sCloadTcpServer.m_CloadServerUser,pConf->strLoginUser,sizeof(sCloadTcpServer.m_CloadServerUser)-1);
	strncpy(sCloadTcpServer.m_CloadServerPwd,pConf->strLoginPwd,sizeof(sCloadTcpServer.m_CloadServerPwd)-1);
	strncpy(sCloadTcpServer.m_DevIMEI,pConf->strDevIMEI,sizeof(sCloadTcpServer.m_DevIMEI)-1);
	strncpy(sCloadTcpServer.m_DevIMSI,pConf->strDevIMSI,sizeof(sCloadTcpServer.m_DevIMSI)-1);
	
	if((ret=pthread_create(&sCloadTcpServer.m_LocalThreadID,NULL,CloadTcpNetThread,&sCloadTcpServer))!=STD_SUCCESS)
	{
		ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_ERR,"CLOAD-TCP net thread failed(%d)...\r\n",ret);
		PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpServerMutex,OldStatus);
		return FALSE;
	}
	if((ret=pthread_create(&sCloadTcpServer.m_ProcThreadID,NULL,CloadTcpProcThread,&sCloadTcpServer))!=STD_SUCCESS)
	{
		ZFY_LES_LogPrintf("CLOAD-TCP",LOG_EVENT_LEVEL_ERR,"CLOAD-TCP tcp thread failed(%d)...\r\n",ret);
		PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpServerMutex,OldStatus);
		return FALSE;
	}
	sCloadTcpServer.m_IsReady=TRUE;
	PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpServerMutex,OldStatus);
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_CloadServerClose
 *功能描述: 平台协议模块关闭
 *输入描述: 无
 *输出描述: 无
 *返回描述: 无
 *作者日期: LJJ/2024/12/02
 *全局声明: sCloadTcpServerMutex,sCloadTcpServer
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern void ZFY_CloadServerClose(void)
{
	int		i,OldStatus;

	printf("-------ZFY_CloadServerClose------\r\n");
	PTHREAD_MUTEX_SAFE_LOCK(sCloadTcpServerMutex,OldStatus);
	if(sCloadTcpServer.m_IsReady)
	{
		sCloadTcpServer.m_IsReqLocalQuit=TRUE;
		sCloadTcpServer.m_IsReqProcQuit=TRUE;
		usleep(PTHREAD_DEFAULT_QUIT_TIMEOUT_US);
		if(sCloadTcpServer.m_IsReqLocalQuit)
			pthread_cancel(sCloadTcpServer.m_LocalThreadID);
		pthread_join(sCloadTcpServer.m_LocalThreadID,NULL);
		if(sCloadTcpServer.m_IsReqProcQuit)
			pthread_cancel(sCloadTcpServer.m_ProcThreadID);
		pthread_join(sCloadTcpServer.m_ProcThreadID,NULL);
		if(sCloadTcpServer.m_LocalFd!=SOCKET_INVALID_FD)
			KillSocket(&sCloadTcpServer.m_LocalFd);
		sCloadTcpServer.m_IsReady=FALSE;
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sCloadTcpServerMutex,OldStatus);
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_4GModelPowerCtrl
 *功能描述: 4G模块电源控制
 *输入描述: 是否上电
 *输出描述: 无
 *返回描述: TRUE/FALSE
 *作者日期: LJJ/2024/12/02
 *全局声明: 无
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_4GModelPowerCtrl(BOOL PowerEnable)
{
	system("echo 97 >/sys/class/gpio/export");  
	system("echo out >/sys/class/gpio/gpio97/direction");	
	if(PowerEnable) 
		system("echo 1 >/sys/class/gpio/gpio97/value");
	else
		system("echo 0 >/sys/class/gpio/gpio97/value");
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_4GModelStartCtrl
 *功能描述: 4G模块开机控制
 *输入描述: 无
 *输出描述: 无
 *返回描述: TRUE/FALSE
 *作者日期: LJJ/2024/12/02
 *全局声明: 无
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_4GModelStartCtrl(void)
{
	system("echo 38 >/sys/class/gpio/export");  
	system("echo out >/sys/class/gpio/gpio38/direction");	
	system("echo 1 >/sys/class/gpio/gpio38/value");
	sleep(1);
	system("echo 0 >/sys/class/gpio/gpio38/value");
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_4GModelResetCtrl
 *功能描述: 4G模块复位控制
 *输入描述: 无
 *输出描述: 无
 *返回描述: TRUE/FALSE
 *作者日期: LJJ/2024/12/02
 *全局声明: 无
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_4GModelResetCtrl(void)
{
	system("echo 40 >/sys/class/gpio/export");  
	system("echo out >/sys/class/gpio/gpio40/direction");	
	system("echo 1 >/sys/class/gpio/gpio40/value");
	sleep(1);
	system("echo 0 >/sys/class/gpio/gpio40/value");
	return TRUE;
}
