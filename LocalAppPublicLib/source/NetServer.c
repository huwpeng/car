/*
 ***************************************************************************************************************************
 ********************************************************************************************************************************
 *			PROGRAM MODULE
 *
 *			$Workfile:		NetServer.c			$
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
 *	ʵ�ֱ�׼�ĺ͹������������
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

#include <net/if_arp.h>
#include <net/route.h>
#include "NetServer.h"
#include "LogEventServer.h"
#include "Protocol.h"


/*
 ***********************************************************************************************************************************************
 * ��������
 ************************************************************************************************************************************************
*/


#define STD_ETH_GROUP_ADDR_MASK					0xFFFFFF80
#define STD_ETH_GROUP_ADDR						0x01005E00
#define ETH_DEV_DEFAULT_NAME					"eth0"
#define PPP_DEV_DEFAULT_NAME					"ppp0"


#define ETH_DEV_DRV_IOCTL_SWITCH_NORMAL			(SIOCDEVPRIVATE+0)
#define ETH_DEV_DRV_IOCTL_SWITCH_MASTER			(SIOCDEVPRIVATE+1)
#define ETH_DEV_DRV_IOCTL_SWITCH_SLAVER			(SIOCDEVPRIVATE+2)



#define	SOCKET_ERROR							(-1)
#define	SOCKETINIT_SELECT_FAIL					(-1)
#define	SOCKETINIT_LISTEN_LEN					8
#define	SOCKETINIT_MCAST_TTL					64
#define	SOCKETINIT_KEEPALIVE_IDLE				1
#define	SOCKETINIT_KEEPALIVE_TIME				3
#define	SOCKETINIT_KEEPALIVE_LIMIT				3
#define	SOCKETINIT_TCP_MAX_SEG_SIZE				1460
#define	SOCKETINIT_TCP_SEND_SIZE				0x80000
#define	SOCKETINIT_TCP_RECV_SIZE				0x80000
#define	SOCKETINIT_UDP_SEND_SIZE				0x80000
#define	SOCKETINIT_UDP_RECV_SIZE				0x80000
#define	SOCKETINIT_DEFAULT_CONNECT_TIMEOUT		5000
#define SOCKETINIT_TCP_SEND_TIMEOUT_MS			10000
#define SOCKETINIT_TCP_RECV_TIMEOUT_MS			10000
#define SOCKETINIT_UDP_SEND_TIMEOUT_MS			5000
#define SOCKETINIT_UDP_RECV_TIMEOUT_MS			5000


#define		NTP_LI_NORMAL							0x00
#define		NTP_LI_DEC								0x01
#define		NTP_LI_INC								0x02
#define		NTP_LI_ALARM							0x03

#define		NTP_VN_VERSION1							0x01
#define		NTP_VN_VERSION2							0x02
#define		NTP_VN_VERSION3							0x03
#define		NTP_VN_VERSION4							0x04

#define		NTP_MODE_RESERVED						0x00
#define		NTP_MODE_SYMMETRIC_ACTIVE				0x01
#define		NTP_MODE_SYMMETRIC_PASSIVE				0x02
#define		NTP_MODE_CLIENT							0x03
#define		NTP_MODE_SERVER							0x04
#define		NTP_MODE_BROADCAST						0x05
#define		NTP_MODE_CONTROL						0x06
#define		NTP_MODE_PRIVATE						0x07

#define		NTP_STRATUM_NULL						0x00
#define		NTP_STRATUM_PRIMARY						0x01
#define		NTP_STRATUM_MAX_SECONDARY_REFERENCE		0x0F

#define		NTP_POLL_INTERVAL_MIN					4
#define		NTP_POLL_INTERVAL_MAX					14

#define		NTP_PRECISION_MIN						(-20)
#define		NTP_PRECISION_MAX						(-6)

#define		SNTP_START_DELAY						1000
#define		SNTP_POLL_MIN_DELAY						1000
#define		SNTP_IDLE_MAX_TIME_OUT					30000
#define		SNTP_CURR_DEFAULT_VERSION				NTP_VN_VERSION3


#define		ICMP_ECHO_REPLY							0
#define		ICMP_DEST_UNREACHABLE					3
#define		ICMP_SRC_QUENCH							4
#define		ICMP_REDIRECT							5
#define		ICMP_ECHO_REQUEST						8
#define		ICMP_ROUTE_NOTICE						9
#define		ICMP_ROUTE_REQUEST						10
#define		ICMP_TIME_EXCEEDED						11
#define		ICMP_PARA_PROBLEM						12
#define		ICMP_TIMESTAMP_REQ						13
#define		ICMP_TIMESTAMP_REPLY					14
#define		ICMP_ADDRMASK_REQ						17
#define		ICMP_ADDRMASK_REPLY						18

#define		ICMP_DEST_UNREACHABLE_NET				0
#define		ICMP_DEST_UNREACHABLE_HOST				1
#define		ICMP_DEST_UNREACHABLE_PROTOCOL			2
#define		ICMP_DEST_UNREACHABLE_PORT				3
#define		ICMP_DEST_UNREACHABLE_FRAGMENT			4
#define		ICMP_DEST_UNREACHABLE_SRC_ROUTER		5
#define		ICMP_DEST_UNREACHABLE_NET_UNKNOW		6
#define		ICMP_DEST_UNREACHABLE_HOST_UNKNOW		7
#define		ICMP_DEST_UNREACHABLE_NET_DISABLE		9
#define		ICMP_DEST_UNREACHABLE_HOST_DISABLE		10
#define		ICMP_DEST_UNREACHABLE_TOS_NET			11
#define		ICMP_DEST_UNREACHABLE_TOS_HOST			12
#define		ICMP_DEST_UNREACHABLE_DISABLE			13
#define		ICMP_DEST_UNREACHABLE_HOST_AUTHORITY	14
#define		ICMP_DEST_UNREACHABLE_PRI				15

#define		ICMP_REDIRECT_NET						0
#define		ICMP_REDIRECT_HOST						1
#define		ICMP_REDIRECT_NET_TOS					2
#define		ICMP_REDIRECT_HOST_TOS					3

#define		ICMP_TIME_EXCEEDED_TRANSFERS			0
#define		ICMP_TIME_EXCEEDED_ENCAPSULATION		1

#define		ICMP_PARA_PROBLEM_ERROR					0
#define		ICMP_PARA_PROBLEM_NEED_OPTION			1
						
#define		ICMP_PING_LEN_MAX						1024
#define		ICMP_PING_LEN_MIN						32
#define		ICMP_PING_INTERVAL_MIN					100
#define		ICMP_PING_TIMEOUT_MIN					1000
#define		ICMP_PING_TTL_MIN						64

#define		NET_API_LOG_NAME						"NET_API"
#define		NET_API_LOG(level,format,...)			ZFY_LES_LogPrintf(NET_API_LOG_NAME,level,format,##__VA_ARGS__)


/*
 ******************************************************************************************************************************************************
 ���Ͷ���
 ***************************************************************************************************************************************
*/

typedef struct
{
#if defined(LITTLE_ENDIAN)
	unsigned int	IHL:4;
	unsigned int	Version:4;

	unsigned int	TOS_Reserved:2;
	unsigned int	TOS_R:1;
	unsigned int	TOS_T:1;
	unsigned int	TOS_D:1;
	unsigned int	TOS_PRI:3;
#elif defined(BIG_ENDIAN)
	unsigned int	Version:4;
	unsigned int	IHL:4;

	unsigned int	TOS_PRI:3;
	unsigned int	TOS_D:1;
	unsigned int	TOS_T:1;
	unsigned int	TOS_R:1;
	unsigned int	TOS_Reserved:2;
#else
	#error Must define LITTLE_ENDIAN or BIG_ENDIAN
#endif

	unsigned int	TotalLen:16;
	unsigned int	IpID:16;

#if defined(LITTLE_ENDIAN)
	unsigned int	FragOffset:13;
	unsigned int	FragRump_MF:1;
	unsigned int	FragMask_DF:1;
	unsigned int	Frag_Reserved:1;
#elif defined(BIG_ENDIAN)
	unsigned int	Frag_Reserved:1;
	unsigned int	FragMask_DF:1;
	unsigned int	FragRump_MF:1;
	unsigned int	FragOffset:13;
#else
	#error Must define LITTLE_ENDIAN or BIG_ENDIAN
#endif

	unsigned int	TTL:8;
	unsigned int	Protocol:8;
	unsigned int	IpCheckSum:16;
	
	U32				SrcIp;
	U32				DstIp;
}IP_CONST_HEAD;

typedef struct
{
	U8		Type;
	U8		Code;
	U16		CheckSum;
	union	
	{
		U32		dwReserved;
		U32		GatewayAddr;
		struct
		{
			U16		IcmpID;
			U16		IcmpSeq;
		}StdData;
		struct
		{
			unsigned int	Offset:8;
			unsigned int	Reserved:24;
		}ParaInfo;
	}HeadData;
}ICMP_HEAD;

typedef union
{
	U32		NetAddrMask;
	struct
	{
		U32	SrcTime;
		U32	RecvTime;
		U32 SecTime;
	}TimeStamp;
	struct
	{
		IP_CONST_HEAD	SrcHead;
		U8				SrcData[8];
	}StdSrcInfo;
	U8	PingData[1];
}ICMP_DATA;

typedef struct
{
	IP_CONST_HEAD	IpHeader;
	ICMP_HEAD		IcmpHeader;
	ICMP_DATA		IcmpInfo;		
}ICMP_MESSAGE,*PICMP_MESSAGE;




typedef struct tNTP_HEADER
{
#if defined(LITTLE_ENDIAN)
	unsigned int Mode:3;
	unsigned int Version:3;
	unsigned int LeapIndicator:2;
#elif defined(BIG_ENDIAN)
	unsigned int LeapIndicator:2;
	unsigned int Version:3;
	unsigned int Mode:3;
#else
	#error Must define LITTLE_ENDIAN or BIG_ENDIAN
#endif
	unsigned int Stratum:8;
	signed int   Poll:8;
	signed int   Precision:8;
}NTP_HEADER,*PNTP_HEADER;

typedef struct tNTP_TIMESTAMP
{
	U32			Seconds;
	U32			Fraction;
}NTP_TIMESTAMP,*PNTP_TIMESTAMP;

typedef struct tNTP_BASE_MSG
{
	NTP_HEADER		Header;
	S32				RootDelay;
	U32				RootDispersion;
	U32				ReferenceID;
	NTP_TIMESTAMP	ReferenceTime;
	NTP_TIMESTAMP	OriginateTime;
	NTP_TIMESTAMP	ReceiveTime;
	NTP_TIMESTAMP	TransmitTime;
}NTP_BASE_MSG,*PNTP_BASE_MSG;

typedef struct tNTP_FULL_MSG
{
	NTP_BASE_MSG	BaseMsg;
	U32				KeyID;
	U8				MsgDigest[16];
}NTP_FULL_MSG,*PNTP_FULL_MSG;

typedef struct tSNTP_SERVER
{
	PLOCAL_TIME_SET_FUNC	pLocalTimeSet;
	Boolean					IsReady;
	BOOL					IsStarted;
	BOOL					IsReqQuit;
	BOOL					IsEnableSecondServer;
	pthread_t				SntpServerPID;
	pthread_t				CallerThread;
	__pid_t					CallerPID;
	U32						PollTimer;
	U32						RecvCount;
	U32						MsgCount;
	U32						RunCount;
	int						ServerFd;
	int						RoundtripDelay;
	struct sockaddr_in		ServerAddr;
	struct sockaddr_in		SecondServerAddr;
	struct sockaddr_in		LastServerAddr;
}SNTP_SERVER,*PSNTP_SERVER;




/*
 ***********************************************************************************************************************************************
 * ����ԭ��
 ************************************************************************************************************************************************
*/



/*
 ****************************************************************************************************************************************************************
 * ȫ�ֱ���
 *****************************************************************************************************************************************************************
*/


static SNTP_SERVER				SntpServer;
static pthread_mutex_t			sSntpServerMutex=PTHREAD_MUTEX_INITIALIZER;
static PZFY_NET_SAFE_REBOOT		sPtrNetServerSafeReboot=NULL;
static pthread_mutex_t			sNetServerMutex=PTHREAD_MUTEX_INITIALIZER;


/*
 ****************************************************************************************************************************************************
 * ��������
 *****************************************************************************************************************************************************
*/




/*
 ************************************************************************************************************************************************************************     
 *��������: NetServerSafeReboot
 *��������: ִ��ϵͳ��ȫ��λ����
 *��������: ��
 *�������: ��
 *��������: ��
 *��������: ZCQ/2013/01/22
 *ȫ������: sPtrNetServerSafeReboot��sNetServerMutex
 *����˵��: ��
 ************************************************************************************************************************************************************************       
 */
static void NetServerSafeReboot(void)
{
	int	CancelStatus;
	
	PTHREAD_MUTEX_SAFE_LOCK(sNetServerMutex,CancelStatus);
	if(sPtrNetServerSafeReboot==NULL)
		reboot(RB_AUTOBOOT);
	else
		(*sPtrNetServerSafeReboot)();
	PTHREAD_MUTEX_SAFE_UNLOCK(sNetServerMutex,CancelStatus);
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_NetServerSetSafeRebootCallback
 *��������: �����������ģ�鰲ȫ��λ�����ص�����
 *��������: ��ȫ��λ�����ص�����
 *�������: ��
 *��������: ��
 *��������: ZCQ/2013/01/22
 *ȫ������: sPtrNetServerSafeReboot��sNetServerMutex
 *����˵��: ��ȫ��λ�����ص�����Ĭ�Ͽգ��ص�����Ϊ��ִ��ϵͳ������λ����������
 ************************************************************************************************************************************************************************       
 */
extern void ZFY_NET_NetServerSetSafeRebootCallback(PZFY_NET_SAFE_REBOOT pCallbackFunc)
{
	int	CancelStatus;
	
	PTHREAD_MUTEX_SAFE_LOCK(sNetServerMutex,CancelStatus);
	sPtrNetServerSafeReboot=pCallbackFunc;
	PTHREAD_MUTEX_SAFE_UNLOCK(sNetServerMutex,CancelStatus);
}

/*
 ************************************************************************************************************************************************************************     
 *��������:	ZFY_NET_NetServerConfig,ZFY_NET_NetServerConfigEx
 *��������: ϵͳ������������������
 *��������: ϵͳ����ӿ�ģʽ���������������ò������ڶ�ETH�ӿ����ر�־
 *�������: ��
 *��������: �ɹ�(TRUE)/ʧ��(FALSE)
 *��������: ZCQ/2013/02/05
 *ȫ������: ��
 *����˵��: ������ӿ��豸������Ĭ��ȱʡ�豸��֧�ַǱ�������Ĭ������;ȡ������ģʽ����֧��(20150806)
 ************************************************************************************************************************************************************************       
 */
extern BOOL ZFY_NET_NetServerConfig(DWORD dwNetIfMode,const PSYS_NET_IF_CONFIG pConfig,BOOL IsBindGatewayToSecondEth)
{
	char				*pDevName;
	int					Fd,RetVal;
	struct rtentry		RouteEntry;
	struct ifreq		IfConfig;
	struct sockaddr_in	LocalAddr;

	if(NULL==pConfig)
		return FALSE;
	if(pConfig->bIsPPP_If)
	{
		if(pConfig->dwDefaultGateway==INADDR_ANY || pConfig->dwDefaultGateway==INADDR_NONE
			|| pConfig->dwDefaultGateway==INADDR_LOOPBACK || IN_CLASSD(pConfig->dwDefaultGateway)
			|| IN_BADCLASS(pConfig->dwDefaultGateway))
		{
			struct in_addr	addr;
			
			addr.s_addr=htonl(pConfig->dwDefaultGateway);
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"��Ч��Ĭ������IP��ַ(%s)...\r\n",inet_ntoa(addr));
			return FALSE;
		}
		if(pConfig->dwRemoteAddr==INADDR_ANY || pConfig->dwRemoteAddr==INADDR_NONE
			|| pConfig->dwRemoteAddr==INADDR_LOOPBACK || IN_CLASSD(pConfig->dwRemoteAddr)
			|| IN_BADCLASS(pConfig->dwRemoteAddr))
		{
			struct in_addr	addr;
			
			addr.s_addr=htonl(pConfig->dwRemoteAddr);
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"��Ч�ĵ�Ե�Զ��Ŀ��IP��ַ(%s)...\r\n",inet_ntoa(addr));
			return FALSE;
		}
	}
	else
	{
		if(pConfig->dwInetProtocolAddr==INADDR_ANY || pConfig->dwInetProtocolAddr==INADDR_NONE
			|| pConfig->dwInetProtocolAddr==INADDR_LOOPBACK || IN_CLASSD(pConfig->dwInetProtocolAddr)
			|| IN_BADCLASS(pConfig->dwInetProtocolAddr) 
			|| (pConfig->dwInetProtocolAddr&(~pConfig->dwSubNetMask))==(~pConfig->dwSubNetMask)
			|| (pConfig->dwInetProtocolAddr&(~pConfig->dwSubNetMask))==0
			|| (pConfig->dwInetProtocolAddr&IN_CLASSA_NET)==0)
		{
			struct in_addr	addr;

			addr.s_addr=htonl(pConfig->dwInetProtocolAddr);
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"��Ч��IP��ַ(%s)...\r\n",inet_ntoa(addr));
			return FALSE;
		}
		if(pConfig->dwDefaultGateway==INADDR_ANY || pConfig->dwDefaultGateway==INADDR_NONE
			|| pConfig->dwDefaultGateway==INADDR_LOOPBACK || IN_CLASSD(pConfig->dwDefaultGateway)
			|| IN_BADCLASS(pConfig->dwDefaultGateway)
			|| (pConfig->dwDefaultGateway&(~pConfig->dwSubNetMask))==(~pConfig->dwSubNetMask)
			|| (pConfig->dwDefaultGateway&(~pConfig->dwSubNetMask))==0
			|| (pConfig->dwDefaultGateway&IN_CLASSA_NET)==0)
		{
			struct in_addr	addr;

			addr.s_addr=htonl(pConfig->dwDefaultGateway);
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"��Ч��Ĭ������IP��ַ(%s)...\r\n",inet_ntoa(addr));
			return FALSE;
		}
		if((pConfig->dwInetProtocolAddr&pConfig->dwSubNetMask)
			!=(pConfig->dwDefaultGateway&pConfig->dwSubNetMask))
		{
			NET_API_LOG(LOG_EVENT_LEVEL_INFO,"���棺ϵͳ����IP��ַ��Ĭ������IP��ַ������һ��...\r\n");
		}
		if(pConfig->dwSubNetMask==INADDR_ANY || pConfig->dwSubNetMask==INADDR_NONE)
		{
			struct in_addr	addr;

			addr.s_addr=htonl(pConfig->dwSubNetMask);
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"��Ч����������(%s)...\r\n",inet_ntoa(addr));
			return FALSE;
		}
	}
	if(strlen(pConfig->strIfDevName)==0)
		pDevName=pConfig->bIsPPP_If?PPP_DEV_DEFAULT_NAME:ETH_DEV_DEFAULT_NAME;
	else
		pDevName=pConfig->strIfDevName;

	Fd=socket(AF_INET,SOCK_DGRAM,0);
	if(Fd==STD_INVALID_HANDLE)
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"��������ӿھ��ʧ��(%s)...\r\n",strerror(errno));
		return FALSE;
	}
	if(dwNetIfMode!=SYS_ADV_NET_IF_TYPE_STD && !pConfig->bIsPPP_If && !pConfig->bIsForceSet && strcmp(pDevName,ETH_DEV_DEFAULT_NAME)==0)
	{
		NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"��ǰϵͳ���������̬�Զ�����,�������ֶ�����...\r\n");
		close(Fd);
		return FALSE;
	}
	if(pConfig->bIsSetIfHwAddr && (dwNetIfMode==SYS_ADV_NET_IF_TYPE_STD || pConfig->bIsForceSet))
	{
		DWORD	EthGroupAddr;

		EthGroupAddr=(pConfig->IfHardwareAddr[0]<<24)|(pConfig->IfHardwareAddr[1]<<16)
			|(pConfig->IfHardwareAddr[2]<<8)|pConfig->IfHardwareAddr[3];
		if((EthGroupAddr&STD_ETH_GROUP_ADDR_MASK)==STD_ETH_GROUP_ADDR)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"��̫Ӳ�������ַ��Ч...\r\n");
			close(Fd);
			return FALSE;
		}

		memset(&IfConfig,0,sizeof(IfConfig));
		strncpy(IfConfig.ifr_name,pDevName,sizeof(IfConfig.ifr_name));
		RetVal=ioctl(Fd,SIOCGIFFLAGS,&IfConfig);
		if(RetVal!=STD_SUCCESS)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ����ӿ��豸(%s)״̬��ѯʧ��(%s)...\r\n",pDevName,strerror(errno));
			close(Fd);
			return FALSE;
		}
		strncpy(IfConfig.ifr_name,pDevName,sizeof(IfConfig.ifr_name));
		IfConfig.ifr_flags &= ~IFF_UP;
		RetVal=ioctl(Fd,SIOCSIFFLAGS,&IfConfig);
		if(RetVal!=STD_SUCCESS)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ����ӿ��豸(%s)�ر�ʧ��(%s)...\r\n",pDevName,strerror(errno));
			close(Fd);
			return FALSE;
		}

		memset(&IfConfig,0,sizeof(IfConfig));
		strncpy(IfConfig.ifr_name,pDevName,sizeof(IfConfig.ifr_name));
		IfConfig.ifr_hwaddr.sa_family=ARPHRD_ETHER;
		memcpy(IfConfig.ifr_hwaddr.sa_data,pConfig->IfHardwareAddr,sizeof(pConfig->IfHardwareAddr));
		RetVal=ioctl(Fd,SIOCSIFHWADDR,&IfConfig);
		if(RetVal!=STD_SUCCESS)
		{
			printf("ϵͳ������̫Ӳ�������ַʧ��(%s)����ϵͳ�Զ���λ...\r\n",strerror(errno));
			NET_API_LOG(LOG_EVENT_LEVEL_ALERT,"ϵͳ������̫Ӳ�������ַʧ��(%s)����ϵͳ�Զ���λ...\r\n",strerror(errno));
			close(Fd);
			NetServerSafeReboot();
			return FALSE;
		}
		
		memset(&IfConfig,0,sizeof(IfConfig));
		strncpy(IfConfig.ifr_name,pDevName,sizeof(IfConfig.ifr_name));
		RetVal=ioctl(Fd,SIOCGIFFLAGS,&IfConfig);
		if(RetVal!=STD_SUCCESS)
		{
			printf("ϵͳ����ӿ��豸(%s)״̬��ѯʧ��(%s)����ϵͳ�Զ���λ...\r\n",pDevName,strerror(errno));
			NET_API_LOG(LOG_EVENT_LEVEL_ALERT,"ϵͳ����ӿ��豸(%s)״̬��ѯʧ��(%s)����ϵͳ�Զ���λ...\r\n",pDevName,strerror(errno));
			close(Fd);
			NetServerSafeReboot();
			return FALSE;
		}
		strncpy(IfConfig.ifr_name,pDevName,sizeof(IfConfig.ifr_name));
		IfConfig.ifr_flags |= (IFF_UP|IFF_RUNNING);
		RetVal=ioctl(Fd,SIOCSIFFLAGS,&IfConfig);
		if(RetVal!=STD_SUCCESS)
		{
			printf("ϵͳ����ӿ��豸(%s)����ʧ��(%s)����ϵͳ�Զ���λ...\r\n",pDevName,strerror(errno));
			NET_API_LOG(LOG_EVENT_LEVEL_ALERT,"ϵͳ����ӿ��豸(%s)����ʧ��(%s)����ϵͳ�Զ���λ...\r\n",pDevName,strerror(errno));
			NetServerSafeReboot();
			close(Fd);
			return FALSE;
		}
	}

	if(dwNetIfMode==SYS_ADV_NET_IF_TYPE_STD || pConfig->bIsForceSet)
	{
		if(!pConfig->bIsIgnoreProtocolAddr)
		{
			memset(&IfConfig,0,sizeof(IfConfig));
			memset(&LocalAddr,0,sizeof(LocalAddr));
			strncpy(IfConfig.ifr_name,pDevName,sizeof(IfConfig.ifr_name));
			LocalAddr.sin_family=AF_INET;
			LocalAddr.sin_addr.s_addr=htonl(pConfig->dwInetProtocolAddr);
			LocalAddr.sin_port=htons(0);
			memcpy(&IfConfig.ifr_addr,&LocalAddr,sizeof(IfConfig.ifr_addr));
			RetVal=ioctl(Fd,SIOCSIFADDR,&IfConfig);
			if(RetVal!=STD_SUCCESS)
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ��������IP��ַʧ��(%s)...\r\n",strerror(errno));
				close(Fd);
				return FALSE;
			}
			
			memset(&IfConfig,0,sizeof(IfConfig));
			memset(&LocalAddr,0,sizeof(LocalAddr));
			strncpy(IfConfig.ifr_name,pDevName,sizeof(IfConfig.ifr_name));
			LocalAddr.sin_family=AF_INET;
			LocalAddr.sin_addr.s_addr=htonl(pConfig->dwSubNetMask);
			LocalAddr.sin_port=htons(0);
			memcpy(&IfConfig.ifr_netmask,&LocalAddr,sizeof(IfConfig.ifr_netmask));
			RetVal=ioctl(Fd,SIOCSIFNETMASK,&IfConfig);
			if(RetVal!=STD_SUCCESS)
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ����������������ʧ��(%s)...\r\n",strerror(errno));
				close(Fd);
				return FALSE;
			}
		}
	}

	if(pConfig->bIsForceSet || dwNetIfMode==SYS_ADV_NET_IF_TYPE_STD || (pConfig->bIsPPP_If && pConfig->bIsSetPPP_Gateway))
	{
		if(!pConfig->bIsIgnoreGateway)
		{
			if(!IsBindGatewayToSecondEth || !strcmp(pDevName,ETH_SECOND_IF_DEFAULT_NAME))
			{
				memset(&LocalAddr,0,sizeof(LocalAddr));
				LocalAddr.sin_family=AF_INET;
				LocalAddr.sin_addr.s_addr=htonl(INADDR_ANY);
				LocalAddr.sin_port=htons(0);
				memset(&RouteEntry,0,sizeof(RouteEntry));
				memcpy(&RouteEntry.rt_dst,&LocalAddr,sizeof(RouteEntry.rt_dst));
				memcpy(&RouteEntry.rt_genmask,&LocalAddr,sizeof(RouteEntry.rt_genmask));
				memcpy(&RouteEntry.rt_gateway,&LocalAddr,sizeof(RouteEntry.rt_gateway));
				RouteEntry.rt_dev=NULL;
				RouteEntry.rt_flags=RTF_UP;
				ioctl(Fd,SIOCDELRT,&RouteEntry);

				if((pConfig->dwInetProtocolAddr&pConfig->dwSubNetMask)!=(pConfig->dwDefaultGateway&pConfig->dwSubNetMask))
				{
					memset(&RouteEntry,0,sizeof(RouteEntry));
					LocalAddr.sin_addr.s_addr=htonl(pConfig->dwDefaultGateway);
					memcpy(&RouteEntry.rt_dst,&LocalAddr,sizeof(RouteEntry.rt_dst));
					LocalAddr.sin_addr.s_addr=htonl(INADDR_ANY);
					memcpy(&RouteEntry.rt_genmask,&LocalAddr,sizeof(RouteEntry.rt_genmask));
					memcpy(&RouteEntry.rt_gateway,&LocalAddr,sizeof(RouteEntry.rt_gateway));
					RouteEntry.rt_dev=pDevName;
					RouteEntry.rt_flags=RTF_UP|RTF_HOST;
					RetVal=ioctl(Fd,SIOCADDRT,&RouteEntry);
				}

				memset(&RouteEntry,0,sizeof(RouteEntry));
				LocalAddr.sin_addr.s_addr=htonl(INADDR_ANY);
				memcpy(&RouteEntry.rt_dst,&LocalAddr,sizeof(RouteEntry.rt_dst));
				memcpy(&RouteEntry.rt_genmask,&LocalAddr,sizeof(RouteEntry.rt_genmask));
				LocalAddr.sin_addr.s_addr=htonl(pConfig->dwDefaultGateway);
				memcpy(&RouteEntry.rt_gateway,&LocalAddr,sizeof(RouteEntry.rt_gateway));
				RouteEntry.rt_dev=pDevName;
				RouteEntry.rt_flags=RTF_UP|RTF_GATEWAY;
				RetVal=ioctl(Fd,SIOCADDRT,&RouteEntry);
				if(RetVal!=STD_SUCCESS)
				{
					NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ��������Ĭ������ʧ��(%s)...\r\n",strerror(errno));
					close(Fd);
					return FALSE;
				}
			}
		}
	}

	close(Fd);
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *��������:	ZFY_NET_NetServerStatus
 *��������: ϵͳ������������������
 *��������: ����ӿ��豸���ͽӿ�Ӳ����ַ��ѯ���
 *�������: �������������ò���
 *��������: �ɹ�(TRUE)/ʧ��(FALSE)
 *��������: ZCQ/2007/02/13
 *ȫ������: ��
 *����˵��: ������ӿ��豸������Ĭ��ȱʡ�豸
 ************************************************************************************************************************************************************************       
 */
extern BOOL ZFY_NET_NetServerStatus(PSYS_NET_IF_CONFIG pConfig)
{
	char				*pDevName;
	FILE				*pFile;
	int					Fd,RetVal;
	struct ifreq		IfConfig;
	struct sockaddr_in	LocalAddr;

	if(NULL==pConfig)
		return FALSE;
	if(strlen(pConfig->strIfDevName)==0)
		pDevName=pConfig->bIsPPP_If?PPP_DEV_DEFAULT_NAME:ETH_DEV_DEFAULT_NAME;
	else
		pDevName=pConfig->strIfDevName;

	Fd=socket(AF_INET,SOCK_DGRAM,0);
	if(Fd==STD_INVALID_HANDLE)
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"��������ӿھ��ʧ��(%s)...\r\n",strerror(errno));
		return FALSE;
	}

	if(pConfig->bIsSetIfHwAddr && !pConfig->bIsPPP_If)
	{
		memset(&IfConfig,0,sizeof(IfConfig));
		strncpy(IfConfig.ifr_name,pDevName,sizeof(IfConfig.ifr_name));
		IfConfig.ifr_hwaddr.sa_family=ARPHRD_ETHER;
		RetVal=ioctl(Fd,SIOCGIFHWADDR,&IfConfig);
		if(RetVal!=STD_SUCCESS)
		{
			printf("ϵͳ��ȡ��̫Ӳ�������ַʧ��(%s)...\r\n",strerror(errno));
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ��ȡ��̫Ӳ�������ַʧ��(%s)...\r\n",strerror(errno));
			close(Fd);
			return FALSE;
		}
		memcpy(pConfig->IfHardwareAddr,IfConfig.ifr_hwaddr.sa_data,sizeof(pConfig->IfHardwareAddr));
	}
	
	memset(&IfConfig,0,sizeof(IfConfig));
	memset(&LocalAddr,0,sizeof(LocalAddr));
	strncpy(IfConfig.ifr_name,pDevName,sizeof(IfConfig.ifr_name));
	RetVal=ioctl(Fd,SIOCGIFADDR,&IfConfig);
	if(RetVal!=STD_SUCCESS)
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ��ȡ����IP��ַʧ��(%s)...\r\n",strerror(errno));
		close(Fd);
		return FALSE;
	}
	memcpy(&LocalAddr,&IfConfig.ifr_addr,sizeof(IfConfig.ifr_addr));
	pConfig->dwInetProtocolAddr=ntohl(LocalAddr.sin_addr.s_addr);

	memset(&IfConfig,0,sizeof(IfConfig));
	memset(&LocalAddr,0,sizeof(LocalAddr));
	strncpy(IfConfig.ifr_name,pDevName,sizeof(IfConfig.ifr_name));
	RetVal=ioctl(Fd,SIOCGIFNETMASK,&IfConfig);
	if(RetVal!=STD_SUCCESS)
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ��ȡ������������ʧ��(%s)...\r\n",strerror(errno));
		close(Fd);
		return FALSE;
	}
	memcpy(&LocalAddr,&IfConfig.ifr_netmask,sizeof(IfConfig.ifr_netmask));
	pConfig->dwSubNetMask=ntohl(LocalAddr.sin_addr.s_addr);

	if(pConfig->bIsPPP_If)
	{
		memset(&IfConfig,0,sizeof(IfConfig));
		memset(&LocalAddr,0,sizeof(LocalAddr));
		strncpy(IfConfig.ifr_name,pDevName,sizeof(IfConfig.ifr_name));
		RetVal=ioctl(Fd,SIOCGIFDSTADDR,&IfConfig);
		if(RetVal!=STD_SUCCESS)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ��ȡ������������ʧ��(%s)...\r\n",strerror(errno));
			close(Fd);
			return FALSE;
		}
		memcpy(&LocalAddr,&IfConfig.ifr_dstaddr,sizeof(IfConfig.ifr_dstaddr));
		pConfig->dwRemoteAddr=ntohl(LocalAddr.sin_addr.s_addr);
	}

	close(Fd);
	if(pConfig->bIsIgnoreGateway)
	{
		pConfig->dwDefaultGateway=pConfig->dwInetProtocolAddr;
		return TRUE;
	}
	else
	{
		pFile=fopen("/proc/net/route","r");
		if(NULL==pFile)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ��ȡ����Ĭ�����ش�ϵͳ·�ɱ�ʧ��(%s)...\r\n",strerror(errno));
			return FALSE;
		}
		if(fscanf(pFile,"%*[^\n]\n")<0)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ��ȡ����Ĭ������ģʽƥ��ʧ��(%s)...\r\n",strerror(errno));
			fclose(pFile);
			return FALSE;
		}
		else
		{
			char				devname[64];
			unsigned long int	d,g,m;
			int					flgs,ref,use,metric,mtu,win,ir,r;

			for(;;)
			{
				r=fscanf(pFile,"%63s%lx%lx%X%d%d%d%lx%d%d%d\n",devname,&d,&g,&flgs,&ref
					,&use,&metric,&m,&mtu,&win,&ir);
				if(r != 11)
				{
					if((r<0) && feof(pFile))
					{
						NET_API_LOG(LOG_EVENT_LEVEL_INFO,"ϵͳ����Ĭ�����ز�����...\r\n");
						pConfig->dwDefaultGateway=pConfig->dwInetProtocolAddr;
						fclose(pFile);
						return TRUE;
					}
					NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ��ȡ����Ĭ�������������ص�ַʧ��(%s)...\r\n",strerror(errno));
					fclose(pFile);
					return FALSE;
				}
				if(!(flgs&RTF_UP))continue;
				if(strcmp(pDevName,devname)==0 && d==0)
				{
					pConfig->dwDefaultGateway=ntohl(g);
					if(pConfig->bIsPPP_If && pConfig->dwDefaultGateway==0)
						pConfig->dwDefaultGateway=pConfig->dwRemoteAddr;
					fclose(pFile);
					return TRUE;
				}
			}
			pConfig->dwDefaultGateway=pConfig->dwInetProtocolAddr;
			fclose(pFile);
			return TRUE;
		}
	}
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_NetServerGetRunStatus
 *��������: ��ȡ����ӿ��豸��ǰ����״̬
 *��������: ϵͳ����ӿ�ģʽ���ӿ��豸��
 *�������: �ӿ��豸��ǰ����״̬
 *��������: �ɹ�(TRUE)/ʧ��(FALSE)
 *��������: ZCQ/2013/02/04
 *ȫ������: ��
 *����˵��: ������ӿ��豸������Ĭ��ȱʡ�豸,ȱʡ�豸���ݵ�ǰϵͳ����ӿ�ģʽȷ��
 ************************************************************************************************************************************************************************       
 */
extern BOOL ZFY_NET_NetServerGetRunStatus(DWORD dwNetIfMode,const char *pIfName,PSYS_NET_IF_RUN_STATUS pStatus)
{
	struct user_net_device_stats
	{
		unsigned long long rx_packets;
		unsigned long long tx_packets;
		unsigned long long rx_bytes;
		unsigned long long tx_bytes;
		unsigned long rx_errors;
		unsigned long tx_errors;
		unsigned long rx_dropped;
		unsigned long tx_dropped;
		unsigned long rx_multicast;
		unsigned long rx_compressed;
		unsigned long tx_compressed;
		unsigned long collisions;

		unsigned long rx_length_errors;
		unsigned long rx_over_errors;
		unsigned long rx_crc_errors;
		unsigned long rx_frame_errors;
		unsigned long rx_fifo_errors;
		unsigned long rx_missed_errors;

		unsigned long tx_aborted_errors;
		unsigned long tx_carrier_errors;
		unsigned long tx_fifo_errors;
		unsigned long tx_heartbeat_errors;
		unsigned long tx_window_errors;
	};
	#if INT_MAX == LONG_MAX
		static const char * const ss_fmt[]=
		{
			"%n%Lu%u%u%u%u%n%n%n%Lu%u%u%u%u%u",
			"%Lu%Lu%u%u%u%u%n%n%Lu%Lu%u%u%u%u%u",
			"%Lu%Lu%u%u%u%u%u%u%Lu%Lu%u%u%u%u%u%u"
		};
	#else
		static const char * const ss_fmt[]=
		{
			"%n%Lu%lu%lu%lu%lu%n%n%n%Lu%lu%lu%lu%lu%lu",
			"%Lu%Lu%lu%lu%lu%lu%n%n%Lu%Lu%lu%lu%lu%lu%lu",
			"%Lu%Lu%lu%lu%lu%lu%lu%lu%Lu%Lu%lu%lu%lu%lu%lu%lu"
		};
	#endif
	FILE	*pFile=NULL;
	int		ProcNetDevVersion;
	char	LineBuf[1024];

	if(NULL==pStatus)
		return FALSE;
	if(NULL==pIfName)
	{
		if(dwNetIfMode==SYS_ADV_NET_IF_TYPE_STD || dwNetIfMode==SYS_ADV_NET_IF_TYPE_DHCP)
			pIfName=ETH_DEV_DEFAULT_NAME;
		else
			pIfName=PPP_DEV_DEFAULT_NAME;
	}
	pFile=fopen("/proc/net/dev","r");
	if(NULL==pFile)
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"��ϵͳ�ļ�/proc/net/devʧ��(%s)...\r\n",strerror(errno));
		return FALSE;
	}
	fgets(LineBuf,sizeof(LineBuf),pFile);
	fgets(LineBuf,sizeof(LineBuf),pFile);
	if(strstr(LineBuf,"compressed"))
		ProcNetDevVersion=2;
	else if(strstr(LineBuf,"bytes"))
		ProcNetDevVersion=1;
	else
		ProcNetDevVersion=0;
	
	while(fgets(LineBuf,sizeof(LineBuf),pFile))
	{
		char	*p,*s;
		int		namestart=0,nameend=0,aliasend;
		char	name[IFNAMSIZ];

		p=LineBuf;
		while(isspace(p[namestart]))
			namestart++;
		nameend=namestart;
		while(p[nameend] && p[nameend]!=':' && !isspace(p[nameend]))
			nameend++;
		if(p[nameend]==':')
		{
			aliasend=nameend+1;
			while(p[aliasend] && isdigit(p[aliasend]))
				aliasend++;
			if(p[aliasend]==':')
				nameend=aliasend;
			if((nameend-namestart)<IFNAMSIZ)
			{
				memcpy(name,&p[namestart],nameend-namestart);
				name[nameend-namestart]='\0';
				p=&p[nameend];
			}
			else
			{
				name[0]='\0';
			}
		}
		else
		{
			name[0]='\0';
		}
		s=p+1;
		if(!strcmp(pIfName,name))
		{
			struct user_net_device_stats	UserNetDevStatus;

			memset(&UserNetDevStatus,0,sizeof(UserNetDevStatus));
			sscanf(s,ss_fmt[ProcNetDevVersion]
				,&UserNetDevStatus.rx_bytes,&UserNetDevStatus.rx_packets
				,&UserNetDevStatus.rx_errors,&UserNetDevStatus.rx_dropped
				,&UserNetDevStatus.rx_fifo_errors,&UserNetDevStatus.rx_frame_errors
				,&UserNetDevStatus.rx_compressed,&UserNetDevStatus.rx_multicast
				,&UserNetDevStatus.tx_bytes,&UserNetDevStatus.tx_packets
				,&UserNetDevStatus.tx_errors,&UserNetDevStatus.tx_dropped
				,&UserNetDevStatus.tx_fifo_errors,&UserNetDevStatus.collisions
				,&UserNetDevStatus.tx_carrier_errors,&UserNetDevStatus.tx_compressed);

			if(ProcNetDevVersion <= 1)
			{
				if(ProcNetDevVersion == 0)
				{
					UserNetDevStatus.rx_bytes=0;
					UserNetDevStatus.tx_bytes=0;
				}
				UserNetDevStatus.rx_multicast=0;
				UserNetDevStatus.rx_compressed=0;
				UserNetDevStatus.tx_compressed=0;
			}
			pStatus->Collisions=UserNetDevStatus.collisions;
			pStatus->RxDropped=UserNetDevStatus.rx_dropped;
			pStatus->RxErrors=UserNetDevStatus.rx_errors;
			pStatus->RxMulticast=UserNetDevStatus.rx_multicast;
			pStatus->RxPackets=UserNetDevStatus.rx_packets;
			pStatus->RxBytes=UserNetDevStatus.rx_bytes;
			pStatus->RxCrcErrs=UserNetDevStatus.rx_crc_errors;
			pStatus->RxOverErrs=UserNetDevStatus.rx_over_errors;
			pStatus->TxBytes=UserNetDevStatus.tx_bytes;
			pStatus->TxDropped=UserNetDevStatus.tx_dropped;
			pStatus->TxErrors=UserNetDevStatus.tx_errors;
			pStatus->TxPackets=UserNetDevStatus.tx_packets;
			fclose(pFile);
			return TRUE;
		}
	}

	if(ferror(pFile))
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"����ϵͳ�ļ�/proc/net/devʧ��...\r\n");
	fclose(pFile);
	return FALSE;
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_CreateAsyncStreamSocketConnect
 *��������: �������ؿͻ���Զ�̷��������첽����
 *��������: Զ�̵�ַ
 *�������: NONE
 *��������: ����ӿڱ�ʶ
 *��������: ZCQ/2009/04/23
 *ȫ������: NONE
 *����˵��: ����ȴ����Ӿ�����ſ�����������
 ************************************************************************************************************************************************************************       
 */
extern int ZFY_NET_CreateAsyncStreamSocketConnect(struct sockaddr_in *pRemoteAddr)
{
	int				Fd,OptionFlagInt;
	struct linger	CurrLonger;

	if(NULL==pRemoteAddr)
	{
		NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"�ͻ����ӿڱ����ṩԶ�̷�������ַ...\r\n");
		return	SOCKET_INVALID_FD;
	}
	Fd=socket(AF_INET,SOCK_STREAM,0);
	if(Fd==SOCKET_INVALID_FD)
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�����������ӿ�ʧ��(%s)...\r\n",strerror(errno));
		return	SOCKET_INVALID_FD;
	}
	OptionFlagInt=1;
	if(setsockopt(Fd,SOL_SOCKET,SO_REUSEADDR,(const void *)&OptionFlagInt,sizeof(OptionFlagInt)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�������ӿڵ�ַ����ʧ��(%s)...\n",strerror(errno));
		close(Fd);
		return SOCKET_INVALID_FD;
	}
	CurrLonger.l_onoff=1;
	CurrLonger.l_linger=0;
	if(setsockopt(Fd,SOL_SOCKET,SO_LINGER,(const void *)&CurrLonger,sizeof(CurrLonger)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�������ӿ��ӳٹرս���ʧ��(%s)...\n",strerror(errno));
		close(Fd);
		return SOCKET_INVALID_FD;
	}
	OptionFlagInt=1;
	if(ioctl(Fd,FIONBIO,(char *)&OptionFlagInt))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�������ӿ����������첽����ʧ��(%s)...\r\n",strerror(errno));
		close(Fd);
		return SOCKET_INVALID_FD;
	}
	if(connect(Fd,(struct sockaddr *)pRemoteAddr,sizeof(struct sockaddr_in)) && errno!=EINPROGRESS)
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿڳ�������Զ�̷�����ʧ��(%s)...\r\n",strerror(errno));
		close(Fd);
		return SOCKET_INVALID_FD;
	}
	return Fd;
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_WaitForAsyncSocketConnectReady
 *��������: �ȴ����ؿͻ���Զ�̷��������첽���Ӿ���
 *��������: ����ӿڡ��ȴ���ʱʱ��
 *�������: �ȴ���ʱ��ǡ���ѡ���ص�ַ
 *��������: �ɹ�(True)/ʧ��(False)
 *��������: ZCQ/2009/04/23
 *ȫ������: NONE
 *����˵��: ���ɹ�����������ȴ���ʱ�����Ч,��pLocalAddrΪNULL����Ա��ص�ַ;
 ************************************************************************************************************************************************************************       
 */
extern Boolean	ZFY_NET_WaitForAsyncSocketConnectReady(int Fd,U32 TimeoutMS,Boolean *pIsReady,struct sockaddr_in *pLocalAddr)
{
	fd_set			ReadFdSet,WriteFdSet,ExceptFdSet;
	struct  timeval	WaitTime;
	struct timeval	Timeout;
	int				ReadyNum,OptionFlagInt,OptionVal;

	if(NULL==pIsReady)
		return False;
	WaitTime.tv_sec=TimeoutMS/1000;
	WaitTime.tv_usec=(TimeoutMS%1000)*1000;
	FD_ZERO(&ReadFdSet);
	FD_ZERO(&WriteFdSet);
	FD_ZERO(&ExceptFdSet);
	FD_SET(Fd,&ReadFdSet);
	FD_SET(Fd,&WriteFdSet);
	FD_SET(Fd,&ExceptFdSet);
	ReadyNum=select(Fd+1,&ReadFdSet,&WriteFdSet,&ExceptFdSet,&WaitTime);
	if(ReadyNum==SOCKETINIT_SELECT_FAIL)
	{
		*pIsReady=False;
		return False;
	}
	if(ReadyNum==0)
	{
		*pIsReady=False;
		return True;
	}
	if(FD_ISSET(Fd,&ExceptFdSet))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ��첽����Զ�̷������쳣ʧ��(%s)...\r\n",strerror(errno));
		*pIsReady=False;
		return False;
	}
	if(FD_ISSET(Fd,&ReadFdSet))
	{
		if(ioctl(Fd,FIONREAD,(char *)&OptionFlagInt))
		{
			*pIsReady=False;
			return False;
		}
		if(OptionFlagInt==0)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_WARNING,"�ͻ����ӿ��첽����Զ�̷�������ʱʧ��(%s)...\r\n",strerror(errno));
			*pIsReady=False;
			return False;
		}
	}
	*pIsReady=True;
	
	if(pLocalAddr)
	{
		OptionVal=sizeof(struct sockaddr_in);
		memset(pLocalAddr,0,sizeof(struct sockaddr_in));
		if(getsockname(Fd,(struct sockaddr *)pLocalAddr,&OptionVal))
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿڻ�ȡ���ص�ַʧ��(%s)...\r\n",strerror(errno));
			return False;
		}
	}
	OptionFlagInt=0;
	if(ioctl(Fd,FIONBIO,(char *)&OptionFlagInt))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�������ӿ���������ͬ������ʧ��(%s)...\r\n",strerror(errno));
		return False;
	}
	OptionVal=SOCKETINIT_TCP_SEND_SIZE;
	if(setsockopt(Fd,SOL_SOCKET,SO_SNDBUF,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ����÷��ͻ�����ʧ��(%s)...\r\n",strerror(errno));
		return False;
	}
	OptionVal=SOCKETINIT_TCP_RECV_SIZE;
	if(setsockopt(Fd,SOL_SOCKET,SO_RCVBUF,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ����ý��ջ�����ʧ��(%s)...\r\n",strerror(errno));
		return False;
	}
	Timeout.tv_sec=SOCKETINIT_TCP_RECV_TIMEOUT_MS/1000;
	Timeout.tv_usec=(SOCKETINIT_TCP_RECV_TIMEOUT_MS%1000)*1000;
	if(setsockopt(Fd,SOL_SOCKET,SO_RCVTIMEO,&Timeout,sizeof(Timeout)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ����ý��ճ�ʱʧ��(%s)...\r\n",strerror(errno));
		return False;
	}
	Timeout.tv_sec=SOCKETINIT_TCP_SEND_TIMEOUT_MS/1000;
	Timeout.tv_usec=(SOCKETINIT_TCP_SEND_TIMEOUT_MS%1000)*1000;
	if(setsockopt(Fd,SOL_SOCKET,SO_SNDTIMEO,&Timeout,sizeof(Timeout)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ����÷��ͳ�ʱʧ��(%s)...\r\n",strerror(errno));
		return False;
	}
	OptionFlagInt=1;
	if(setsockopt(Fd,SOL_SOCKET,SO_KEEPALIVE,(const void *)&OptionFlagInt,sizeof(OptionFlagInt)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ��������Ӵ��ʧ��(%s)...\r\n",strerror(errno));
		return False;
	}
	OptionVal=SOCKETINIT_KEEPALIVE_IDLE;
	if(setsockopt(Fd,IPPROTO_TCP,TCP_KEEPIDLE,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ��������ӿ���ʱ��ʧ��(%s)...\r\n",strerror(errno));
		return False;
	}
	OptionVal=SOCKETINIT_KEEPALIVE_TIME;
	if(setsockopt(Fd,IPPROTO_TCP,TCP_KEEPINTVL,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ��������Ӵ��̽������ʧ��(%s)...\r\n",strerror(errno));
		return False;
	}
	OptionVal=SOCKETINIT_KEEPALIVE_LIMIT;
	if(setsockopt(Fd,IPPROTO_TCP,TCP_KEEPCNT,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ��������Ӵ�ʱʧ��(%s)...\r\n",strerror(errno));
		return False;
	}
	OptionVal=0;
	if(setsockopt(Fd,IPPROTO_TCP,TCP_NODELAY,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ����ÿ�����Ӧģʽʧ��(%s)...\r\n",strerror(errno));
		return False;
	}
	OptionVal=SOCKETINIT_TCP_MAX_SEG_SIZE;
	if(setsockopt(Fd,IPPROTO_TCP,TCP_MAXSEG,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ��������Ƭ��ʧ��(%s)...\r\n",strerror(errno));
		return False;
	}
	OptionVal=1;
	if(setsockopt(Fd,IPPROTO_TCP,TCP_QUICKACK,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ����ÿ���Ӧ��ģʽʧ��(%s)...\r\n",strerror(errno));
		return False;
	}
	return True;
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_CreateSocket
 *��������: ��������ʼ������ӿ�
 *��������: �ӿ����͡���ʼ��ģʽ�����ص�ַ��Զ�̵�ַ���ಥ���ʶ(�豸�����ʶ)
 *�������: ���ݽӿ����ͺͳ�ʼ��ģʽ��ѡ������ص�ַ��Զ�̵�ַ
 *��������: ����ӿڱ�ʶ
 *��������: ZCQ/2008/01/28
 *ȫ������: ��
 *����˵��: �ͻ�ģʽ�����ṩԶ�̵�ַ����ѡ���ص�ַ���ڽ����Զ���ı��ص�ַ;������ģʽ�����ṩ���ص�ַ��Զ�̵�ַ����;
 *			UDP���������ڽ��ձ����ṩ���ص�ַ,��ֻ����������ʡ�Ա��ص�ַʱϵͳ�Զ��;
 *			UDP���Ϳ�ѡԶ�̵�ַ������Ϊ��������ṩ�������Ӱ󶨣��ಥ��㲥����Ϊ��ѡ�������������ӦĬ�ϵ�ַ;
 *			�ಥ���ʶ�����ಥ��Ч���û��ಥ��ȫ��Ч���Զ��ಥ��24λ��Ч(ϵͳͨѶЭ��Ĭ��ȡ��̫�豸48λ�����ַ��24λ),��8λ�̶�ΪĬ��ֵ;
 ************************************************************************************************************************************************************************       
 */
extern int	ZFY_NET_CreateSocket(int Type,int Mode,struct sockaddr_in *pLocalAddr,struct sockaddr_in *pRemoteAddr,U32 DevNetID)
{
	return ZFY_NET_CreateSocketEx(Type,Mode,pLocalAddr,pRemoteAddr,DevNetID,SOCKETINIT_DEFAULT_CONNECT_TIMEOUT);
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_CreateSocketEx
 *��������: ��������ʼ������ӿ�
 *��������: �ӿ����͡���ʼ��ģʽ�����ص�ַ��Զ�̵�ַ���ಥ���ʶ(�豸�����ʶ)�����ӳ�ʱʱ��(ms)
 *�������: ���ݽӿ����ͺͳ�ʼ��ģʽ��ѡ������ص�ַ��Զ�̵�ַ
 *��������: ����ӿڱ�ʶ
 *��������: ZCQ/2009/04/22
 *ȫ������: ��
 *����˵��: �ͻ�ģʽ�����ṩԶ�̵�ַ����ѡ���ص�ַ���ڽ����Զ���ı��ص�ַ;������ģʽ�����ṩ���ص�ַ��Զ�̵�ַ����;
 *			UDP���������ڽ��ձ����ṩ���ص�ַ,��ֻ����������ʡ�Ա��ص�ַʱϵͳ�Զ��;
 *			UDP���Ϳ�ѡԶ�̵�ַ������Ϊ��������ṩ�������Ӱ󶨣��ಥ��㲥����Ϊ��ѡ�������������ӦĬ�ϵ�ַ;
 *			�ಥ���ʶ�����ಥ��Ч���û��ಥ��ȫ��Ч���Զ��ಥ��24λ��Ч(ϵͳͨѶЭ��Ĭ��ȡ��̫�豸48λ�����ַ��24λ),��8λ�̶�ΪĬ��ֵ;
 ************************************************************************************************************************************************************************       
 */
extern int	ZFY_NET_CreateSocketEx(int Type,int Mode,struct sockaddr_in *pLocalAddr,struct sockaddr_in *pRemoteAddr,U32 DevNetID,U32 ConnectTimeoutMS)
{
	char			OptionFlagChar;
	int				Fd=SOCKET_INVALID_FD,OptionVal,OptionFlagInt;
	U32				MCastIPAddr;
	struct linger	CurrLonger;
	struct timeval	Timeout;
	struct  ip_mreq	McastAddr;

	if(Type==SOCKET_TYPE_TCP)
	{
		if(Mode==SOCKET_TCP_MODE_SERVER)
		{
			Fd=socket(AF_INET,SOCK_STREAM,0);
			if(Fd==SOCKET_INVALID_FD)
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�����������ӿ�ʧ��(%s)...\r\n",strerror(errno));
				return	SOCKET_INVALID_FD;
			}
			OptionFlagInt=1;
			if(setsockopt(Fd,SOL_SOCKET,SO_REUSEADDR,(const void *)&OptionFlagInt,sizeof(OptionFlagInt)))
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�������ӿڵ�ַ����ʧ��(%s)...\n",strerror(errno));
				close(Fd);
				return SOCKET_INVALID_FD;
			}
			CurrLonger.l_onoff=1;
			CurrLonger.l_linger=0;
			if(setsockopt(Fd,SOL_SOCKET,SO_LINGER,(const void *)&CurrLonger,sizeof(CurrLonger)))
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�������ӿ��ӳٹرս���ʧ��(%s)...\n",strerror(errno));
				close(Fd);
				return SOCKET_INVALID_FD;
			}
			if(pLocalAddr==NULL)
			{
				NET_API_LOG(LOG_EVENT_LEVEL_WARNING,"���������ӿڱ����ṩ���ص�ַ...\n");
				close(Fd);
				return SOCKET_INVALID_FD;
			}
			if(bind(Fd,(struct sockaddr *)pLocalAddr,sizeof(struct sockaddr_in)))
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"���������ӿڰ���ص�ַʧ��(%s)...\n",strerror(errno));
				close(Fd);
				return SOCKET_INVALID_FD;
			}
			if(listen(Fd,SOCKETINIT_LISTEN_LEN))
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"���������ӿ����ü���ģʽʧ��(%s)...\n",strerror(errno));
				close(Fd);
				return SOCKET_INVALID_FD;
			}
		}
		else if(Mode==SOCKET_TCP_MODE_CLIENT)
		{
			Boolean	IsReady=FALSE;

			Fd=CreateAsyncStreamSocketConnect(pRemoteAddr);
			if(Fd==SOCKET_INVALID_FD)
				return SOCKET_INVALID_FD;
			if(!WaitForAsyncSocketConnectReady(Fd,ConnectTimeoutMS,&IsReady,pLocalAddr))
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿڵȴ�����Զ�̷���������ʧ��(%s)...\r\n",strerror(errno));
				close(Fd);
				return SOCKET_INVALID_FD;
			}
			if(!IsReady)
			{
				NET_API_LOG(LOG_EVENT_LEVEL_WARNING,"�ͻ����ӿڵȴ�����Զ�̷�����������ʱ(%s)...\r\n",strerror(errno));
				close(Fd);
				return SOCKET_INVALID_FD;
			}
		}
		else
		{
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"�������ӿڳ�ʼ��ģʽ��Ч(%d)",Mode);
			return SOCKET_INVALID_FD;
		}
	}
	else if(Type==SOCKET_TYPE_UDP)
	{
		Fd=socket(AF_INET,SOCK_DGRAM,0);
		if(Fd==SOCKET_INVALID_FD)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�������籨�Ľӿ�ʧ��(%s)...\r\n",strerror(errno));
			return SOCKET_INVALID_FD;
		}
		OptionFlagInt=1;
		if(setsockopt(Fd,SOL_SOCKET,SO_REUSEADDR,(const void *)&OptionFlagInt,sizeof(OptionFlagInt)))
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"���籨�Ľӿڵ�ַ����ʧ��(%s)...\n",strerror(errno));
			close(Fd);
			return SOCKET_INVALID_FD;
		}
		OptionVal=SOCKETINIT_UDP_SEND_SIZE;
		if(setsockopt(Fd,SOL_SOCKET,SO_SNDBUF,(const void *)&OptionVal,sizeof(OptionVal)))
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"���籨�Ľӿ����÷��ͻ�����ʧ��(%s)...\r\n",strerror(errno));
			close(Fd);
			return SOCKET_INVALID_FD;
		}
		OptionVal=SOCKETINIT_UDP_RECV_SIZE;
		if(setsockopt(Fd,SOL_SOCKET,SO_RCVBUF,(const void *)&OptionVal,sizeof(OptionVal)))
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"���籨�Ľӿ����ý��ջ�����ʧ��(%s)...\r\n",strerror(errno));
			close(Fd);
			return SOCKET_INVALID_FD;
		}
		Timeout.tv_sec=SOCKETINIT_UDP_RECV_TIMEOUT_MS/1000;
		Timeout.tv_usec=(SOCKETINIT_UDP_RECV_TIMEOUT_MS%1000)*1000;
		if(setsockopt(Fd,SOL_SOCKET,SO_RCVTIMEO,&Timeout,sizeof(Timeout)))
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"���籨�Ľӿ����ý��ճ�ʱʧ��(%s)...\r\n",strerror(errno));
			close(Fd);
			return SOCKET_INVALID_FD;
		}
		Timeout.tv_sec=SOCKETINIT_UDP_SEND_TIMEOUT_MS/1000;
		Timeout.tv_usec=(SOCKETINIT_UDP_SEND_TIMEOUT_MS%1000)*1000;
		if(setsockopt(Fd,SOL_SOCKET,SO_SNDTIMEO,&Timeout,sizeof(Timeout)))
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"���籨�Ľӿ����÷��ͳ�ʱʧ��(%s)...\r\n",strerror(errno));
			close(Fd);
			return SOCKET_INVALID_FD;
		}
		OptionFlagChar=SOCKETINIT_MCAST_TTL;
		if(setsockopt(Fd,IPPROTO_IP,IP_MULTICAST_TTL,(const void *)&OptionFlagChar,sizeof(OptionFlagChar)))
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"���籨�Ľӿ����öಥ·������ʧ��(%s)...\n",strerror(errno));
			close(Fd);
			return SOCKET_INVALID_FD;
		}
		if(pLocalAddr)
		{
			struct sockaddr_in	TempLocalAddr=*pLocalAddr;
			
			if(Mode==SOCKET_UDP_MODE_MCAST || Mode==SOCKET_UDP_MODE_USER_MCAST)
			{
				if(Mode==SOCKET_UDP_MODE_USER_MCAST)
					TempLocalAddr.sin_addr.s_addr=htonl(DevNetID);
				else
					TempLocalAddr.sin_addr.s_addr=htonl(DEV_NET_ID_TO_AUTO_MCASTGROUP(DevNetID));
			}
			if(bind(Fd,(struct sockaddr *)&TempLocalAddr,sizeof(struct sockaddr_in)))
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"���籨�Ľӿڰ���ص�ַʧ��(%s)...\n",strerror(errno));
				close(Fd);
				return SOCKET_INVALID_FD;
			}
		}
		else
		{
			struct sockaddr_in	TempLocalAddr;

			memset(&TempLocalAddr,0,sizeof(TempLocalAddr));
			TempLocalAddr.sin_family=AF_INET;
			TempLocalAddr.sin_addr.s_addr=htonl(INADDR_ANY);
			TempLocalAddr.sin_port=htons(0);
			if(bind(Fd,(struct sockaddr *)&TempLocalAddr,sizeof(struct sockaddr_in)))
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"���籨�Ľӿڰ���ص�ַʧ��(%s)...\n",strerror(errno));
				close(Fd);
				return SOCKET_INVALID_FD;
			}
		}
		if(Mode==SOCKET_UDP_MODE_COMM)
		{
			if(pRemoteAddr)
			{
				if(connect(Fd,(struct sockaddr *)pRemoteAddr,sizeof(struct sockaddr_in)))
				{
					NET_API_LOG(LOG_EVENT_LEVEL_ERR,"���籨�Ľӿ�����Զ�̷�����ʧ��(%s)...\r\n",strerror(errno));
					close(Fd);
					return SOCKET_INVALID_FD;
				}
			}
		}
		else if(Mode==SOCKET_UDP_MODE_MCAST || Mode==SOCKET_UDP_MODE_USER_MCAST)
		{
			if(Mode==SOCKET_UDP_MODE_USER_MCAST)
			{
				MCastIPAddr=DevNetID;
				if(!IN_CLASSD(MCastIPAddr))
				{
					NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"���籨�Ľӿ���ͼ����Ƿ��ಥ��(0x%X)...\n",(unsigned int)MCastIPAddr);
					close(Fd);
					return SOCKET_INVALID_FD;
				}
			}
			else
			{
				MCastIPAddr=DEV_NET_ID_TO_AUTO_MCASTGROUP(DevNetID);
			}
			McastAddr.imr_interface.s_addr=htonl(INADDR_ANY);
			McastAddr.imr_multiaddr.s_addr=htonl(MCastIPAddr);
			if(pRemoteAddr)
			{
				memset(pRemoteAddr,0,sizeof(struct sockaddr_in));
				pRemoteAddr->sin_family=AF_INET;
				pRemoteAddr->sin_addr.s_addr=htonl(MCastIPAddr);
				pRemoteAddr->sin_port=htons(AVCM_SERVER_PORT);	
			}
			if(setsockopt(Fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,(const void *)&McastAddr,sizeof(McastAddr)))
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"���籨�Ľӿڼ���ಥ��ʧ��(%s)...\n",strerror(errno));
				close(Fd);
				return SOCKET_INVALID_FD;
			}
			OptionFlagChar=0;
			if(setsockopt(Fd,IPPROTO_IP,IP_MULTICAST_LOOP,(const void *)&OptionFlagChar,sizeof(OptionFlagChar)))
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"���籨�Ľӿڽ�ֹ�ಥ����ʧ��(%s)...\n",strerror(errno));
				close(Fd);
				return SOCKET_INVALID_FD;
			}
		}
		else if(Mode==SOCKET_UDP_MODE_BCAST)
		{
			if(pRemoteAddr)
			{
				memset(pRemoteAddr,0,sizeof(struct sockaddr_in));
				pRemoteAddr->sin_family=AF_INET;
				pRemoteAddr->sin_addr.s_addr=htonl(INADDR_BROADCAST);
				pRemoteAddr->sin_port=htons(AVCM_SERVER_BPORT);
			}
			OptionFlagInt=1;
			if(setsockopt(Fd,SOL_SOCKET,SO_BROADCAST,(const void *)&OptionFlagInt,sizeof(OptionFlagInt)))
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"���籨�Ľӿ����ù㲥ʧ��(%s)...\n",strerror(errno));
				close(Fd);
				return SOCKET_INVALID_FD;
			}	
		}
		else
		{
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"���籨�Ľӿڳ�ʼ��ģʽ��Ч(%d)",Mode);
			close(Fd);
			return SOCKET_INVALID_FD;
		}
	}
	else
	{
		NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"����ӿ�������Ч(%d)",Type);
		return SOCKET_INVALID_FD;
	}

	return	Fd;
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_KillSocketEx,ZFY_NET_KillSocket
 *��������: �ر�����ӿ�
 *��������: ����ӿڱ�ʶָ�롢�鲥��ַ
 *�������: �������ӿڱ�ʶ
 *��������: ��
 *��������: ZCQ/2007/01/24
 *ȫ������: ��
 *����˵��: ��
 ************************************************************************************************************************************************************************       
 */
extern void ZFY_NET_KillSocketEx(int *pFd,U32 GroupAddr)
{
	if(IN_CLASSD(GroupAddr))
	{
		struct  ip_mreq		McastAddr;

		McastAddr.imr_interface.s_addr=htonl(INADDR_ANY);
		McastAddr.imr_multiaddr.s_addr=htonl(GroupAddr);
		if(setsockopt(*pFd,IPPROTO_IP,IP_DROP_MEMBERSHIP,(const void *)&McastAddr,sizeof(McastAddr)))
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ����ӿ��˳��ಥ��ʧ��(%s)...\r\n",strerror(errno));
	}
	if(close(*pFd))NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ر�����ӿ�ʧ��(%s)...\r\n",strerror(errno));
	*pFd=SOCKET_INVALID_FD;
}
extern void ZFY_NET_KillSocket(int *pFd)
{
	ZFY_NET_KillSocketEx(pFd,0);
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_AcceptSocket
 *��������: �ӱ��ط������˿ڽ���Զ�̿ͻ�������
 *��������: ����������ӿ�ָ�롢����������ӿڱ��ص�ַ
 *�������: ����������ӿڡ���ѡ���ص�ַ����ѡԶ�̵�ַ
 *��������: ����ӿڱ�ʶ
 *��������: ZCQ/2013/01/25
 *ȫ������: ��
 *����˵��: ��pLocalAddrΪNULL����Ա��ص�ַ;��pRemoteAddrΪNULL�����Զ�̵�ַ;
 ************************************************************************************************************************************************************************       
 */
extern int	ZFY_NET_AcceptSocket(int *pServerFd,const struct sockaddr_in *pServerLocalAddr,struct sockaddr_in *pLocalAddr,struct sockaddr_in *pRemoteAddr)
{
	int				Fd,NewFd,AddrLen,OptionFlagInt,OptionVal;
	struct timeval	Timeout;
	
	if(pServerFd==NULL || pServerLocalAddr==NULL)
		return	SOCKET_INVALID_FD;
	Fd=*pServerFd;
	AddrLen=pRemoteAddr==NULL?0:sizeof(struct sockaddr_in);
	NewFd=accept(Fd,(struct sockaddr *)pRemoteAddr,&AddrLen);
	if(NewFd==SOCKET_INVALID_FD)
	{
		if(errno==EWOULDBLOCK)
		{
			KillSocket(&Fd);
			Fd=CreateSocket(SOCKET_TYPE_TCP,SOCKET_TCP_MODE_SERVER,
				(struct sockaddr_in *)pServerLocalAddr,NULL,INVALID_DEV_NET_ID);
			if(Fd==SOCKET_INVALID_FD)
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ALERT,"ϵͳ�����ؽ���������������ӿ�ʧ�ܶ��Զ���λ(%s)...\n",strerror(errno));
				sleep(1);
				NetServerSafeReboot();
				return SOCKET_INVALID_FD;
			}
			else
			{
				*pServerFd=Fd;
				NET_API_LOG(LOG_EVENT_LEVEL_WARNING,"ϵͳ�����ؽ���������������ӿڳɹ�...\n");
				return SOCKET_INVALID_FD;
			}
		}
		else
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"���������Խ��ܿͻ�����ʧ��(%s)...\r\n",strerror(errno));
			return SOCKET_INVALID_FD;
		}
	}
	if(pLocalAddr)
	{
		AddrLen=sizeof(struct sockaddr_in);
		if(getsockname(NewFd,(struct sockaddr *)pLocalAddr,&AddrLen))
		{		
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"��ȡ���ӱ��ص�ַʧ��(%s)...\r\n",strerror(errno));
			close(NewFd);
			return SOCKET_INVALID_FD;
		}	
	}
	OptionVal=1;
	if(setsockopt(NewFd,SOL_SOCKET,SO_REUSEADDR,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ����õ�ַ����ʧ��(%s)...\r\n",strerror(errno));
		close(NewFd);
		return SOCKET_INVALID_FD;
	}
	OptionVal=SOCKETINIT_TCP_SEND_SIZE;
	if(setsockopt(NewFd,SOL_SOCKET,SO_SNDBUF,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ����÷��ͻ�����ʧ��(%s)...\r\n",strerror(errno));
		close(NewFd);
		return SOCKET_INVALID_FD;
	}
	OptionVal=SOCKETINIT_TCP_RECV_SIZE;
	if(setsockopt(NewFd,SOL_SOCKET,SO_RCVBUF,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ����ý��ջ�����ʧ��(%s)...\r\n",strerror(errno));
		close(NewFd);
		return SOCKET_INVALID_FD;
	}
	Timeout.tv_sec=SOCKETINIT_TCP_RECV_TIMEOUT_MS/1000;
	Timeout.tv_usec=(SOCKETINIT_TCP_RECV_TIMEOUT_MS%1000)*1000;
	if(setsockopt(NewFd,SOL_SOCKET,SO_RCVTIMEO,&Timeout,sizeof(Timeout)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ����ý��ճ�ʱʧ��(%s)...\r\n",strerror(errno));
		close(NewFd);
		return SOCKET_INVALID_FD;
	}
	Timeout.tv_sec=SOCKETINIT_TCP_SEND_TIMEOUT_MS/1000;
	Timeout.tv_usec=(SOCKETINIT_TCP_SEND_TIMEOUT_MS%1000)*1000;
	if(setsockopt(NewFd,SOL_SOCKET,SO_SNDTIMEO,&Timeout,sizeof(Timeout)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ����÷��ͳ�ʱʧ��(%s)...\r\n",strerror(errno));
		close(NewFd);
		return SOCKET_INVALID_FD;
	}
	OptionFlagInt=1;
	if(setsockopt(NewFd,SOL_SOCKET,SO_KEEPALIVE,(const void *)&OptionFlagInt,sizeof(OptionFlagInt)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ��������Ӵ��ʧ��(%s)...\r\n",strerror(errno));
		close(NewFd);
		return SOCKET_INVALID_FD;
	}
	OptionVal=SOCKETINIT_KEEPALIVE_IDLE;
	if(setsockopt(NewFd,IPPROTO_TCP,TCP_KEEPIDLE,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ��������ӿ���ʱ��ʧ��(%s)...\r\n",strerror(errno));
		close(NewFd);
		return SOCKET_INVALID_FD;
	}
	OptionVal=SOCKETINIT_KEEPALIVE_TIME;
	if(setsockopt(NewFd,IPPROTO_TCP,TCP_KEEPINTVL,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ��������Ӵ��̽������ʧ��(%s)...\r\n",strerror(errno));
		close(NewFd);
		return SOCKET_INVALID_FD;
	}
	OptionVal=SOCKETINIT_KEEPALIVE_LIMIT;
	if(setsockopt(NewFd,IPPROTO_TCP,TCP_KEEPCNT,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ��������Ӵ�ʱʧ��(%s)...\r\n",strerror(errno));
		close(NewFd);
		return SOCKET_INVALID_FD;
	}
	OptionVal=0;
	if(setsockopt(NewFd,IPPROTO_TCP,TCP_NODELAY,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ����ÿ�����Ӧģʽʧ��(%s)...\r\n",strerror(errno));
		close(NewFd);
		return SOCKET_INVALID_FD;
	}
	OptionVal=SOCKETINIT_TCP_MAX_SEG_SIZE;
	if(setsockopt(NewFd,IPPROTO_TCP,TCP_MAXSEG,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ��������Ƭ��ʧ��(%s)...\r\n",strerror(errno));
		close(NewFd);
		return SOCKET_INVALID_FD;
	}
	OptionVal=1;
	if(setsockopt(NewFd,IPPROTO_TCP,TCP_QUICKACK,(const void *)&OptionVal,sizeof(OptionVal)))
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�ͻ����ӿ����ÿ���Ӧ��ģʽʧ��(%s)...\r\n",strerror(errno));
		close(NewFd);
		return SOCKET_INVALID_FD;
	}

	return NewFd;
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_WaitForSocketEx,ZFY_NET_WaitForSocket
 *��������: �ȴ�����ӿھ���
 *��������: �������ӿڱ�ʶ������ӿڱ�ʶ��д���쳣���ϡ��ȴ���ʱ(ms)
 *�������: ��������ӿ���
 *��������: �ɹ�(True)/ʧ��(False)
 *��������: ZCQ/2007/01/24
 *ȫ������: ��
 *����˵��: ����ʱΪ������������;��pTimeOutΪNULL��ȷ������;
 ************************************************************************************************************************************************************************       
 */
extern Boolean	ZFY_NET_WaitForSocketEx(int MaxFd,fd_set *pRset,fd_set *pWset,fd_set *pEset,const U32 *pTimeOut,int *pFdNum)
{
	struct  timeval		WaitTime;
	int					RetVal;

	if(pTimeOut)
	{
		WaitTime.tv_sec=*pTimeOut/1000;
		WaitTime.tv_usec=(*pTimeOut%1000)*1000;
		RetVal=select(MaxFd+1,pRset,pWset,pEset,&WaitTime);
	}
	else
		RetVal=select(MaxFd+1,pRset,pWset,pEset,NULL);
	if(RetVal==SOCKETINIT_SELECT_FAIL)
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ�ȴ�����ӿھ���ʧ��(%s)...",strerror(errno));
		*pFdNum=0;
		return False;
	}
	*pFdNum=RetVal;
	return True;	
}
extern Boolean	ZFY_NET_WaitForSocket(int MaxFd,fd_set *pRset,fd_set *pWset,const U32 *pTimeOut,int *pFdNum)
{
	return ZFY_NET_WaitForSocketEx(MaxFd,pRset,pWset,NULL,pTimeOut,pFdNum);
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_SendStream
 *��������: ����������
 *��������: �������ӿڱ�ʶ�����ͻ��������䳤��
 *�������: ��
 *��������: �ɹ�(True)/ʧ��(False)
 *��������: ZCQ/2007/01/24
 *ȫ������: ��
 *����˵��: ��
 ************************************************************************************************************************************************************************       
 */
extern Boolean	ZFY_NET_SendStream(int Fd,const void *Buf,U32 Len)
{
	U8			*pc;
	U32			Num;
	int			Step;

	pc=(U8 *)Buf;
	Num=Len;
	while(Num>0)
	{
		Step=send(Fd,pc,Num>SOCKETINIT_TCP_MAX_SEG_SIZE?SOCKETINIT_TCP_MAX_SEG_SIZE:Num,0);
		if(Step<0)
		{
			if(errno==EPIPE || errno==ECONNRESET)
			{
				NET_API_LOG(LOG_EVENT_LEVEL_WARNING,"Զ�������ر����ӵ����������ж�(%s)...\r\n",strerror(errno));
				return False; 
			}
			else
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�������ӿڷ�������ʧ��(%s)...\r\n",strerror(errno));
				return False;
			}
		}
		else
		{
			pc += Step;
			Num -= Step;
		}
	}
	return True;
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_RecvStream
 *��������: ����������
 *��������: �������ӿڱ�ʶ�����ջ��������䳤��
 *�������: ��
 *��������: �ɹ�(True)/ʧ��(False)
 *��������: ZCQ/2007/01/24
 *ȫ������: ��
 *����˵��: ��
 ************************************************************************************************************************************************************************       
 */
extern Boolean	ZFY_NET_RecvStream(int Fd,void *Buf,U32 Len)
{
	U8			*pc;
	U32			Num;
	int			Step;

	pc=(U8 *)Buf;
	Num=Len;
	while(Num>0)
	{
		Step=recv(Fd,pc,Num,MSG_WAITALL);
		if(Step==0)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_WARNING,"Զ�������ر����ӵ����������ж�...\r\n");
			return False; 
		}
		else if(Step<0)
		{
			if(errno==ECONNRESET)
			{
				NET_API_LOG(LOG_EVENT_LEVEL_WARNING,"Զ��������λ���ӵ����������ж�(%s)...\r\n",strerror(errno));
				return False; 
			}
			else if(errno==ECONNABORTED)
			{
				NET_API_LOG(LOG_EVENT_LEVEL_WARNING,"����������ֹ���ӵ����������ж�(%s)...\r\n",strerror(errno));
				return False; 	
			}
			else
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�������ӿڽ�������ʧ��(%s)...\r\n",strerror(errno));
				return False;
			}
		}
		else
		{
			Num -= Step;
			pc += Step;
		}
	}
	return True;
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_SendUdp
 *��������: �����û����ĵ�Ե㷢��
 *��������: �����û����Ľӿڱ�ʶ�����ͻ��������䳤��
 *�������: ��
 *��������: �ɹ�(True)/ʧ��(False)
 *��������: ZCQ/2007/01/24
 *ȫ������: ��
 *����˵��: �����û����Ľӿڱ�ʶ�����Ѿ���Զ��������������
 ************************************************************************************************************************************************************************       
 */
extern Boolean	ZFY_NET_SendUdp(int Fd,const void *Buf,U32 Len)
{
	U32	Step;

	if(Len>0)
	{
		Step=send(Fd,Buf,Len,0);
		if(Step != Len)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�����û����Ľӿڷ�������ʧ��(%s,%d,%d)...\r\n",strerror(errno),Step,(int)Len);
			return False;	
		}
	}
	return True;
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_SendUdpTo
 *��������: �����û����ĵ�Զ෢��
 *��������: �����û����Ľӿڱ�ʶ�����ͻ��������䳤�ȡ�Ŀ��Զ�̵�ַ
 *�������: ��
 *��������: �ɹ�(True)/ʧ��(False)
 *��������: ZCQ/2007/01/24
 *ȫ������: ��
 *����˵��: ����ָ��Ŀ��Զ�̵�ַ(�����ಥ��ַ��㲥��ַ),��δ�󶨱��ص�ַ����ϵͳ��ʱ��̬�Զ�����
 ************************************************************************************************************************************************************************       
 */
extern Boolean	ZFY_NET_SendUdpTo(int Fd,const void *Buf,U32 Len,const struct sockaddr_in *pRemoteAddr)
{
	U32	Step;

	if(Len>0)
	{
		Step=sendto(Fd,Buf,Len,0,(const struct sockaddr *)pRemoteAddr,sizeof(struct sockaddr_in));
		if(Step != Len)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�����û����Ľӿڷ�������ʧ��(%s,%d,%d)...\r\n",strerror(errno),Step,(int)Len);
			return False;
		}
	}
	return True;	
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_RecvUdp
 *��������: �����û����ĵ�Ե����
 *��������: �����û����Ľӿڱ�ʶ�����ջ��������䳤��
 *�������: ʵ�ʽ����ֽ���
 *��������: �ɹ�(True)/ʧ��(False)
 *��������: ZCQ/2009/04/23
 *ȫ������: ��
 *����˵��: �����û����Ľӿڱ�ʶ�����Ѿ���Զ��������������
 ************************************************************************************************************************************************************************       
 */
extern Boolean	ZFY_NET_RecvUdp(int Fd,void *Buf,U32 *Len)
{
	int	Step;

	if(*Len==0)return True;
	Step=recv(Fd,Buf,*Len,0);
	if(Step>=0)
	{
		*Len=Step;
		return True;
	}
	else
	{
		if(EWOULDBLOCK!=errno)
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�����û����Ľӿڽ�������ʧ��(%s)...\r\n",strerror(errno));
		return False;
	}
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_RecvUdpFrom
 *��������: �����û����Ķ�����
 *��������: �����û����Ľӿڱ�ʶ�����ջ��������䳤�ȡ���ѡԶ�̹��˵�ַ
 *�������: ʵ�ʽ����ֽ���������Զ�̹��˵�ַ������˽��(��ѡ)
 *��������: �ɹ�(True)/ʧ��(False)
 *��������: ZCQ/2009/04/23
 *ȫ������: ��
 *����˵��: ��pRemoteAddrΪNULL�����;��Զ�̵�ַΪ����ֱ�����ʵ��Զ�̵�ַ,��Զ�̶˿�Ϊ������Զ˿ڹ���;pRemoteAddr��pConflict����ͬʱΪNULL��ͬʱ��NULL
 ************************************************************************************************************************************************************************       
 */
extern Boolean	ZFY_NET_RecvUdpFrom(int Fd,void *Buf,U32 *Len,struct sockaddr_in *pRemoteAddr,Boolean *pConflict)
{
	int					Step,AddrLen;
	struct sockaddr_in	PeerAddr;

	if(*Len==0)return True;
	memset(&PeerAddr,0,sizeof(PeerAddr));
	AddrLen=sizeof(PeerAddr);
	Step=recvfrom(Fd,Buf,*Len,0,(struct sockaddr *)&PeerAddr,&AddrLen);
	if(Step>=0)
	{
		*Len=Step;
		if(pRemoteAddr&&pConflict)
		{
			*pConflict=False;
			if(pRemoteAddr->sin_addr.s_addr!=htonl(0))
			{
				if(PeerAddr.sin_addr.s_addr != pRemoteAddr->sin_addr.s_addr)
				{
					*pConflict=True;
					return True;
				}
				if(pRemoteAddr->sin_port)
				{
					if(PeerAddr.sin_port != pRemoteAddr->sin_port)
						*pConflict=True;
				}
			}
			else
			{
				*pRemoteAddr=PeerAddr;
			}
		}
		else
		{
			if(!(!pRemoteAddr && !pConflict))
				return False;
		}
		return	True;
	}
	else
	{
		if(EWOULDBLOCK!=errno)
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"�����û����Ľӿڽ�������ʧ��(%s)...\r\n",strerror(errno));
		return False;
	}		
}




/*
 ************************************************************************************************************************************************************************     
 *��������: IcmpCheckSum
 *��������: ����ICMP����У���
 *��������: ���Ļ������Լ����ĳ���
 *�������: NONE
 *��������: ICMP����У���
 *��������: ZCQ/2007/09/06
 *ȫ������: NONE
 *����˵��: ICMP����У���Ϊ��������16λ�����ۼӺ�ȡ��
 ************************************************************************************************************************************************************************       
*/
static U16	IcmpCheckSum(U16 *buf,U32 size)
{
	register	U32		sum=0;
	U32					nwords;

	for(nwords=size>>1;nwords>0;nwords--)
		sum += *buf++;
#if defined(LITTLE_ENDIAN)
	sum += (size&0x01)?*buf&0x00FF:0;
#elif defined(BIG_ENDIAN)
	sum += (size&0x01)?*buf&0xFF00:0;
#else
	#error Must define LITTLE_ENDIAN or BIG_ENDIAN
#endif
	while(sum>>16)sum=(sum&0xFFFF)+(sum>>16);
	return	~(U16)sum;
}

/*
 ************************************************************************************************************************************************************************     
 *��������: IcmpMessageAnalyze
 *��������: ����ICMP����
 *��������: ICMP����
 *�������: NONE
 *��������: NONE
 *��������: ZCQ/2007/09/06
 *ȫ������: NONE
 *����˵��: ����ICMP���ľ�Ϊ�����ֽ���
 ************************************************************************************************************************************************************************       
*/
static void  IcmpMessageAnalyze(PICMP_MESSAGE Recvip)
{
	struct sockaddr_in SrcAddr;

	SrcAddr.sin_addr.s_addr=Recvip->IpHeader.SrcIp;
	switch(Recvip->IcmpHeader.Type)
	{
	case ICMP_ECHO_REPLY :
		NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP������Ӧ����(%d,%d)...",inet_ntoa(SrcAddr.sin_addr)
			,Recvip->IcmpHeader.HeadData.StdData.IcmpID,Recvip->IcmpHeader.HeadData.StdData.IcmpSeq);
		break;
	case ICMP_DEST_UNREACHABLE :
		switch(Recvip->IcmpHeader.Code)
		{
		case ICMP_DEST_UNREACHABLE_NET :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPĿ�����粻�ɵ��ﱨ��...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_DEST_UNREACHABLE_HOST :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPĿ���������ɵ��ﱨ��...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_DEST_UNREACHABLE_PROTOCOL :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPĿ��Э�鲻�ɵ��ﱨ��...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_DEST_UNREACHABLE_PORT :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPĿ��˿ڲ��ɵ��ﱨ��...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_DEST_UNREACHABLE_FRAGMENT :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPԴվ�ܾ���Ƭ���²��ɵ��ﱨ��...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_DEST_UNREACHABLE_SRC_ROUTER :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPԴվѡ·ʧ�ܵ��²��ɵ��ﱨ��...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_DEST_UNREACHABLE_NET_UNKNOW :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPĿ������δ֪����...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_DEST_UNREACHABLE_HOST_UNKNOW :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPĿ������δ֪����...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_DEST_UNREACHABLE_NET_DISABLE :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPĿ�������ֹ����...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_DEST_UNREACHABLE_HOST_DISABLE :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPĿ��������ֹ����...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_DEST_UNREACHABLE_TOS_NET :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPĿ��������񲻿ɴﱨ��...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_DEST_UNREACHABLE_TOS_HOST :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPĿ���������񲻿ɴﱨ��...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_DEST_UNREACHABLE_DISABLE :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPͨѶ��ֹ����...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_DEST_UNREACHABLE_HOST_AUTHORITY :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP����ԽȨ����...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_DEST_UNREACHABLE_PRI :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP��������Ȩ��ֹ����...",inet_ntoa(SrcAddr.sin_addr));
			break;
		default :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPĿ�겻�ɴﱨ��(%d)..."
				,inet_ntoa(SrcAddr.sin_addr),Recvip->IcmpHeader.Code);
			break;
		}
		break;
	case ICMP_SRC_QUENCH :
		NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPԴվ���Ʊ���...",inet_ntoa(SrcAddr.sin_addr));
		break;
	case ICMP_REDIRECT :
		switch(Recvip->IcmpHeader.Code)
		{
		case ICMP_REDIRECT_NET :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP�����ض�����...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_REDIRECT_HOST :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP�����ض�����...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_REDIRECT_NET_TOS :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP��������ض�����...",inet_ntoa(SrcAddr.sin_addr));
			break;
		case ICMP_REDIRECT_HOST_TOS :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP���������ض�����...",inet_ntoa(SrcAddr.sin_addr));
			break;
		default :
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP�ض�����(%d)..."
				,inet_ntoa(SrcAddr.sin_addr),Recvip->IcmpHeader.Code);
			break;
		}
		break;
	case ICMP_ECHO_REQUEST :
		NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP����������...",inet_ntoa(SrcAddr.sin_addr));
		break;
	case ICMP_ROUTE_NOTICE :
		NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP·��ͨ�汨��...",inet_ntoa(SrcAddr.sin_addr));
		break;
	case ICMP_ROUTE_REQUEST	:
		NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP·��������...",inet_ntoa(SrcAddr.sin_addr));
		break;
	case ICMP_TIME_EXCEEDED :
		if(Recvip->IcmpHeader.Code==ICMP_TIME_EXCEEDED_TRANSFERS)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP���䳬ʱ����...",inet_ntoa(SrcAddr.sin_addr));
		}
		else
		{
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP���鳬ʱ����...",inet_ntoa(SrcAddr.sin_addr));
		}
		break;
	case ICMP_PARA_PROBLEM :
		if(Recvip->IcmpHeader.Code==ICMP_PARA_PROBLEM_ERROR)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP����������...",inet_ntoa(SrcAddr.sin_addr));
		}
		else
		{
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP����ȱ�ٱ���...",inet_ntoa(SrcAddr.sin_addr));
		}
		break;
	case ICMP_TIMESTAMP_REQ	:
		NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPʱ���ǩ������...",inet_ntoa(SrcAddr.sin_addr));
		break;
	case ICMP_TIMESTAMP_REPLY :
		NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMPʱ���ǩ��Ӧ����...",inet_ntoa(SrcAddr.sin_addr));
		break;
	case ICMP_ADDRMASK_REQ :
		NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP��������������...",inet_ntoa(SrcAddr.sin_addr));
		break;
	case ICMP_ADDRMASK_REPLY :
		NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP����������Ӧ����...",inet_ntoa(SrcAddr.sin_addr));
		break;
	default :
		NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ�յ���������(%s)��ICMP����(%d,%d)..."
			,inet_ntoa(SrcAddr.sin_addr),Recvip->IcmpHeader.Type,Recvip->IcmpHeader.Code);
		break;
	}
}

/*
 ************************************************************************************************************************************************************************     
 *��������: IcmpPingValidate
 *��������: ��֤ICMP���Ա���
 *��������: Ŀ��IP,������,��Ӧ����,�����ֽڳ���,�ϸ���֤��־,���ķ�����־
 *�������: NONE
 *��������: �ɹ�(1)/ʧ��(0)
 *��������: ZCQ/2007/09/06
 *ȫ������: NONE
 *����˵��: ����ICMP���ľ�Ϊ�����ֽ���
 ************************************************************************************************************************************************************************       
*/
static int	IcmpPingValidate(U32 DestIpAddr,PICMP_MESSAGE Sendip,PICMP_MESSAGE Recvip,U32 IcmpLen,Boolean bStrickValidate,Boolean bIsAnalyze)
{
	if(Recvip->IcmpHeader.Type==ICMP_ECHO_REPLY)
	{
		if(Recvip->IcmpHeader.HeadData.StdData.IcmpID==Sendip->IcmpHeader.HeadData.StdData.IcmpID)
		{
			if(IcmpCheckSum((U16 *)&Recvip->IcmpHeader,IcmpLen) == 0)
			{
				if(bStrickValidate)
				{
					if(DestIpAddr==ntohl(Recvip->IpHeader.SrcIp))
					{
						if(Recvip->IcmpHeader.HeadData.StdData.IcmpSeq==Sendip->IcmpHeader.HeadData.StdData.IcmpSeq)
						{
							return 1;
						}
					}
				}
				else
				{
					return 1;
				}
			}
		}
	}
	
	if(bIsAnalyze)
		IcmpMessageAnalyze(Recvip);
	return 0;
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_IcmpPing
 *��������: ��֤Ŀ����������ͨ�����
 *��������: Ŀ��IP,���ʱ��,̽����ʱ��(ms),̽�ⳬʱʱ��(ms),�û������ֽڳ���,�豸����ID,�ϸ���֤��־,���ķ�����־
 *�������: NONE
 *��������: �ɹ�(True)/ʧ��(False)
 *��������: ZCQ/2007/09/06
 *ȫ������: NONE
 *����˵��: ͬ����֤
 ************************************************************************************************************************************************************************       
*/
extern Boolean ZFY_NET_IcmpPing(U32 DestIP,int TTL,U32 Interval,U32 TimeOut,U32 Len,U32 DevNetID,Boolean bStrick,Boolean bIsAnalyze)
{
	U32					SendCount=0,RecvCount=0,MaxCount,IcmpLen;
	U16					SendSeq=0;
	int					i,Fd,FlagInt,Size;
	char				Flagchar;
	struct sockaddr_in	LocalAddr,RemoteAddr;
	PICMP_MESSAGE		pSendMessage,pRecvMessage;
	U8					SendBuf[ICMP_PING_LEN_MAX+sizeof(ICMP_HEAD)+sizeof(IP_CONST_HEAD)];
	U8					RecvBuf[ICMP_PING_LEN_MAX+sizeof(ICMP_HEAD)+sizeof(IP_CONST_HEAD)];

	if(TTL<ICMP_PING_TTL_MIN)TTL=ICMP_PING_TTL_MIN;
	if(Interval<ICMP_PING_INTERVAL_MIN)Interval=ICMP_PING_INTERVAL_MIN;
	if(TimeOut<ICMP_PING_TIMEOUT_MIN)TimeOut=ICMP_PING_TIMEOUT_MIN;
	if(Len>ICMP_PING_LEN_MAX)Len=ICMP_PING_LEN_MAX;
	if(Len<ICMP_PING_LEN_MIN)Len=ICMP_PING_LEN_MIN;
	MaxCount=(TimeOut+Interval-1)/Interval;
	Size=sizeof(LocalAddr);
	
	if((Fd=socket(AF_INET,SOCK_RAW,IPPROTO_ICMP)) == SOCKET_ERROR)
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"����ICMP���Ľӿ�ʧ��(%s)...",strerror(errno));
		return False;
	}
	memset(&LocalAddr,0,sizeof(LocalAddr));
	LocalAddr.sin_family=AF_INET;
	if(bind(Fd,(struct sockaddr *)&LocalAddr,sizeof(LocalAddr)) == SOCKET_ERROR)
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ICMP���Ľӿڰ󶨱��ص�ַʧ��(%s)...",strerror(errno));
		close(Fd);
		return False;
	}
	Flagchar=0;
	if(setsockopt(Fd,IPPROTO_IP,IP_MULTICAST_LOOP,&Flagchar,sizeof(Flagchar)) == SOCKET_ERROR)
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"��ֹICMP���Ľӿڶಥ�ͻ�ʧ��(%s)...",strerror(errno));
		close(Fd);
		return False;
	}
	if(IN_MULTICAST(DestIP))
	{
		Flagchar=(char)TTL;
		if(setsockopt(Fd,IPPROTO_IP,IP_MULTICAST_TTL,&Flagchar,sizeof(Flagchar)) == SOCKET_ERROR)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"����ICMP���Ľӿڶಥ���ʱ��ʧ��(%s)...",strerror(errno));
			close(Fd);
			return False;	
		}
	}
	else
	{
		FlagInt=TTL;
		if(setsockopt(Fd,IPPROTO_IP,IP_TTL,(char *)&FlagInt,sizeof(FlagInt)) == SOCKET_ERROR)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"����ICMP���Ľӿڵ������ʱ��ʧ��(%s)...",strerror(errno));
			close(Fd);
			return False;
		}
	}
	FlagInt=1;
	if(ioctl(Fd,FIONBIO,(char *)&FlagInt) == SOCKET_ERROR)
	{
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"����ICMP���Ľӿ��첽����ģʽʧ��(%s)...\r\n",strerror(errno));
		close(Fd);
		return False;
	}

	IcmpLen=Len+sizeof(ICMP_HEAD);
	pSendMessage=(PICMP_MESSAGE)SendBuf;
	pRecvMessage=(PICMP_MESSAGE)RecvBuf;
	pSendMessage->IcmpHeader.Type=ICMP_ECHO_REQUEST;
	pSendMessage->IcmpHeader.Code=0;
	srand(DevNetID^clock());
	SendSeq=rand()>>16;
	pSendMessage->IcmpHeader.HeadData.StdData.IcmpID=rand()>>16;
	pSendMessage->IcmpHeader.HeadData.StdData.IcmpID=htons(pSendMessage->IcmpHeader.HeadData.StdData.IcmpID);
	pSendMessage->IcmpHeader.HeadData.StdData.IcmpSeq=SendSeq++;
	pSendMessage->IcmpHeader.HeadData.StdData.IcmpSeq=htons(pSendMessage->IcmpHeader.HeadData.StdData.IcmpSeq);
	pSendMessage->IcmpHeader.CheckSum=0;
	pSendMessage->IcmpHeader.CheckSum=IcmpCheckSum((U16 *)&pSendMessage->IcmpHeader,IcmpLen);
	memset(&RemoteAddr,0,sizeof(RemoteAddr));
	RemoteAddr.sin_family=AF_INET;
	RemoteAddr.sin_addr.s_addr=htonl(DestIP);

	while(recvfrom(Fd,(char *)pRecvMessage,sizeof(IP_CONST_HEAD)+IcmpLen,0,(struct sockaddr *)&LocalAddr,&Size)>0);
	for(i=0;i<(int)MaxCount;i++)
	{
		if(sendto(Fd,(char *)&pSendMessage->IcmpHeader,IcmpLen,0,(struct sockaddr *)&RemoteAddr,sizeof(RemoteAddr)) == SOCKET_ERROR)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳICMP���Ľӿڷ��ͻ���������ʧ��(%s)...\r\n",strerror(errno));
			break;
		}
		SendCount++;
		MSLEEP(Interval);
		while(recvfrom(Fd,(char *)pRecvMessage,sizeof(IP_CONST_HEAD)+IcmpLen
			,0,(struct sockaddr *)&LocalAddr,&Size)>0)
	    {
			RecvCount += IcmpPingValidate(DestIP,pSendMessage,pRecvMessage,IcmpLen,bStrick,bIsAnalyze);
	    }
		pSendMessage->IcmpHeader.HeadData.StdData.IcmpSeq=SendSeq++;
		pSendMessage->IcmpHeader.HeadData.StdData.IcmpSeq=htons(pSendMessage->IcmpHeader.HeadData.StdData.IcmpSeq);
		pSendMessage->IcmpHeader.CheckSum=0;
		pSendMessage->IcmpHeader.CheckSum=IcmpCheckSum((U16 *)&pSendMessage->IcmpHeader,IcmpLen);
	}

	close(Fd);
	if(bStrick)
		return (SendCount>0 && SendCount==RecvCount)?TRUE:FALSE;
	else
		return (SendCount>0 && RecvCount>0)?TRUE:FALSE;
}







/******************************************************************************************************************************************************
*
*
*	������ʱ��Э��SNTPʵ��
*
*
***************************************************************************************************************************************************************/




/*
 ************************************************************************************************************************************************************************     
 *��������: SntpPacketSwap
 *��������: ������ʱ��Э��SNTP�����ֽ���任
 *��������: ������ʱ��Э��SNTP�������ġ����絽���������任���
 *�������: �ֽ���任��ļ�����ʱ��Э��SNTP��������
 *��������: ��
 *��������: ZCQ/2007/07/17
 *ȫ������: ��
 *����˵��: ��
 ************************************************************************************************************************************************************************       
 */
static void SntpPacketSwap(PNTP_BASE_MSG pMsg,BOOL IsNetToHost)
{
	if(IsNetToHost)
	{
		pMsg->OriginateTime.Fraction=ntohl(pMsg->OriginateTime.Fraction);
		pMsg->OriginateTime.Seconds=ntohl(pMsg->OriginateTime.Seconds);
		pMsg->ReceiveTime.Fraction=ntohl(pMsg->ReceiveTime.Fraction);
		pMsg->ReceiveTime.Seconds=ntohl(pMsg->ReceiveTime.Seconds);
		pMsg->ReferenceID=ntohl(pMsg->ReferenceID);
		pMsg->ReferenceTime.Fraction=ntohl(pMsg->ReferenceTime.Fraction);
		pMsg->ReferenceTime.Seconds=ntohl(pMsg->ReferenceTime.Seconds);
		pMsg->RootDelay=ntohl(pMsg->RootDelay);
		pMsg->RootDispersion=ntohl(pMsg->RootDispersion);
		pMsg->TransmitTime.Fraction=ntohl(pMsg->TransmitTime.Fraction);
		pMsg->TransmitTime.Seconds=ntohl(pMsg->TransmitTime.Seconds);
	}
	else
	{
		pMsg->OriginateTime.Fraction=htonl(pMsg->OriginateTime.Fraction);
		pMsg->OriginateTime.Seconds=htonl(pMsg->OriginateTime.Seconds);
		pMsg->ReceiveTime.Fraction=htonl(pMsg->ReceiveTime.Fraction);
		pMsg->ReceiveTime.Seconds=htonl(pMsg->ReceiveTime.Seconds);
		pMsg->ReferenceID=htonl(pMsg->ReferenceID);
		pMsg->ReferenceTime.Fraction=htonl(pMsg->ReferenceTime.Fraction);
		pMsg->ReferenceTime.Seconds=htonl(pMsg->ReferenceTime.Seconds);
		pMsg->RootDelay=htonl(pMsg->RootDelay);
		pMsg->RootDispersion=htonl(pMsg->RootDispersion);
		pMsg->TransmitTime.Fraction=htonl(pMsg->TransmitTime.Fraction);
		pMsg->TransmitTime.Seconds=htonl(pMsg->TransmitTime.Seconds);
	}
}

/*
 ************************************************************************************************************************************************************************     
 *��������: SntpClientAgent
 *��������: ������ʱ��Э��ͻ��˴������
 *��������: ��
 *�������: ��
 *��������: ��
 *��������: ZCQ/2007/11/14
 *ȫ������: SntpServer
 *����˵��: ��ɼ�����ʱ��Э��ͻ���Э�������ʵ���Զ���ʱ��û��ʵ����֤���ƣ�֧��˫�㲥�����������ʱ
 ************************************************************************************************************************************************************************       
 */
static void *SntpClientAgent(void *pArg)
{
	U32					Timeout,LastTime,IdleTimeout;
	Boolean				IsEnableBroadcast,IsFirstSend;
	int					ReadyFdNum,MsgLen;
	NTP_TIMESTAMP		LastTimeStamp;
	struct sockaddr_in	LocalAddr,RemoteAddr;
	struct timespec     RealTime;
	fd_set				Rset;
	NTP_FULL_MSG		Msg;
	
	memset(&Msg,0,sizeof(Msg));
	memset(&LocalAddr,0,sizeof(LocalAddr));
	memset(&RemoteAddr,0,sizeof(RemoteAddr));
	LocalAddr.sin_family=AF_INET;
	RemoteAddr.sin_family=AF_INET;
	clock_gettime(CLOCK_REALTIME,&RealTime);
	LastTime=RealTime.tv_sec;
	LastTimeStamp.Seconds=0;
	LastTimeStamp.Fraction=0;
	IsEnableBroadcast=(SntpServer.PollTimer==0 || SntpServer.ServerAddr.sin_addr.s_addr==0)?True:False;
	for(IsFirstSend=True;;)
	{
		if(SntpServer.IsReqQuit)
		{
			SntpServer.IsReqQuit=FALSE;
			break;
		}
		pthread_testcancel();
		MSLEEP(SNTP_START_DELAY);

		LocalAddr.sin_addr.s_addr=htonl(INADDR_ANY);
		LocalAddr.sin_port=SntpServer.ServerAddr.sin_port;
		SntpServer.ServerFd=CreateSocket(SOCKET_TYPE_UDP,SOCKET_UDP_MODE_BCAST,&LocalAddr,NULL,INVALID_DEV_NET_ID);
		if(SntpServer.ServerFd==SOCKET_INVALID_FD)
		{
			NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ������ʱ��Э��ͻ��˴���������˿�ʧ��(%s)...\r\n",strerror(errno));
			continue;
		}
		if(IN_CLASSD(ntohl(SntpServer.ServerAddr.sin_addr.s_addr)))
		{
			struct  ip_mreq	McastAddr;

			McastAddr.imr_interface.s_addr=htonl(INADDR_ANY);
			McastAddr.imr_multiaddr.s_addr=SntpServer.ServerAddr.sin_addr.s_addr;
			if(setsockopt(SntpServer.ServerFd,IPPROTO_IP,IP_ADD_MEMBERSHIP,(const void *)&McastAddr,sizeof(McastAddr)))
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ������ʱ��Э��ͻ��˴������ಥ��ʧ��(%s)...\r\n",strerror(errno));
				KillSocketEx(&SntpServer.ServerFd,ntohl(SntpServer.ServerAddr.sin_addr.s_addr));
				continue;
			}
		}

		for(Timeout=SNTP_POLL_MIN_DELAY,IdleTimeout=0;;)
		{
			if(SntpServer.IsReqQuit)
				break;
			pthread_testcancel();

			FD_ZERO(&Rset);
			FD_SET(SntpServer.ServerFd,&Rset);
			if(!WaitForSocket(SntpServer.ServerFd,&Rset,NULL,&Timeout,&ReadyFdNum))
			{
				NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ������ʱ��Э��ͻ��˴��������Ϣʧ��(%s)...\r\n",strerror(errno));
				KillSocketEx(&SntpServer.ServerFd,ntohl(SntpServer.ServerAddr.sin_addr.s_addr));
				break;
			}
			if(FD_ISSET(SntpServer.ServerFd,&Rset))
			{
				int	 AddrLen;

				AddrLen=sizeof(RemoteAddr);
				MsgLen=recvfrom(SntpServer.ServerFd,(char *)&Msg,sizeof(Msg),0,(struct sockaddr *)&RemoteAddr,&AddrLen);
				if(MsgLen<0)
				{
					NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ������ʱ��Э��ͻ��˴��������Ϣʧ��(%s)...\r\n",strerror(errno));
					KillSocketEx(&SntpServer.ServerFd,ntohl(SntpServer.ServerAddr.sin_addr.s_addr));
					break;
				}
				if(MsgLen>0)
				{
					SntpServer.RecvCount++;
					if(MsgLen==sizeof(Msg.BaseMsg))
					{
						SntpPacketSwap(&Msg.BaseMsg,TRUE);
						for(;;)
						{
							if(Msg.BaseMsg.Header.LeapIndicator==NTP_LI_ALARM)
								break;
							if(Msg.BaseMsg.Header.Version<NTP_VN_VERSION1 
								|| Msg.BaseMsg.Header.Version>NTP_VN_VERSION4)
									break;
							if(Msg.BaseMsg.Header.Mode!=NTP_MODE_SERVER 
								&& Msg.BaseMsg.Header.Mode!=NTP_MODE_BROADCAST)
									break;
							if(Msg.BaseMsg.Header.Mode==NTP_MODE_SERVER
								&& Msg.BaseMsg.Header.Version!=SNTP_CURR_DEFAULT_VERSION)
									break;
							if(!IsEnableBroadcast && Msg.BaseMsg.Header.Mode==NTP_MODE_BROADCAST)
								break;
							if(Msg.BaseMsg.Header.Stratum<NTP_STRATUM_PRIMARY 
								|| Msg.BaseMsg.Header.Stratum>=NTP_STRATUM_MAX_SECONDARY_REFERENCE)
									break;
							if(Msg.BaseMsg.Header.Mode==NTP_MODE_SERVER)
							{
								if(Msg.BaseMsg.OriginateTime.Seconds!=LastTimeStamp.Seconds
									|| Msg.BaseMsg.OriginateTime.Fraction!=LastTimeStamp.Fraction)
										break;
								if(Msg.BaseMsg.ReceiveTime.Seconds==0
									&& Msg.BaseMsg.ReceiveTime.Fraction==0)
										break;
								if(Msg.BaseMsg.TransmitTime.Seconds==0
									&& Msg.BaseMsg.TransmitTime.Fraction==0)
										break;
								if(!SntpServer.IsReady)
								{
									if(SntpServer.SecondServerAddr.sin_addr.s_addr!=RemoteAddr.sin_addr.s_addr)
									{
										SntpServer.IsReady=True;
										if(SntpServer.ServerAddr.sin_addr.s_addr!=RemoteAddr.sin_addr.s_addr)
										{
											if(IN_CLASSD(ntohl(SntpServer.ServerAddr.sin_addr.s_addr)))
											{
												struct  ip_mreq	McastAddr;

												McastAddr.imr_interface.s_addr=htonl(INADDR_ANY);
												McastAddr.imr_multiaddr.s_addr=SntpServer.ServerAddr.sin_addr.s_addr;
												if(setsockopt(SntpServer.ServerFd,IPPROTO_IP,IP_DROP_MEMBERSHIP,(const void *)&McastAddr,sizeof(McastAddr)))
													NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ������ʱ��Э��ͻ��˴������˿��˳��ಥ��ʧ��(%s)...\r\n",strerror(errno));
											}
											SntpServer.ServerAddr.sin_addr.s_addr=RemoteAddr.sin_addr.s_addr;
											SntpServer.LastServerAddr=SntpServer.ServerAddr;
										}
									}
								}
								if(SntpServer.LastServerAddr.sin_addr.s_addr==RemoteAddr.sin_addr.s_addr)
								{
									double OriginateTime,ReceiveTime,TransmitTime,ArriveTime,RoundtripTime,OffsetTime;

									clock_gettime(CLOCK_REALTIME,&RealTime);
									OriginateTime=Msg.BaseMsg.OriginateTime.Seconds
										+(double)Msg.BaseMsg.OriginateTime.Fraction/65536.0/65536.0;
									ReceiveTime=Msg.BaseMsg.ReceiveTime.Seconds
										+(double)Msg.BaseMsg.ReceiveTime.Fraction/65536.0/65536.0;
									TransmitTime=Msg.BaseMsg.TransmitTime.Seconds
										+(double)Msg.BaseMsg.TransmitTime.Fraction/65536.0/65536.0;
									ArriveTime=RealTime.tv_sec+(double)RealTime.tv_nsec/1000000000.0;
									RoundtripTime=(ArriveTime-OriginateTime)-(ReceiveTime-TransmitTime);
									OffsetTime=((ReceiveTime-OriginateTime)+(TransmitTime-ArriveTime))/2;
									TransmitTime=ArriveTime+OffsetTime;
									Msg.BaseMsg.TransmitTime.Seconds=(U32)TransmitTime;
									Msg.BaseMsg.TransmitTime.Fraction
										=(TransmitTime-Msg.BaseMsg.TransmitTime.Seconds)*65536.0*65536.0;
									SntpServer.RoundtripDelay=RoundtripTime*1000;
								}
								else
									break;
							}
							else
							{
								if(Msg.BaseMsg.TransmitTime.Seconds==0
									&& Msg.BaseMsg.TransmitTime.Fraction==0)
										break;
							}
							SntpServer.MsgCount++;
							if(SntpServer.pLocalTimeSet!=NULL)
							{
								Msg.BaseMsg.TransmitTime.Seconds=Msg.BaseMsg.TransmitTime.Seconds-(((DWORD)365*70+70/4)*24*3600);
								(*SntpServer.pLocalTimeSet)(Msg.BaseMsg.TransmitTime.Seconds);
							}
							break;
						}
					}
				}
			}
			if(ReadyFdNum==0 && IsEnableBroadcast)
			{
				IdleTimeout += Timeout;
				if(IdleTimeout>=SNTP_IDLE_MAX_TIME_OUT)
				{
					IdleTimeout=0;
					NET_API_LOG(LOG_EVENT_LEVEL_INFO,"ϵͳ������ʱ��Э��ͻ��˴��������������(Recv=%d,Msg=%d,Delay=%d)...\r\n"
						,SntpServer.RecvCount,SntpServer.MsgCount,SntpServer.RoundtripDelay);
				}
			}
			if(IsEnableBroadcast)continue;

			clock_gettime(CLOCK_REALTIME,&RealTime);
			if(LastTime>(U32)RealTime.tv_sec)
				LastTime=RealTime.tv_sec;
			if(IsFirstSend)
			{
				IsFirstSend=False;
				LastTime=RealTime.tv_sec-SntpServer.PollTimer;
			}
			if(LastTime+SntpServer.PollTimer<=(U32)RealTime.tv_sec)
			{
				LastTime=RealTime.tv_sec;
				memset(&Msg,0,sizeof(Msg));
				Msg.BaseMsg.Header.LeapIndicator=NTP_LI_NORMAL;
				Msg.BaseMsg.Header.Version=SNTP_CURR_DEFAULT_VERSION;
				Msg.BaseMsg.Header.Mode=NTP_MODE_CLIENT;
				Msg.BaseMsg.TransmitTime.Seconds=RealTime.tv_sec;
				Msg.BaseMsg.TransmitTime.Fraction
					=(U32)((double)RealTime.tv_nsec/1000000000.0*65536.0*65536.0);
				LastTimeStamp=Msg.BaseMsg.TransmitTime;
				SntpPacketSwap(&Msg.BaseMsg,FALSE);
				if(SntpServer.RunCount++%2 && SntpServer.IsEnableSecondServer)
					SntpServer.LastServerAddr=SntpServer.SecondServerAddr;
				else
					SntpServer.LastServerAddr=SntpServer.ServerAddr;
				if(!SendUdpTo(SntpServer.ServerFd,&Msg.BaseMsg,sizeof(Msg.BaseMsg),&SntpServer.LastServerAddr))
				{
					NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ������ʱ��Э��ͻ��˴�������Ϣʧ��(%s)...\r\n",strerror(errno));
					KillSocketEx(&SntpServer.ServerFd,ntohl(SntpServer.ServerAddr.sin_addr.s_addr));
					break;
				}
				NET_API_LOG(LOG_EVENT_LEVEL_INFO,"ϵͳ������ʱ��Э��ͻ��˴��������������(Recv=%d,Msg=%d,Delay=%d)...\r\n"
					,SntpServer.RecvCount,SntpServer.MsgCount,SntpServer.RoundtripDelay);
			}				
		}
	}
	return NULL;
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_SntpStart
 *��������: ������ʱ��Э���������
 *��������: ʱ�����ûص�������������ʱ��Э���������ַ��˿ڡ�ʱ��У������(s)���ڶ���������ַ
 *�������: ��
 *��������: �ɹ�(TRUE)/ʧ��(FALSE)
 *��������: ZCQ/2007/11/14
 *ȫ������: SntpServer,sSntpServerMutex
 *����˵��: �Զ�����������ʱ��Э��ͻ��˴������
 ************************************************************************************************************************************************************************       
 */
extern BOOL ZFY_NET_SntpStart(PLOCAL_TIME_SET_FUNC pTimeSet,U32 ServerIP,U16 ServerPort,U16 PollTimer,U32 SecondServerIP)
{
	int	OldStatus;

	if(ServerPort==0)
		return FALSE;
	PTHREAD_MUTEX_SAFE_LOCK(sSntpServerMutex,OldStatus);
	if(SntpServer.IsStarted)
	{
		PTHREAD_MUTEX_SAFE_UNLOCK(sSntpServerMutex,OldStatus);
		NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ������ʱ��Э��ͻ��˴�������Ѿ�����,����ֹͣ��������...\r\n");
		return TRUE;
	}
	memset(&SntpServer,0,sizeof(SntpServer));
	SntpServer.IsReady=FALSE;
	SntpServer.IsReqQuit=FALSE;
	SntpServer.pLocalTimeSet=pTimeSet;
	SntpServer.PollTimer=PollTimer;
	SntpServer.ServerFd=SOCKET_INVALID_FD;
	SntpServer.SntpServerPID=INVALID_PTHREAD_ID;
	SntpServer.RecvCount=0;
	SntpServer.MsgCount=0;
	SntpServer.RoundtripDelay=0;
	SntpServer.ServerAddr.sin_family=AF_INET;
	SntpServer.ServerAddr.sin_addr.s_addr=htonl(ServerIP);
	SntpServer.ServerAddr.sin_port=htons(ServerPort);
	SntpServer.LastServerAddr=SntpServer.ServerAddr;
	SntpServer.SecondServerAddr.sin_family=AF_INET;
	SntpServer.SecondServerAddr.sin_addr.s_addr=htonl(SecondServerIP);
	SntpServer.SecondServerAddr.sin_port=htons(ServerPort);
	if(SecondServerIP!=INADDR_ANY && SecondServerIP!=INADDR_NONE
		&& (IN_CLASSA(SecondServerIP) || IN_CLASSB(SecondServerIP) || IN_CLASSC(SecondServerIP)))
			SntpServer.IsEnableSecondServer=TRUE;
	else
		SntpServer.IsEnableSecondServer=FALSE;
	SntpServer.CallerThread=pthread_self();
	SntpServer.CallerPID=getpid();
	if(pthread_create(&SntpServer.SntpServerPID,NULL,SntpClientAgent,NULL)!=STD_SUCCESS)
	{
		PTHREAD_MUTEX_SAFE_UNLOCK(sSntpServerMutex,OldStatus);
		NET_API_LOG(LOG_EVENT_LEVEL_ERR,"ϵͳ������ʱ��Э��ͻ��˴����������ʧ��(%s)...\r\n",strerror(errno));
		return FALSE;
	}
	SntpServer.IsStarted=TRUE;
	PTHREAD_MUTEX_SAFE_UNLOCK(sSntpServerMutex,OldStatus);
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *��������: ZFY_NET_SntpStop
 *��������: ������ʱ��Э�����ֹͣ
 *��������: ��
 *�������: ��
 *��������: ��
 *��������: ZCQ/2007/07/17
 *ȫ������: SntpServer,sSntpServerMutex
 *����˵��: ���ټ�����ʱ��Э��ͻ��˴������
 ************************************************************************************************************************************************************************       
 */
extern void ZFY_NET_SntpStop(void)
{
	int	OldStatus;

	PTHREAD_MUTEX_SAFE_LOCK(sSntpServerMutex,OldStatus);
	if(SntpServer.IsStarted)
	{
		if(!pthread_equal(pthread_self(),SntpServer.CallerThread))
			NET_API_LOG(LOG_EVENT_LEVEL_DEBUG,"ϵͳ������ʱ��Э��ͻ��˴���������ر��̲߳�һ��(0x%X,0x%X)...\r\n",(unsigned int)pthread_self(),(unsigned int)SntpServer.CallerThread);
		SntpServer.IsReqQuit=TRUE;
		usleep(PTHREAD_DEFAULT_QUIT_TIMEOUT_US);
		if(SntpServer.IsReqQuit)
			pthread_cancel(SntpServer.SntpServerPID);
		pthread_join(SntpServer.SntpServerPID,NULL);
		SntpServer.SntpServerPID=INVALID_PTHREAD_ID;
		if(SntpServer.ServerFd!=SOCKET_INVALID_FD)
			KillSocketEx(&SntpServer.ServerFd,ntohl(SntpServer.ServerAddr.sin_addr.s_addr));
		SntpServer.IsStarted=FALSE;
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sSntpServerMutex,OldStatus);
}








/*
 ************************************************************************************************************************************************************************     
 *��������:
 *��������:
 *��������:
 *�������:
 *��������:
 *��������:
 *ȫ������:
 *����˵��:
 ************************************************************************************************************************************************************************       
 */




/******************************************NetServer.c File End******************************************************************************************/
