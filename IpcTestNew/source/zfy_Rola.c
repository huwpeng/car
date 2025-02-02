/*
 ***************************************************************************************************************************
 ********************************************************************************************************************************
 *			PROGRAM MODULE
 *
 *			$Workfile:		zfy_Rola.c			$
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
 *	实现ROLA模块通讯功能
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
#include "zfy_Rola.h"
#include "zfy_Conf.h"
#include "ZFY_Ipc.h"
#include "LogEventServer.h"
#include "aes.h"


/*
 ************************************************************************************************************************************************************************
 * 常数定义
 ************************************************************************************************************************************************************************
*/
#define SER_DEV_NAME	"/dev/ttyS2"

#define SER_DEV_OUTPUT_QUEUE_SIZE				64
#define SER_DEV_INPUT_QUEUE_SIZE				64
#define SER_DEV_USER_TOTAL						32

#define SER_DEV_INPUT_NALU_SIZE						1024
#define SER_DEV_OUTPUT_NALU_SIZE					1024

#define SER_DEV_UNIT_MIN_IDLE_TIME_MS				2			
#define SER_DEV_UNIT_MAX_IDLE_TIME_MS				10
#define SER_DEV_UNIT_SEND_POLL_TIMEOUT_MIN_MS		5000
#define SER_DEV_POLL_TIMER_MIN_MS					10
#define SER_DEV_POLL_TIMER_MAX_MS					50
#define SER_DEV_UNIT_MIN_TIMEOUT_MS					10
#define SER_DEV_MAX_RETRY_COUNT						180
#define SER_DEV_UART_BAUDRATE						9600

#define SER_DEV_ROLA_MASTER_MODE					1

/*
 ************************************************************************************************************************************************************************
 类型定义
 ************************************************************************************************************************************************************************
*/
typedef struct tSER_DEV_BAUD_RATE_PAIR
{
	int				BaudRateFlag;
	DWORD			BaudRateVal;
}SER_DEV_BAUD_RATE_PAIR,*PSER_DEV_BAUD_RATE_PAIR;

static const SER_DEV_BAUD_RATE_PAIR	sSerDevBaudRateTab[]=
{
	{B50,50},{B75,75},{B110,110},{B134,134},{B150,150},{B200,200},{B300,300},{B600,600},
	{B1200,1200},{B1800,1800},{B2400,2400},{B4800,4800},{B9600,9600},{B19200,19200},{B38400,38400},
	{B57600,57600},{B115200,115200},{B230400,230400},{B460800,460800}
};

typedef struct tSER_UNIT_LOCAL_SET
{
	DWORD						dwBaudRate;
	DWORD						dwDataBits;
	DWORD						dwStopBits;
	DWORD						dwReplyTimeoutMS;
	DWORD						dwRecvTimeoutMS;
	DWORD						dwCurrMRU;
	BOOL						IsVerify;
	BOOL						IsOddVerify;
	BOOL						IsConstVerify;
	BOOL						IsRs485Mode;
	BOOL						IsRawMode;
	BOOL						IsHwFlowCtrl;
	BOOL						IsSoftFlowCtrl;
	BOOL						IsGetOwner;
	BOOL						IsLowDelay;
}SER_UNIT_LOCAL_SET,*PSER_UNIT_LOCAL_SET;

typedef struct tSER_UNIT_STATUS
{
	int							DevDrvHandle;
	BOOL						IsReqSendQuit;
	BOOL						IsReqRecvQuit;
	BOOL						IsReqWorkQuit;
	BOOL						IsInError;
	pthread_t					SendThreadID;
	pthread_t					RecvThreadID;
	pthread_t					WorkThreadID;
	DWORD						SendCount;
	DWORD						RecvCount;
	DWORD						ErrorCount;
	DWORD						ErrLeftCount;
	DWORD						ErrLeftLenAll;
	char						UartDevName[256];
}SER_UNIT_STATUS,*PSER_UNIT_STATUS;

typedef struct tSER_DEV_SERVER
{
	BOOL						IsReqQuit;
	BOOL						IsReqAlarmQuit;
	BOOL						IsInitReady;
	BOOL						IsAlarmTrig;
	pthread_t					AlarmThreadID;
	pthread_t					ManagerThreadID;
	pthread_t					CallerThread;
	__pid_t						CallerPID;
	int							Rs485DevDrvHandle;
	int							GpsDevDrvHandle;
	DWORD						DataLostCount;
	SER_UNIT_LOCAL_SET			UnitSet;
	SER_UNIT_STATUS				UnitStatus;
}SER_DEV_SERVER,*PSER_DEV_SERVER;

typedef struct tSER_DEV_OUTPUT_QUEUE_NODE
{
	DWORD						dwDevUnit;
	DWORD						dwUserIndex;
	DWORD						dwLen;
	void						*pBuf;
}SER_DEV_OUTPUT_QUEUE_NODE,*PSER_DEV_OUTPUT_QUEUE_NODE;

typedef struct tSER_DEV_USER_STATUS
{
	BOOL						IsEnable;
	DWORD						Tail;
	DWORD						Count;
	DWORD						OverCount;
}SER_DEV_USER_STATUS,*PSER_DEV_USER_STATUS;

typedef struct tSER_DEV_OUTPUT_QUEUE
{
	DWORD						Head;
	DWORD						OverCount;
	BOOL						IsCondInited;
	BOOL						IsReady;
	pthread_cond_t				ReadyCond;
	SER_DEV_USER_STATUS			UserTab[32];
	SER_DEV_OUTPUT_QUEUE_NODE	Queue[SER_DEV_OUTPUT_QUEUE_SIZE];
}SER_DEV_OUTPUT_QUEUE,*PSER_DEV_OUTPUT_QUEUE;

typedef struct tSER_DEV_INPUT_QUEUE_NODE
{
	DWORD						dwUserIndex;
	DWORD						dwLen;
	void						*pBuf;
	BOOL						IsTransData;
}SER_DEV_INPUT_QUEUE_NODE,*PSER_DEV_INPUT_QUEUE_NODE;

typedef struct tSER_DEV_INPUT_QUEUE
{
	DWORD						dwHead;
	DWORD						dwTail;
	DWORD						dwCount;
	DWORD						dwOverCount;
	BOOL						bIsCondInited;
	BOOL						bIsReady;
	pthread_cond_t				ReadyCond;
	pthread_cond_t				WriteCond;
	SER_DEV_INPUT_QUEUE_NODE	Queue[SER_DEV_INPUT_QUEUE_SIZE];
}SER_DEV_INPUT_QUEUE,*PSER_DEV_INPUT_QUEUE;


/*
 ************************************************************************************************************************************************************************
 * 全局变量
 ************************************************************************************************************************************************************************
*/
static pthread_mutex_t	sSerProMutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t	sSerDevMutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t	sSerDevHwMutex=PTHREAD_MUTEX_INITIALIZER;
static SER_DEV_SERVER	sSerDevServer;

static pthread_mutex_t		sSerOutputQueueMutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t		sSerInputQueueMutex=PTHREAD_MUTEX_INITIALIZER;
static SER_DEV_OUTPUT_QUEUE	sSerDevOutputQueue;
static SER_DEV_INPUT_QUEUE	sSerDevInputQueue;


/*
 ****************************************************************************************************************************************************
 * 函数定义
 *****************************************************************************************************************************************************
*/
static int SerDevHwUnitOpen(const char *pName,const PSER_UNIT_LOCAL_SET pConfig)
{
	int						DevHandle=STD_INVALID_HANDLE;
	DWORD					i;
	BOOL					IsFindBaudRate;
	struct termios			Option;
	struct serial_struct	serial;

	DevHandle=open(pName,O_RDWR|O_NOCTTY);
	if(DevHandle==STD_INVALID_HANDLE)
		return DevHandle;
	if(pConfig->IsLowDelay)
	{
		if(ioctl(DevHandle,TIOCGSERIAL,&serial)==STD_SUCCESS)
		{
			serial.flags |= ASYNC_LOW_LATENCY;
			if(ioctl(DevHandle,TIOCSSERIAL,&serial)!=STD_SUCCESS)
				ZFY_LES_LogPrintf("ROLA",LOG_EVENT_LEVEL_WARNING,"serial ioctl (%s)...\r\n",pName,strerror(errno));
		}
	}
	if(tcgetattr(DevHandle,&Option)!=STD_SUCCESS)
	{
		close(DevHandle);
		return STD_INVALID_HANDLE;
	}
	for(i=0,IsFindBaudRate=FALSE;i<sizeof(sSerDevBaudRateTab)/sizeof(sSerDevBaudRateTab[0]);i++)
	{
		if(sSerDevBaudRateTab[i].BaudRateVal==pConfig->dwBaudRate)
		{
			if(cfsetispeed(&Option,sSerDevBaudRateTab[i].BaudRateFlag)!=STD_SUCCESS)
			{
				close(DevHandle);
				return STD_INVALID_HANDLE;
			}
			if(cfsetospeed(&Option,sSerDevBaudRateTab[i].BaudRateFlag)!=STD_SUCCESS)
			{
				close(DevHandle);
				return STD_INVALID_HANDLE;
			}
			IsFindBaudRate=TRUE;
			break;
		}
	}
	if(!IsFindBaudRate)
	{
		close(DevHandle);
		return STD_INVALID_HANDLE;
	}

	switch(pConfig->dwDataBits)
	{
		case 5:
			Option.c_cflag &= ~CSIZE;
			Option.c_cflag |= CS5;
			break;
		case 6:
			Option.c_cflag &= ~CSIZE;
			Option.c_cflag |= CS6;
			break;
		case 7:
			Option.c_cflag &= ~CSIZE;
			Option.c_cflag |= CS7;
			break;
		case 8:
			Option.c_cflag &= ~CSIZE;
			Option.c_cflag |= CS8;
			break;
		default:
			close(DevHandle);
			return STD_INVALID_HANDLE;
	}

	if(pConfig->dwStopBits==2)
		Option.c_cflag |= CSTOPB;
	else if(pConfig->dwStopBits==1)
		Option.c_cflag &= ~CSTOPB;
	else
	{
		close(DevHandle);
		return STD_INVALID_HANDLE;
	}

	if(pConfig->IsVerify)
	{
		Option.c_cflag |= PARENB;
		Option.c_iflag |= INPCK;
	}
	else
	{
		Option.c_cflag &= ~PARENB;
		Option.c_iflag &= ~INPCK;
	}
	if(pConfig->IsConstVerify)
	{
		Option.c_cflag |= CMSPAR;
		Option.c_iflag |= (IGNBRK|IGNPAR);
	}
	else
	{
		Option.c_cflag &= ~CMSPAR;
		Option.c_iflag &= ~(IGNBRK|IGNPAR);
	}
	if(pConfig->IsOddVerify)
		Option.c_cflag |= PARODD;
	else
		Option.c_cflag &= ~PARODD;

	Option.c_cflag |= (CLOCAL|CREAD);
	if(pConfig->IsHwFlowCtrl)
		Option.c_cflag |= CRTSCTS;
	else
		Option.c_cflag &= ~CRTSCTS;

	if(pConfig->IsGetOwner)
	{
		Option.c_cflag &= ~CLOCAL;
		Option.c_cflag |= HUPCL;
	}

	if(pConfig->IsRawMode)
	{
		Option.c_lflag &= ~(ICANON|ECHO|ECHOE|ISIG);
		Option.c_oflag &= ~OPOST;
		Option.c_iflag &= ~(ISTRIP|INLCR|IGNCR|ICRNL|IUCLC);
	}
	else
	{
		Option.c_lflag |= (ICANON|ECHO|ECHOE|ISIG);
		Option.c_oflag |= OPOST;
	}

	if(pConfig->IsSoftFlowCtrl)
		Option.c_iflag |= (IXON|IXOFF|IXANY);
	else
		Option.c_iflag &= ~(IXON|IXOFF|IXANY);
	
	if(pConfig->IsLowDelay)
	{
		Option.c_cc[VMIN]=1;
		Option.c_cc[VTIME]=0;
	}
	else
	{
		Option.c_cc[VMIN]=0;
		Option.c_cc[VTIME]=1;
	}
	if(tcsetattr(DevHandle,TCSANOW,&Option)!=STD_SUCCESS)
	{
		close(DevHandle);
		return STD_INVALID_HANDLE;
	}
	return DevHandle;
}

static DWORD SerDevUnitSendTimeOut(const PSER_UNIT_LOCAL_SET pUnitSet,DWORD Len,BOOL IsStrict)
{
	DWORD	TimeOut;

	if(IsStrict)
	{
		TimeOut=SER_DEV_UNIT_MIN_IDLE_TIME_MS+((1+pUnitSet->dwDataBits+pUnitSet->dwStopBits
			+(pUnitSet->IsVerify?1:0))*Len*1000+pUnitSet->dwBaudRate/2)/pUnitSet->dwBaudRate;
	}
	else
	{
		TimeOut=SER_DEV_UNIT_MAX_IDLE_TIME_MS+((1+pUnitSet->dwDataBits+pUnitSet->dwStopBits
			+(pUnitSet->IsVerify?1:0))*Len*1000+pUnitSet->dwBaudRate/2)/pUnitSet->dwBaudRate*2;
	}

	return TimeOut;
}

static DWORD SerDevUnitRecvTimeOut(const PSER_UNIT_LOCAL_SET pUnitSet,DWORD Len,BOOL IsStrict)
{
	DWORD	TimeOut;

	if(IsStrict)
	{
		TimeOut=SER_DEV_UNIT_MIN_IDLE_TIME_MS+((1+pUnitSet->dwDataBits+pUnitSet->dwStopBits
			+(pUnitSet->IsVerify?1:0))*Len*1000+pUnitSet->dwBaudRate/2)/pUnitSet->dwBaudRate;
	}
	else
	{
		TimeOut=SER_DEV_UNIT_MAX_IDLE_TIME_MS+((1+pUnitSet->dwDataBits+pUnitSet->dwStopBits
			+(pUnitSet->IsVerify?1:0))*Len*1000+pUnitSet->dwBaudRate/2)/pUnitSet->dwBaudRate*2;
	}

	return TimeOut;
}

static void SerDevQueueUnInit(BOOL IsLock)
{
	int i,j,CancelStatus,OldStatus;

	if(IsLock)
	{
		PTHREAD_MUTEX_SAFE_LOCK(sSerInputQueueMutex,OldStatus);
		PTHREAD_MUTEX_SAFE_LOCK(sSerOutputQueueMutex,CancelStatus);
	}
	
	if(sSerDevInputQueue.bIsCondInited)
	{
		if(IsLock)
		{
			pthread_cond_broadcast(&sSerDevInputQueue.ReadyCond);
			pthread_cond_broadcast(&sSerDevInputQueue.WriteCond);
		}
		pthread_cond_destroy(&sSerDevInputQueue.ReadyCond);
		pthread_cond_destroy(&sSerDevInputQueue.WriteCond);
		sSerDevInputQueue.bIsCondInited=FALSE;
	}
	for(j=0;j<SER_DEV_INPUT_QUEUE_SIZE;j++)
	{
		if(NULL!=sSerDevInputQueue.Queue[j].pBuf)
		{
			free(sSerDevInputQueue.Queue[j].pBuf);
			sSerDevInputQueue.Queue[j].pBuf=NULL;
		}
	}
	sSerDevInputQueue.bIsReady=FALSE;
	
	if(sSerDevOutputQueue.IsCondInited)
	{
		if(IsLock)pthread_cond_broadcast(&sSerDevOutputQueue.ReadyCond);
		pthread_cond_destroy(&sSerDevOutputQueue.ReadyCond);
		sSerDevOutputQueue.IsCondInited=FALSE;
	}
	for(j=0;j<SER_DEV_OUTPUT_QUEUE_SIZE;j++)
	{
		if(NULL!=sSerDevOutputQueue.Queue[j].pBuf)
		{
			free(sSerDevOutputQueue.Queue[j].pBuf);
			sSerDevOutputQueue.Queue[j].pBuf=NULL;
		}
	}
	sSerDevOutputQueue.IsReady=FALSE;
	if(IsLock)
	{
		PTHREAD_MUTEX_SAFE_UNLOCK(sSerOutputQueueMutex,CancelStatus);
		PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,OldStatus);
	}
}

static BOOL SerDevQueueInit(void)
{
	int i,j,CancelStatus,OldStatus;

	PTHREAD_MUTEX_SAFE_LOCK(sSerInputQueueMutex,OldStatus);
	PTHREAD_MUTEX_SAFE_LOCK(sSerOutputQueueMutex,CancelStatus);
	memset(&sSerDevInputQueue,0,sizeof(sSerDevInputQueue));
	
	pthread_cond_init(&sSerDevInputQueue.ReadyCond,NULL);
	pthread_cond_init(&sSerDevInputQueue.WriteCond,NULL);
	sSerDevInputQueue.bIsCondInited=TRUE;
	for(j=0;j<SER_DEV_INPUT_QUEUE_SIZE;j++)
	{
		sSerDevInputQueue.Queue[j].pBuf=malloc(SER_DEV_INPUT_NALU_SIZE);
		if(NULL==sSerDevInputQueue.Queue[j].pBuf)
		{
			SerDevQueueUnInit(FALSE);
			PTHREAD_MUTEX_SAFE_UNLOCK(sSerOutputQueueMutex,CancelStatus);
			PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,OldStatus);
			return FALSE;
		}
	}
	sSerDevInputQueue.bIsReady=TRUE;

	memset(&sSerDevOutputQueue,0,sizeof(sSerDevOutputQueue));
	pthread_cond_init(&sSerDevOutputQueue.ReadyCond,NULL);
	sSerDevOutputQueue.IsCondInited=TRUE;
	for(j=0;j<SER_DEV_OUTPUT_QUEUE_SIZE;j++)
	{
		sSerDevOutputQueue.Queue[j].pBuf=malloc(SER_DEV_OUTPUT_NALU_SIZE);
		if(NULL==sSerDevOutputQueue.Queue[j].pBuf)
		{
			SerDevQueueUnInit(FALSE);
			PTHREAD_MUTEX_SAFE_UNLOCK(sSerOutputQueueMutex,CancelStatus);
			PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,OldStatus);
			return FALSE;
		}
	}
	sSerDevOutputQueue.IsReady=TRUE;
	PTHREAD_MUTEX_SAFE_UNLOCK(sSerOutputQueueMutex,CancelStatus);
	PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,OldStatus);
	return TRUE;
}

static BOOL SerDevWriteOutputQueue(void *pBuf,DWORD Len)
{
	int						i,OldStatus;
	PSER_DEV_OUTPUT_QUEUE	pQueue;

	PTHREAD_MUTEX_SAFE_LOCK(sSerOutputQueueMutex,OldStatus);
	pQueue=&sSerDevOutputQueue;
	if(pQueue->IsReady && Len>0 && Len<=SER_DEV_OUTPUT_NALU_SIZE)
	{
		memcpy(pQueue->Queue[pQueue->Head].pBuf,pBuf,Len);
		pQueue->Queue[pQueue->Head].dwLen=Len;
		pQueue->Queue[pQueue->Head].dwDevUnit=0;
		pQueue->Queue[pQueue->Head].dwUserIndex=0;
		pQueue->Head=(pQueue->Head+1)%SER_DEV_OUTPUT_QUEUE_SIZE;
		pthread_cond_broadcast(&pQueue->ReadyCond);
		for(i=0;i<SER_DEV_USER_TOTAL;i++)
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
	}
	else
	{
		PTHREAD_MUTEX_SAFE_UNLOCK(sSerOutputQueueMutex,OldStatus);
		return FALSE;
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sSerOutputQueueMutex,OldStatus);
	return TRUE;
}

static BOOL SerDevReadInputQueue(void *pBuf,DWORD *pSize,const DWORD *pTimeOutMS)
{
	int						Ret,OldStatus;
	PSER_DEV_INPUT_QUEUE	pQueue;
	struct timespec			DestTime;
	
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
	PTHREAD_MUTEX_SAFE_LOCK(sSerInputQueueMutex,OldStatus);
	pQueue=&sSerDevInputQueue;
	for(;;)
	{
		if(!pQueue->bIsReady)
		{
			PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,OldStatus);
			return FALSE;
		}
		if(pQueue->dwTail==pQueue->dwHead)
		{
			if(NULL==pTimeOutMS)
			{
				if(pthread_cond_wait(&pQueue->ReadyCond,&sSerInputQueueMutex)!=STD_SUCCESS)
				{
					PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,OldStatus);
					return FALSE;
				}
				continue;
			}
			else if(*pTimeOutMS==0)
			{
				*pSize=0;
				break;
			}
			else
			{
				if((Ret=pthread_cond_timedwait(&pQueue->ReadyCond,&sSerInputQueueMutex,&DestTime))!=STD_SUCCESS)
				{
					if(Ret==ETIMEDOUT)
					{
						*pSize=0;
						break;
					}
					PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,OldStatus);
					return FALSE;
				}
				continue;
			}
		}
		else
		{
			if(pQueue->Queue[pQueue->dwTail].dwLen>*pSize)
			{
				PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,OldStatus);
				return FALSE;
			}
			
			memcpy(pBuf,pQueue->Queue[pQueue->dwTail].pBuf,pQueue->Queue[pQueue->dwTail].dwLen);
			*pSize=pQueue->Queue[pQueue->dwTail].dwLen;
			pQueue->dwTail=(pQueue->dwTail+1)%SER_DEV_INPUT_QUEUE_SIZE;
			pQueue->dwCount--;
			pthread_cond_broadcast(&pQueue->WriteCond);
			break;
		}
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,OldStatus);
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

static BOOL ProtocolAnalyse1F(const BYTE *pBuf,DWORD *pLen,BOOL *pIsCutted)
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
				printf("---1--DataLen=%d----\r\n",DataLen);
				DataLen=DataLen<<8;
				printf("--2---DataLen=%d----\r\n",DataLen);
				DataLen+=pBuf[13];
				printf("---3--DataLen=%d----\r\n",DataLen);
				if(*pLen<(DWORD)(DataLen+16))	
				{
					*pIsCutted=TRUE;
					return TRUE;
				}
				else
				{
					crc16 = pBuf[14+DataLen]<<8 | pBuf[15+DataLen];
					U16Temp = CRC_16_S_CCIT_FALSE(pBuf,14+DataLen);
					printf("----crc16=0x%x---U16Temp=0x%x----\r\n",crc16,U16Temp);
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

static void LoraParseRecvData(BYTE *pBuf,DWORD Len)
{
	static BYTE		RecvBuf[1024];
	static DWORD	RecvLen=0;
	DWORD			i,j,TempLen,ComDataLen,OverPushLen;
	BOOL			IsCutted=FALSE;
	int				OldStatus;

	if((Len+RecvLen)<=sizeof(RecvBuf))
	{
		memcpy(&RecvBuf[RecvLen],pBuf,Len);
		RecvLen += Len;
	}
	else
	{
		sSerDevServer.DataLostCount += Len+RecvLen-sizeof(RecvBuf);
		memcpy(&RecvBuf[RecvLen],pBuf,sizeof(RecvBuf)-RecvLen);
		RecvLen=sizeof(RecvBuf);
	}
	for(i=0;i<RecvLen;)
	{
		TempLen=RecvLen-i;
		if(ProtocolAnalyse1F(&RecvBuf[i],&TempLen,&IsCutted))
		{
			if(IsCutted)
			{
				if(i>0)
				{
					memcpy(RecvBuf,&RecvBuf[i],RecvLen-i);
					RecvLen -= i;
				}
				return;
			}
			else
			{
				struct AES_ctx ctx;
				int num,j=0;
				DWORD datalen = 0;
				BYTE key[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};
				
				PTHREAD_MUTEX_SAFE_LOCK(sSerProMutex,OldStatus);
				
				datalen = RecvBuf[i+12]<<8 |RecvBuf[i+13];
				AES_init_ctx(&ctx, key);
				printf("-------datalen=%d-------\r\n",datalen);
				num = datalen/16;
				for(j=0;j<num;j++)
				{
					AES_ECB_decrypt(&ctx, &RecvBuf[i+j*16+14]);
				}
				//if(RecvBuf[i+1]==0xff)	//addr
				{
					printf("-----type=0x%x------\r\n",RecvBuf[i+11]);
					switch(RecvBuf[i+11])
					{
					case 0x0A:	//初始包
						{
							BYTE SendBuf[256]={0};
							BYTE CmdBuf[256]={0};
							WORD Count=0,MsgLen=16;
							BYTE DevMsgId=0x7A;
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
						#if SER_DEV_ROLA_MASTER_MODE
							sprintf(CmdBuf,"BBF+LORASEND=0000000000000000,");
							ZFY_RolaWriteData(CmdBuf,strlen(CmdBuf),FALSE,NULL);
						#endif
							ZFY_RolaWriteData(SendBuf,17+sizeof(MsgBody),FALSE,NULL);
						}
						break;
					case 0x0B:	//心跳包
						{
							BYTE SendBuf[256]={0};
							BYTE CmdBuf[256]={0};
							WORD Count=0,MsgLen=16;
							BYTE DevMsgId=0x7B;
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
						#if SER_DEV_ROLA_MASTER_MODE
							sprintf(CmdBuf,"BBF+LORASEND=0000000000000000,");
							ZFY_RolaWriteData(CmdBuf,strlen(CmdBuf),FALSE,NULL);
						#endif
							ZFY_RolaWriteData(SendBuf,17+sizeof(MsgBody),FALSE,NULL);
						}
						break;
					case 0x0C:	//事件上报包
						{
							BYTE SendBuf[256]={0};
							BYTE CmdBuf[256]={0};
							WORD Count=0,MsgLen=16;
							BYTE DevMsgId=0x7C;
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
						#if SER_DEV_ROLA_MASTER_MODE
							sprintf(CmdBuf,"BBF+LORASEND=0000000000000000,");
							ZFY_RolaWriteData(CmdBuf,strlen(CmdBuf),FALSE,NULL);
						#endif
							ZFY_RolaWriteData(SendBuf,17+sizeof(MsgBody),FALSE,NULL);
						}
						break;
					case 0x0D:	//校时包
						break;
					case 0x0E:	//配置包
						break;
					case 0x0F:	//参数查询包
						break;
					default:
						ZFY_LES_LogPrintf("LORA",LOG_EVENT_LEVEL_NOTICE,"***protocol(Cmd=0x%X)...\r\n",RecvBuf[i+3]);
						break;
					}
				}
				//else
				//	ZFY_LES_LogPrintf("LORA",LOG_EVENT_LEVEL_NOTICE,"***unspported protocol(Addr=0x%X)...\r\n",RecvBuf[i+1]);
				PTHREAD_MUTEX_SAFE_UNLOCK(sSerProMutex,OldStatus);
				
				i += TempLen;
			}
		}
		else
		{
			i++;
			sSerDevServer.DataLostCount++;
		}
	}
	RecvLen=0;
}

static void *SerDevUnitRecvThread(void *pArg)
{
	int							UnitIndex=(int)pArg;
	int							CurrLen,TotalLen,ReadyNum;
	PSER_UNIT_LOCAL_SET			pConfig=NULL;
	PSER_UNIT_STATUS			pStatus=NULL;
	struct timespec				LastTime,CurrTime;
	BYTE						LocalBuf[1024];
	struct  timeval				WaitTime;
	fd_set						WriteSet,ReadSet;

	printf("-------SerDevUnitRecvThread-------\r\n");
	ZFY_LES_LogPrintf("ROLA",LOG_EVENT_LEVEL_INFO,"******lora serial(PID=%d,PTID=%u) recv...\r\n"
		,getpid(),pthread_self());
	pConfig=&sSerDevServer.UnitSet;
	pStatus=&sSerDevServer.UnitStatus;
	
	{
		//BYTE buf[19]={0x1F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x01,0x0A,0x00,0x02,0x55,0xaa,0x12,0x2c,0xF1};
		//LoraParseRecvData(buf,19);
	}
	for(;;)
	{
		if(pStatus->IsReqRecvQuit)
		{
			pStatus->IsReqRecvQuit=FALSE;
			break;
		}
		pthread_testcancel();
		if(pStatus->IsInError)
		{
			MSLEEP(SER_DEV_POLL_TIMER_MIN_MS);
			continue;
		}

		FD_ZERO(&ReadSet);
		FD_SET(pStatus->DevDrvHandle,&ReadSet);
		WaitTime.tv_sec=0;
		WaitTime.tv_usec=SER_DEV_POLL_TIMER_MAX_MS*1000;
		ReadyNum=select(pStatus->DevDrvHandle+1,&ReadSet,NULL,NULL,&WaitTime);
		if(ReadyNum<0)
		{
			pStatus->IsInError=TRUE;
			continue;
		}
		if(ReadyNum==0 || !FD_ISSET(pStatus->DevDrvHandle,&ReadSet))
		{
			MSLEEP(1);
			continue;
		}

		CurrLen=read(pStatus->DevDrvHandle,LocalBuf,sizeof(LocalBuf));
		if(CurrLen<0)
		{
			pStatus->IsInError=TRUE;
			continue;
		}
		else if(CurrLen==0)
		{
			MSLEEP(1);
			continue;
		}
		else
		{
			TotalLen=CurrLen;
			pStatus->RecvCount++;
		}
		LoraParseRecvData(LocalBuf,TotalLen);
	}

	return NULL;
}

static void *SerDevUnitSendThread(void *pArg)
{
	int						UnitIndex=(int)pArg;
	int						CurrLen,ReadyNum;
	PSER_UNIT_LOCAL_SET		pConfig=NULL;
	PSER_UNIT_STATUS		pStatus=NULL;
	BYTE					*pCurrBuf;
	DWORD					ReadTimeout,TotalLen,WriteTimeout;
	struct  timeval			WaitTime;
	fd_set					WriteSet;
	BYTE					LocalBuf[1024];

	printf("-------SerDevUnitSendThread-------\r\n");
	ZFY_LES_LogPrintf("LORA",LOG_EVENT_LEVEL_INFO,"******rola serial send(PID=%d,PTID=%u)...\r\n"
		,getpid(),pthread_self());
	pConfig=&sSerDevServer.UnitSet;
	pStatus=&sSerDevServer.UnitStatus;
	ReadTimeout=SER_DEV_POLL_TIMER_MAX_MS;
	for(WriteTimeout=SER_DEV_UNIT_SEND_POLL_TIMEOUT_MIN_MS;;)
	{
		 if(pStatus->IsReqSendQuit)
		 {
			 pStatus->IsReqSendQuit=FALSE;
			 break;
		 }
		 pthread_testcancel();
		 if(pStatus->IsInError)
		 {
			MSLEEP(SER_DEV_POLL_TIMER_MIN_MS);
			 continue;
		 }
		 TotalLen=sizeof(LocalBuf);

		 if(!SerDevReadInputQueue(LocalBuf,&TotalLen,&ReadTimeout))
		 {
			 pStatus->IsInError=TRUE;
			 continue;
		 }
		 if(TotalLen==0)continue;

		 for(pCurrBuf=LocalBuf;TotalLen>0;)
		 {
			 if(pStatus->IsReqSendQuit)
				 break;
			 pthread_testcancel();
			 
			 FD_ZERO(&WriteSet);
			 FD_SET(pStatus->DevDrvHandle,&WriteSet);
			 WaitTime.tv_sec=WriteTimeout/1000;
			 WaitTime.tv_usec=(WriteTimeout%1000)*1000;
			 ReadyNum=select(pStatus->DevDrvHandle+1,NULL,&WriteSet,NULL,&WaitTime);
			 if(ReadyNum<=0 || !FD_ISSET(pStatus->DevDrvHandle,&WriteSet))
			 {
				 pStatus->IsInError=TRUE;
				 break;
			 }
			 printf("-----send--buf=0x%x--0x%x---len=%d----\r\n",pCurrBuf[0],pCurrBuf[1],TotalLen);
			 CurrLen=write(pStatus->DevDrvHandle,pCurrBuf,TotalLen);
			 if(CurrLen<=0)
			 {
				 pStatus->IsInError=TRUE;
				 break;
			 }
			 pStatus->SendCount++;
			 pCurrBuf += CurrLen;
			 TotalLen -= CurrLen;
			 WriteTimeout=SER_DEV_UNIT_SEND_POLL_TIMEOUT_MIN_MS+SerDevUnitSendTimeOut(pConfig,CurrLen,FALSE);
		 }
	 }
	 
	 return NULL;
}

static void SerDevManagerThreadExit(void *pArg)
{
	PSER_DEV_SERVER		pServer=(PSER_DEV_SERVER)pArg;
	int					i;

	pServer->IsReqQuit=FALSE;
	if(pServer->UnitStatus.WorkThreadID!=INVALID_PTHREAD_ID)
		pServer->UnitStatus.IsReqWorkQuit=TRUE;
	if(pServer->UnitStatus.SendThreadID!=INVALID_PTHREAD_ID)
		pServer->UnitStatus.IsReqSendQuit=TRUE;
	if(pServer->UnitStatus.RecvThreadID!=INVALID_PTHREAD_ID)
		pServer->UnitStatus.IsReqRecvQuit=TRUE;
	usleep(PTHREAD_DEFAULT_QUIT_TIMEOUT_US);
	if(pServer->UnitStatus.WorkThreadID!=INVALID_PTHREAD_ID && pServer->UnitStatus.IsReqWorkQuit)
		pthread_cancel(pServer->UnitStatus.WorkThreadID);
	if(pServer->UnitStatus.SendThreadID!=INVALID_PTHREAD_ID && pServer->UnitStatus.IsReqSendQuit)
		pthread_cancel(pServer->UnitStatus.SendThreadID);
	if(pServer->UnitStatus.RecvThreadID!=INVALID_PTHREAD_ID && pServer->UnitStatus.IsReqRecvQuit)
		pthread_cancel(pServer->UnitStatus.RecvThreadID);
	if(pServer->UnitStatus.WorkThreadID!=INVALID_PTHREAD_ID)
	{
		pServer->UnitStatus.WorkThreadID=INVALID_PTHREAD_ID;
		pServer->UnitStatus.IsReqWorkQuit=FALSE;
	}
	if(pServer->UnitStatus.SendThreadID!=INVALID_PTHREAD_ID)
	{
		pServer->UnitStatus.SendThreadID=INVALID_PTHREAD_ID;
		pServer->UnitStatus.IsReqSendQuit=FALSE;
	}
	if(pServer->UnitStatus.RecvThreadID!=INVALID_PTHREAD_ID)
	{
		pServer->UnitStatus.RecvThreadID=INVALID_PTHREAD_ID;
		pServer->UnitStatus.IsReqRecvQuit=FALSE;
	}
	if(pServer->UnitStatus.DevDrvHandle!=STD_INVALID_HANDLE)
	{
		close(pServer->UnitStatus.DevDrvHandle);
		pServer->UnitStatus.DevDrvHandle=STD_INVALID_HANDLE;
	}
	pServer->IsReqQuit=FALSE;
	ZFY_LES_LogPrintf("LORA",LOG_EVENT_LEVEL_NOTICE,"lora serial managerex...\r\n");
}

static void *SerDevManagerThread(void *pArg)
{
	PSER_DEV_SERVER		pServer=(PSER_DEV_SERVER)pArg;
	int					i=0,CancelStatus,RetryCount,ReportTime,InfCheckTime=0;
	BOOL				IsRestore=FALSE;
	
	ZFY_LES_LogPrintf("LORA",LOG_EVENT_LEVEL_INFO,"******lora serial manager(PID=%d,PTID=%u)...\r\n",getpid(),pthread_self());
	
	printf("-------SerDevManagerThread-------\r\n");
	pServer->UnitStatus.DevDrvHandle=STD_INVALID_HANDLE;
	pServer->UnitStatus.IsInError=FALSE;
	pServer->UnitStatus.IsReqRecvQuit=FALSE;
	pServer->UnitStatus.IsReqSendQuit=FALSE;
	pServer->UnitStatus.IsReqWorkQuit=FALSE;
	pServer->UnitStatus.RecvThreadID=INVALID_PTHREAD_ID;
	pServer->UnitStatus.SendThreadID=INVALID_PTHREAD_ID;
	pServer->UnitStatus.WorkThreadID=INVALID_PTHREAD_ID;	
	pthread_cleanup_push(SerDevManagerThreadExit,pServer);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
	for(RetryCount=0;;)
	{
		if(pServer->IsReqQuit)
		{
			pServer->IsReqQuit=FALSE;
			break;
		}
		pthread_testcancel();
		PTHREAD_MUTEX_SAFE_LOCK(sSerDevHwMutex,CancelStatus);
		if(pServer->UnitStatus.DevDrvHandle==STD_INVALID_HANDLE)
		{
			memset(pServer->UnitStatus.UartDevName,0,sizeof(pServer->UnitStatus.UartDevName));
			strcpy(pServer->UnitStatus.UartDevName,SER_DEV_NAME);
			pServer->UnitStatus.DevDrvHandle=SerDevHwUnitOpen(pServer->UnitStatus.UartDevName,&pServer->UnitSet);
		}
		PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevHwMutex,CancelStatus);
		if(pServer->UnitStatus.DevDrvHandle==STD_INVALID_HANDLE)
		{
			ZFY_LES_LogPrintf("LORA",LOG_EVENT_LEVEL_ERR,"lora serial(Name=%s) open failed...\r\n",pServer->UnitStatus.UartDevName);
			sleep(1);
			break;
		}
		if(pServer->UnitStatus.SendThreadID==INVALID_PTHREAD_ID)
		{
			pthread_attr_t		ThreadAttr;
			struct sched_param	ThreadSchedParam;
			
			pthread_attr_init(&ThreadAttr);
			pthread_attr_setdetachstate(&ThreadAttr,PTHREAD_CREATE_JOINABLE);
			pthread_attr_setinheritsched(&ThreadAttr,PTHREAD_EXPLICIT_SCHED);
			pthread_attr_setschedpolicy(&ThreadAttr,SCHED_RR);
			ThreadSchedParam.sched_priority=PTHREAD_DEFAULT_SCHED_PRIORITY+SYS_APP_SCHED_PRIORITY+APP_REALTIME_SCHED_PRIORITY
				+SYNC_APP_SCHED_PRIORITY+FOREGROUND_APP_SCHED_PRIORITY+RS232_SCHED_PRIORITY+DATA_SCHED_PRIORITY;
			pthread_attr_setschedparam(&ThreadAttr,&ThreadSchedParam);
			if(pthread_create(&pServer->UnitStatus.SendThreadID,&ThreadAttr,SerDevUnitSendThread,(void *)i)!=STD_SUCCESS)
			{
				pthread_attr_destroy(&ThreadAttr);
				pServer->UnitStatus.SendThreadID=INVALID_PTHREAD_ID;
				ZFY_LES_LogPrintf("LORA",LOG_EVENT_LEVEL_NOTICE,"rola serial send thread failed(Name=%s)...\r\n",pServer->UnitStatus.UartDevName);
				sleep(1);
				break;
			}
			pthread_attr_destroy(&ThreadAttr);
		}
		
		if(pServer->UnitStatus.RecvThreadID==INVALID_PTHREAD_ID)
		{
			pthread_attr_t		ThreadAttr;
			struct sched_param	ThreadSchedParam;
			
			pthread_attr_init(&ThreadAttr);
			pthread_attr_setdetachstate(&ThreadAttr,PTHREAD_CREATE_JOINABLE);
			pthread_attr_setinheritsched(&ThreadAttr,PTHREAD_EXPLICIT_SCHED);
			pthread_attr_setschedpolicy(&ThreadAttr,SCHED_RR);
			ThreadSchedParam.sched_priority=PTHREAD_DEFAULT_SCHED_PRIORITY+SYS_APP_SCHED_PRIORITY+APP_REALTIME_SCHED_PRIORITY
				+SYNC_APP_SCHED_PRIORITY+FOREGROUND_APP_SCHED_PRIORITY+RS232_SCHED_PRIORITY+DATA_SCHED_PRIORITY;
			pthread_attr_setschedparam(&ThreadAttr,&ThreadSchedParam);
			if(pthread_create(&pServer->UnitStatus.RecvThreadID,&ThreadAttr,SerDevUnitRecvThread,(void *)i)!=STD_SUCCESS)
			{
				pthread_attr_destroy(&ThreadAttr);
				pServer->UnitStatus.RecvThreadID=INVALID_PTHREAD_ID;
				ZFY_LES_LogPrintf("LORA",LOG_EVENT_LEVEL_NOTICE,"rola serial recv thread failed(Name=%s)...\r\n",pServer->UnitStatus.UartDevName);
				sleep(1);
				break;
			}
			pthread_attr_destroy(&ThreadAttr);
		}
		

		for(RetryCount=0,ReportTime=0;;)
		{
			if(pServer->IsReqQuit)
				break;
			pthread_testcancel();
			sleep(1);

			IsRestore=FALSE;
			if(pServer->UnitStatus.IsInError)
			{
				int		TempCancelStatus;

				PTHREAD_CANCEL_LOCK(CancelStatus);
				if(pServer->UnitStatus.WorkThreadID!=INVALID_PTHREAD_ID)
					pServer->UnitStatus.IsReqWorkQuit=TRUE;
				if(pServer->UnitStatus.SendThreadID!=INVALID_PTHREAD_ID)
					pServer->UnitStatus.IsReqSendQuit=TRUE;
				if(pServer->UnitStatus.RecvThreadID!=INVALID_PTHREAD_ID)
					pServer->UnitStatus.IsReqRecvQuit=TRUE;
				usleep(PTHREAD_DEFAULT_QUIT_TIMEOUT_US);
				if(pServer->UnitStatus.WorkThreadID!=INVALID_PTHREAD_ID && pServer->UnitStatus.IsReqWorkQuit)
					pthread_cancel(pServer->UnitStatus.WorkThreadID);
				if(pServer->UnitStatus.SendThreadID!=INVALID_PTHREAD_ID && pServer->UnitStatus.IsReqSendQuit)
					pthread_cancel(pServer->UnitStatus.SendThreadID);
				if(pServer->UnitStatus.RecvThreadID!=INVALID_PTHREAD_ID && pServer->UnitStatus.IsReqRecvQuit)
					pthread_cancel(pServer->UnitStatus.RecvThreadID);
				if(pServer->UnitStatus.WorkThreadID!=INVALID_PTHREAD_ID)
				{
					pthread_join(pServer->UnitStatus.WorkThreadID,NULL);
					pServer->UnitStatus.WorkThreadID=INVALID_PTHREAD_ID;
					pServer->UnitStatus.IsReqWorkQuit=FALSE;
				}
				if(pServer->UnitStatus.SendThreadID!=INVALID_PTHREAD_ID)
				{
					pthread_join(pServer->UnitStatus.SendThreadID,NULL);
					pServer->UnitStatus.SendThreadID=INVALID_PTHREAD_ID;
					pServer->UnitStatus.IsReqSendQuit=FALSE;
				}
				if(pServer->UnitStatus.RecvThreadID!=INVALID_PTHREAD_ID)
				{
					pthread_join(pServer->UnitStatus.RecvThreadID,NULL);
					pServer->UnitStatus.RecvThreadID=INVALID_PTHREAD_ID;
					pServer->UnitStatus.IsReqRecvQuit=FALSE;
				}
				PTHREAD_MUTEX_SAFE_LOCK(sSerDevHwMutex,TempCancelStatus);
				if(pServer->UnitStatus.DevDrvHandle!=STD_INVALID_HANDLE)
				{
					close(pServer->UnitStatus.DevDrvHandle);
					pServer->UnitStatus.DevDrvHandle=STD_INVALID_HANDLE;
				}
				PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevHwMutex,TempCancelStatus);
				PTHREAD_CANCEL_UNLOCK(CancelStatus);
				pServer->UnitStatus.IsInError=FALSE;
				pServer->UnitStatus.ErrorCount++;
				IsRestore=TRUE;
				ZFY_LES_LogPrintf("LORA",LOG_EVENT_LEVEL_ERR,"rola serial restore(Name=%s)...\r\n",pServer->UnitStatus.UartDevName);
			}

			if(IsRestore)
			{
				IsRestore=FALSE;
				sleep(1);
				break;
			}
		}
	}

	PTHREAD_CANCEL_LOCK(CancelStatus);
	pthread_cleanup_pop(TRUE);
	PTHREAD_CANCEL_UNLOCK(CancelStatus);
	return NULL;
}

static void *AlarmProcThread(void *pArg)
{
	PSER_DEV_SERVER		pServer=(PSER_DEV_SERVER)pArg;
	int					i=0,CancelStatus,RetryCount,ReportTime,InfCheckTime=0;
	BOOL				IsRestore=FALSE;
	
	ZFY_LES_LogPrintf("LORA",LOG_EVENT_LEVEL_INFO,"******lora serial alarm(PID=%d,PTID=%u)...\r\n",getpid(),pthread_self());
	
	printf("-------AlarmProcThread-------\r\n");
	
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
	printf("-------AlarmProcThread---000----\r\n");
	pServer->IsAlarmTrig=TRUE;
	for(RetryCount=0;;)
	{
		if(pServer->IsReqAlarmQuit)
		{
			pServer->IsReqAlarmQuit=FALSE;
			break;
		}
		pthread_testcancel();
		
		if(pServer->IsAlarmTrig)
		{
			IPC_CONFIG	IpcConf;
			IPC_API_CONF IpcApiConf;
			struct in_addr 	Addr;
			char snap_path[64]={0};
			char rec_path[64]={0};
			time_t start_time;
			time_t stop_time;
	
			memset(&IpcConf,0,sizeof(IpcConf));
			memset(&IpcApiConf,0,sizeof(IpcApiConf));
			ZFY_ConfIpcConfig(FALSE,&IpcConf);
			Addr.s_addr=htonl(IpcConf.IpcIp);
			IpcApiConf.strIpcServerIP=(char *)inet_ntoa(Addr);
			IpcApiConf.IpcServerPort=IpcConf.IpcPort;
			IpcApiConf.strLoginUser=IpcConf.IpcUser;
			IpcApiConf.strLoginPwd=IpcConf.IpcPwd;
			IpcApiConf.strPicPath="/home/car/pic/";
			IpcApiConf.strRecPath="/home/car/rec/";
			ZFY_IpcInit(&IpcApiConf);
			ZFY_IpcGetTime(0);
			ZFY_IpcStartRecord(0);
			time(&start_time);
			ZFY_IpcSnapShot("alarm",0,IpcConf.CapPath[0]);
			printf("------IpcConf.CapPath[0]=%s-----\r\n",IpcConf.CapPath[0]);
			sleep(5);
			ZFY_IpcSnapShot("alarm",0,IpcConf.CapPath[1]);
			printf("------IpcConf.CapPath[1]=%s-----\r\n",IpcConf.CapPath[1]);
			sleep(5);
			ZFY_IpcSnapShot("alarm",0,IpcConf.CapPath[2]);
			printf("------IpcConf.CapPath[2]=%s-----\r\n",IpcConf.CapPath[2]);
			sleep(5);
			ZFY_IpcSnapShot("alarm",0,IpcConf.CapPath[3]);
			printf("------IpcConf.CapPath[3]=%s-----\r\n",IpcConf.CapPath[3]);
			sleep(5);
			ZFY_IpcSnapShot("alarm",0,IpcConf.CapPath[4]);
			printf("------IpcConf.CapPath[4]=%s-----\r\n",IpcConf.CapPath[4]);
			sleep(5);
			sleep(10);
			time(&stop_time);
			ZFY_IpcStopRecord(0);
			ZFY_IpcLoadRecord(start_time,stop_time,0,IpcConf.RecordPath[0]);
			printf("------IpcConf.RecordPath[0]=%s-----\r\n",IpcConf.RecordPath[0]);
			IpcConf.RecordStart[0]=start_time;
			IpcConf.RecordEnd[0]=stop_time;
			IpcConf.AlarmFlag=1;
			ZFY_ConfIpcConfig(TRUE,&IpcConf);
			pServer->IsAlarmTrig=FALSE;
		}
	}
	
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_RolaDevOpen
 *功能描述: ROLA模块打开
 *输入描述: 无
 *输出描述: 无
 *返回描述: TRUE/FALSE
 *作者日期: LJJ/2024/12/02
 *全局声明: sSerDevMutex,sSerDevServer
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_RolaDevOpen(void)
{
	int							OldStatus;
	DWORD						i,j;
	pthread_attr_t				ThreadAttr;
	struct sched_param			ThreadSchedParam;
	BOOL						IsFindBaudRate;

	printf("-------ZFY_RolaDevOpen-------\r\n");
	PTHREAD_MUTEX_SAFE_LOCK(sSerDevMutex,OldStatus);
	if(sSerDevServer.IsInitReady)
	{
		PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,OldStatus);
		ZFY_LES_LogPrintf("LORA",LOG_EVENT_LEVEL_NOTICE,"rola serial have ready...\r\n");
		return TRUE;
	}
	memset(&sSerDevServer,0,sizeof(sSerDevServer));
	sSerDevServer.UnitSet.dwBaudRate=SER_DEV_UART_BAUDRATE;
	sSerDevServer.UnitSet.dwCurrMRU=255;
	sSerDevServer.UnitSet.dwDataBits=8;
	sSerDevServer.UnitSet.dwRecvTimeoutMS=0;
	sSerDevServer.UnitSet.IsRs485Mode=FALSE;
	sSerDevServer.UnitSet.dwReplyTimeoutMS=0;
	sSerDevServer.UnitSet.dwStopBits=1;
	sSerDevServer.UnitSet.IsVerify=FALSE;
	sSerDevServer.UnitSet.IsConstVerify=FALSE;
	sSerDevServer.UnitSet.IsOddVerify=FALSE;
	sSerDevServer.UnitSet.IsHwFlowCtrl=FALSE;
	sSerDevServer.UnitSet.IsSoftFlowCtrl=FALSE;
	sSerDevServer.UnitSet.IsRawMode=TRUE;
	sSerDevServer.UnitSet.IsGetOwner=FALSE;
	sSerDevServer.IsReqQuit=FALSE;
	sSerDevServer.Rs485DevDrvHandle=STD_INVALID_HANDLE;
	sSerDevServer.UnitStatus.DevDrvHandle=STD_INVALID_HANDLE;
	sSerDevServer.UnitStatus.RecvThreadID=INVALID_PTHREAD_ID;
	sSerDevServer.UnitStatus.SendThreadID=INVALID_PTHREAD_ID;
	sSerDevServer.UnitStatus.WorkThreadID=INVALID_PTHREAD_ID;
	sSerDevServer.UnitStatus.IsInError=FALSE;
	sSerDevServer.UnitStatus.IsReqRecvQuit=FALSE;
	sSerDevServer.UnitStatus.IsReqSendQuit=FALSE;
	sSerDevServer.UnitStatus.IsReqWorkQuit=FALSE;
	sSerDevServer.CallerPID=getpid();
	sSerDevServer.CallerThread=pthread_self();
	if(!SerDevQueueInit())
	{
		PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,OldStatus);
		ZFY_LES_LogPrintf("LORA",LOG_EVENT_LEVEL_NOTICE,"rola serial queue init failed...\r\n");
		return FALSE;
	}
	printf("-------ZFY_RolaDevOpen--111-----\r\n");
	pthread_attr_init(&ThreadAttr);
	pthread_attr_setdetachstate(&ThreadAttr,PTHREAD_CREATE_JOINABLE);
	pthread_attr_setinheritsched(&ThreadAttr,PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&ThreadAttr,SCHED_RR);
	ThreadSchedParam.sched_priority=PTHREAD_DEFAULT_SCHED_PRIORITY+SYS_APP_SCHED_PRIORITY+APP_REALTIME_SCHED_PRIORITY
		+SYNC_APP_SCHED_PRIORITY+BACKGROUND_APP_SCHED_PRIORITY+DATA_SCHED_PRIORITY;
	pthread_attr_setschedparam(&ThreadAttr,&ThreadSchedParam);
	if(pthread_create(&sSerDevServer.ManagerThreadID,&ThreadAttr,SerDevManagerThread,&sSerDevServer)!=STD_SUCCESS)
	{
		SerDevQueueUnInit(TRUE);
		pthread_attr_destroy(&ThreadAttr);
		PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,OldStatus);
		ZFY_LES_LogPrintf("LORA",LOG_EVENT_LEVEL_NOTICE,"lora serial manager thread create failed...\r\n");
		return FALSE;
	}
	if(pthread_create(&sSerDevServer.AlarmThreadID,NULL,AlarmProcThread,&sSerDevServer)!=STD_SUCCESS)
	{
		SerDevQueueUnInit(TRUE);
		pthread_attr_destroy(&ThreadAttr);
		PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,OldStatus);
		ZFY_LES_LogPrintf("LORA",LOG_EVENT_LEVEL_NOTICE,"lora serial alarm thread create failed...\r\n");
		return FALSE;
	}
	printf("-------ZFY_RolaDevOpen---222----\r\n");
	pthread_attr_destroy(&ThreadAttr);
	sSerDevServer.IsInitReady=TRUE;
	PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,OldStatus);
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_RolaDevClose
 *功能描述: ROLA模块关闭
 *输入描述: 无
 *输出描述: 无
 *返回描述: 无
 *作者日期: LJJ/2024/12/02
 *全局声明: sSerDevMutex,sSerDevServer
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern void ZFY_RolaDevClose(void)
{
	int	OldStatus;

	PTHREAD_MUTEX_SAFE_LOCK(sSerDevMutex,OldStatus);
	if(sSerDevServer.IsInitReady)
	{
		if(!pthread_equal(pthread_self(),sSerDevServer.CallerThread))
		{
			ZFY_LES_LogPrintf("LORA",LOG_EVENT_LEVEL_NOTICE,"lora serial close(0x%X,0x%X)...\r\n",(unsigned int)pthread_self()
				,(unsigned int)sSerDevServer.CallerThread);
		}
		sSerDevServer.IsReqQuit=TRUE;
		usleep(2*PTHREAD_DEFAULT_QUIT_TIMEOUT_US);
		if(sSerDevServer.IsReqQuit)
			pthread_cancel(sSerDevServer.ManagerThreadID);
		pthread_join(sSerDevServer.ManagerThreadID,NULL);
		sSerDevServer.ManagerThreadID=INVALID_PTHREAD_ID;
		sSerDevServer.IsReqQuit=FALSE;	
		sSerDevServer.IsReqAlarmQuit=TRUE;
		usleep(2*PTHREAD_DEFAULT_QUIT_TIMEOUT_US);
		if(sSerDevServer.IsReqAlarmQuit)
			pthread_cancel(sSerDevServer.AlarmThreadID);
		pthread_join(sSerDevServer.AlarmThreadID,NULL);
		sSerDevServer.AlarmThreadID=INVALID_PTHREAD_ID;
		sSerDevServer.IsReqAlarmQuit=FALSE;			
		SerDevQueueUnInit(TRUE);
		sSerDevServer.IsInitReady=FALSE;
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,OldStatus);
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_RolaWriteData
 *功能描述: ROLA模块发送数据
 *输入描述: 数据缓冲区,数据长度,溢出标志,超时
 *输出描述: 无
 *返回描述: TRUE/FALSE
 *作者日期: LJJ/2024/12/02
 *全局声明: sSerDevMutex,sSerDevServer
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_RolaWriteData(const void *pBuf,DWORD dwSize,BOOL IsAutoOver,const DWORD *pTimeOutMS)
{
	int						OldStatus,CancelStatus;
	struct timespec			DestTime;
	PSER_DEV_INPUT_QUEUE	pQueue;

	if(NULL==pBuf || dwSize==0)
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
		PTHREAD_MUTEX_SAFE_LOCK(sSerDevMutex,OldStatus);
		if(!sSerDevServer.IsInitReady)
		{
			PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,OldStatus);
			return FALSE;
		}
		PTHREAD_MUTEX_SAFE_LOCK(sSerInputQueueMutex,CancelStatus);
		pQueue=&sSerDevInputQueue;
		if(!pQueue->bIsReady)
		{
			PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,CancelStatus);
			PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,OldStatus);
			return FALSE;
		}
		if(((pQueue->dwHead+1)%SER_DEV_INPUT_QUEUE_SIZE)==pQueue->dwTail)
		{
			if(IsAutoOver)
			{
				pQueue->dwHead=pQueue->dwTail;
				pQueue->dwCount=0;
				pQueue->dwOverCount++;
				memcpy(pQueue->Queue[pQueue->dwHead].pBuf,pBuf,dwSize);
				pQueue->Queue[pQueue->dwHead].dwLen=dwSize;
				pQueue->Queue[pQueue->dwHead].dwUserIndex=0;
				pQueue->dwHead=(pQueue->dwHead+1)%SER_DEV_INPUT_QUEUE_SIZE;
				pQueue->dwCount++;
				pthread_cond_broadcast(&pQueue->ReadyCond);
				break;
			}
			else if(NULL==pTimeOutMS)
			{
				PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,CancelStatus);
				if(pthread_cond_wait(&pQueue->WriteCond,&sSerInputQueueMutex)!=STD_SUCCESS)
				{
					PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,OldStatus);
					return FALSE;
				}
				PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,OldStatus);
				continue;
			}
			else if(*pTimeOutMS==0)
			{
				PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,CancelStatus);
				PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,OldStatus);
				return FALSE;
			}
			else
			{
				PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,CancelStatus);
				if(pthread_cond_timedwait(&pQueue->WriteCond,&sSerInputQueueMutex,&DestTime)!=STD_SUCCESS)
				{
					PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,OldStatus);
					return FALSE;
				}
				PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,OldStatus);
				continue;
			}
		}
		else
		{
			memcpy(pQueue->Queue[pQueue->dwHead].pBuf,pBuf,dwSize);
			pQueue->Queue[pQueue->dwHead].dwLen=dwSize;
			pQueue->Queue[pQueue->dwHead].dwUserIndex=0;
			pQueue->dwHead=(pQueue->dwHead+1)%SER_DEV_INPUT_QUEUE_SIZE;
			pQueue->dwCount++;
			pthread_cond_broadcast(&pQueue->ReadyCond);
			break;
		}
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sSerInputQueueMutex,CancelStatus);
	PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,OldStatus);
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_RolaReadData
 *功能描述: ROLA模块接收数据
 *输入描述: 数据缓冲区,数据长度,超时
 *输出描述: 无
 *返回描述: TRUE/FALSE
 *作者日期: LJJ/2024/12/02
 *全局声明: sSerDevMutex,sSerDevServer
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_RolaReadData(void *pBuf,DWORD *pBufSize,const DWORD *pTimeOutMS)
{
	int						Ret,OldStatus,CancelStatus;
	PSER_DEV_OUTPUT_QUEUE	pQueue=NULL;
	struct timespec			DestTime;

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
		PTHREAD_MUTEX_SAFE_LOCK(sSerDevMutex,OldStatus);
		if(!sSerDevServer.IsInitReady)
		{
			PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,OldStatus);
			return FALSE;
		}
		PTHREAD_MUTEX_SAFE_LOCK(sSerOutputQueueMutex,CancelStatus);
		pQueue=&sSerDevOutputQueue;
		if(!pQueue->IsReady || !pQueue->UserTab[0].IsEnable)
		{
			PTHREAD_MUTEX_SAFE_UNLOCK(sSerOutputQueueMutex,CancelStatus);
			PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,OldStatus);
			return FALSE;
		}
		if(pQueue->Head==pQueue->UserTab[0].Tail)
		{
			if(NULL==pTimeOutMS)
			{
				PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,CancelStatus);
				if(pthread_cond_wait(&pQueue->ReadyCond,&sSerOutputQueueMutex)!=STD_SUCCESS)
				{
					PTHREAD_MUTEX_SAFE_UNLOCK(sSerOutputQueueMutex,OldStatus);
					return FALSE;
				}
				PTHREAD_MUTEX_SAFE_UNLOCK(sSerOutputQueueMutex,OldStatus);
				continue;
			}
			else if(*pTimeOutMS==0)
			{
				*pBufSize=0;
				break;
			}
			else
			{
				PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,CancelStatus);
				if((Ret=pthread_cond_timedwait(&pQueue->ReadyCond,&sSerOutputQueueMutex,&DestTime))!=STD_SUCCESS)
				{
					PTHREAD_MUTEX_SAFE_UNLOCK(sSerOutputQueueMutex,OldStatus);
					if(Ret==ETIMEDOUT)
					{
						*pBufSize=0;
						return TRUE;
					}
					return FALSE;
				}
				PTHREAD_MUTEX_SAFE_UNLOCK(sSerOutputQueueMutex,OldStatus);
				continue;
			}
		}
		else
		{
			if(pQueue->Queue[pQueue->UserTab[0].Tail].dwLen>*pBufSize)
			{
				PTHREAD_MUTEX_SAFE_UNLOCK(sSerOutputQueueMutex,CancelStatus);
				PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,OldStatus);
				return FALSE;
			}
			memcpy(pBuf,pQueue->Queue[pQueue->UserTab[0].Tail].pBuf
				,pQueue->Queue[pQueue->UserTab[0].Tail].dwLen);
			*pBufSize=pQueue->Queue[pQueue->UserTab[0].Tail].dwLen;
			pQueue->UserTab[0].Tail=(pQueue->UserTab[0].Tail+1)%SER_DEV_OUTPUT_QUEUE_SIZE;
			pQueue->UserTab[0].Count--;
			break;
		}	
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sSerOutputQueueMutex,CancelStatus);
	PTHREAD_MUTEX_SAFE_UNLOCK(sSerDevMutex,OldStatus);
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_RolaPowerCtrl
 *功能描述: ROLA模块电源控制
 *输入描述: 是否上电
 *输出描述: 无
 *返回描述: TRUE/FALSE
 *作者日期: LJJ/2024/12/02
 *全局声明: 无
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_RolaPowerCtrl(BOOL PowerEnable)
{
	system("echo 88 >/sys/class/gpio/export");  
	system("echo out >/sys/class/gpio/gpio88/direction");	
	if(PowerEnable) 
		system("echo 1 >/sys/class/gpio/gpio88/value");
	else
		system("echo 0 >/sys/class/gpio/gpio88/value");
	system("echo uart 0x1111 > /proc/nvt_info/nvt_pinmux/pinmux_set");
	sleep(1);
#if SER_DEV_ROLA_MASTER_MODE
	system("echo \"BBF+LORACFG=22,0000000000000001\" >/dev/ttyS2");
#endif
	return TRUE;
}