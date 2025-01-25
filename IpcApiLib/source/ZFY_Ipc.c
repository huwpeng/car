/*
 ***************************************************************************************************************************
 ********************************************************************************************************************************
 *			PROGRAM MODULE
 *
 *			$Workfile:		ZFY_Ipc.c			$
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
 *	实现IPC抓拍录像识别功能
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
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>
#include "NetServer.h"
#include "ZFY_Ipc.h"
#include "LogEventServer.h"
#include "YaAppPublic.h"
#include "ZFY_sha256.h"
#include <sys/vfs.h> 
#include "iksemel.h"


/*
 ************************************************************************************************************************************************************************
 * 常数定义
 ************************************************************************************************************************************************************************
*/
#define	IPC_PIC_MAX_FILE_NUM					15000
#define FSU_IPC_DEL_PIC_FILE_MAX_NUM			(IPC_PIC_MAX_FILE_NUM/100)
#define HTTP_BUFFSIZE 							2048
#define	IPC_PIC_MAX_FILE_LEN					128
#define	IPC_PIC_MAX_ACCOUNT_NUM					3
#define FSU_IPC_PIC_FILE_SIZE_MAX				(200*1024)
#define FSU_IPC_ALARM_CAP_BASE_DIR				"/opt/"			
#define FSU_IPC_MOUNT_DIR						"/opt/"
#define FSU_IPC_UPLOAD_DIR						"/srv/ftp/UPLOAD/"

#define FSU_IPC_SECOND_USER						"admin"
#define FSU_IPC_SECOND_PWD						"admin"
#define FSU_IPC_THIRD_USER						"admin"
#define FSU_IPC_THIRD_PWD						"admin123456"

#define SP_SYS_IPC_NUM_MAX						4

/*
 ************************************************************************************************************************************************************************
 类型定义
 ************************************************************************************************************************************************************************
*/
typedef struct _IPC_LOGIN_CAP
{
	char		sessionID[64+4];
	char		challenge[128+4];
	char		iterations[16+4];
	char		isIrreversible[16+4];
	char		salt[256+4];
	char		sessionIDVersion[16+4];
}IPC_LOGIN_CAP,*PIPC_LOGIN_CAP;

typedef struct _IPC_SESSION
{
	char		isSupportLoginTiming[16+4];
	char		statusValue[16+4];
	char		statusString[32+4];
	char		sessionID[128+4];
	char		sessionIDVersion[16+4];
}IPC_SESSION,*PIPC_SESSION;

typedef struct _IPC_PIC_FILE_DESC
{
	char		*filename;
	long int	filetime;
}IPC_PIC_FILE_DESC,*PIPC_PIC_FILE_DESC;

typedef struct tIPC_SERVER
{
	BOOL		IsInitReady;
	BOOL		IsReqWatchQuit;
	pthread_t	ptWatchThreadID;
	char		m_FSUID[48];
	short 		m_IpcServerPort;
	char		m_strIpcServerIP[32];
	char		m_strLoginUser[32];
	char		m_strLoginPwd[32];
	char		m_strCapPath[64];
	char		m_strRecPath[64];
}IPC_SERVER,*PIPC_SERVER;

/*
 ************************************************************************************************************************************************************************
 * 函数原型
 ************************************************************************************************************************************************************************
*/


/*
 ************************************************************************************************************************************************************************
 * 全局变量
 ************************************************************************************************************************************************************************
*/
static pthread_mutex_t			sPicFileOpMutex=PTHREAD_MUTEX_INITIALIZER;
static char 					PicFileName[IPC_PIC_MAX_FILE_NUM][IPC_PIC_MAX_FILE_LEN];
static char 					*pPicFile[IPC_PIC_MAX_FILE_NUM];
static IPC_PIC_FILE_DESC		gFileList[IPC_PIC_MAX_FILE_NUM];
static DWORD					gFileHead,gFileTail,gIpcAccountIndex[SP_SYS_IPC_NUM_MAX];
static BOOL 					sFsuIpcIsInitReady=FALSE;
static IPC_SERVER				sIpcServer;


/*
 ****************************************************************************************************************************************************
 * 函数定义
 *****************************************************************************************************************************************************
*/
static int blockUntilReadable(int socket) 
{
	fd_set rd_set;
	int result = -1;
	unsigned numFds;
	struct timeval 	t;

	do 
	{
		FD_ZERO(&rd_set);
		if (socket < 0) 
			break;
		FD_SET((unsigned) socket, &rd_set);
		numFds = socket+1;
		t.tv_sec=0;
		t.tv_usec=5000000;
		result = select(numFds, &rd_set, NULL, NULL,&t);
		if (result == 0) 
		{
			break; 
		} 
		else if (result <= 0) 
		{
			ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"select() error\n ");
			break;
		}
		if (!FD_ISSET(socket, &rd_set)) 
		{
			ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"select() error - !FD_ISSET\n");
			break;
		}	
	} while (0);

	return result;
}

static char* getLine(char* startOfLine) 
{
	char* ptr;
	
	for (ptr = startOfLine; *ptr != '\0'; ++ptr) 
	{
		if (*ptr == '\r' || *ptr == '\n') 
		{
			*ptr++ = '\0';
			if (*ptr == '\n') 
				++ptr;
			return ptr;
		}
	}
	return NULL;
}

static int parseResponseCode(char* line, unsigned int * responseCode) 
{
	if (sscanf(line, "%*s%u", responseCode) != 1) 
	{
		ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"no response code in line\n");
		return -1;
	}
	return 0;
}

static int getResponse(int socketNum,char* responseBuffer,unsigned responseBufferSize) 
{
	int		fSocketNum;
	char	*lastToCheck=NULL;
	char	*p=NULL;
	int		bytesRead=0;
	ssize_t bytesReadNow=0;

	fSocketNum=socketNum;
	if(responseBufferSize==0 || NULL==responseBuffer)
		return 0;
	*(responseBuffer)='\0';
	while(bytesRead<(int)responseBufferSize) 
	{
		lastToCheck=NULL;
		if(blockUntilReadable(fSocketNum)<=0)
		{
			ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"socket is unreadable\n");
			break;
		}
		bytesReadNow=recv(fSocketNum,(unsigned char*)(responseBuffer+bytesRead),responseBufferSize-bytesRead,0);
		if(bytesReadNow<0) 
		{
			ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"HTTP response was truncated\n");
			break;
		}
		else if(bytesReadNow==0)
		{
			ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"HTTP response recv OK end\n");
			break;
		}
		else
			bytesRead += bytesReadNow;
	}
	return bytesRead;
}

static BOOL CreateDigestRandom(char *pRandom,int RandomLen)
{
	int k=0,j=0,flag=0;
	unsigned int seed;
	
	if(pRandom==NULL || RandomLen<=0)
		return FALSE;
	pRandom[0]='\0';
	seed=(unsigned)time(NULL);
	srand(seed);
	for(j=0;j<RandomLen-1;j++)
	{
		unsigned int random_num=rand();
		flag = random_num%3;
		if(flag == 0)
		{
			pRandom[k++]='0'+random_num%10;
		}
		else if(flag == 1)
		{
			pRandom[k++]='a'+random_num%26;
		}
		else if(flag == 2)
		{
			pRandom[k++]='A'+random_num%26;
		}
		srand(random_num);
	}
	pRandom[k]='\0';
	return TRUE;
}

static BOOL GetTargetString(const char *pSrcString,char *pStartString,char EndChar,char *pDstString,long DstLen)
{
	char *pStart = NULL;
	char *pEnd = NULL;
	
	if(pSrcString==NULL || pStartString==NULL || pDstString==NULL || DstLen<=0)
		return FALSE;
	pStart = strstr(pSrcString,pStartString);
	if(pStart==NULL)
		return FALSE;
	pStart += strlen(pStartString);
	pEnd = strchr(pStart,EndChar);
	if(pEnd==NULL)
		return FALSE;
	if(DstLen < pEnd-pStart)
		return FALSE;
	pDstString[pEnd-pStart]='\0';
	memcpy(pDstString,pStart,pEnd-pStart);
	return TRUE;
}

static BOOL GreateDigestAuthResponse(const char *pRequest,long RequestSize,const char *pUrl,const char *pUser,const char *pPwd,char *pResponse)
{
	char A1[100],A2[80],md5_A2[16],md5_A1[256],contact[512],rsp[16] ;
	char *Cnonce=NULL,*nc="00000001";
	char Realm[128],Nonce[128],Qop[128];
	unsigned char md5_buf[256],CnonceLen=33;;
	
	if(!GetTargetString(pRequest,"realm=\"",'\"',Realm,sizeof(Realm)))
		return FALSE;
	if(!GetTargetString(pRequest,"nonce=\"",'\"',Nonce,sizeof(Nonce)))
		return FALSE;
	if(!GetTargetString(pRequest,"qop=\"",'\"',Qop,sizeof(Qop)))
		return FALSE;
	Cnonce=(char*)malloc(CnonceLen*sizeof(char));
	if(Cnonce==NULL)
		return FALSE;
	if(!CreateDigestRandom(Cnonce,9))
	{
		free(Cnonce);
		return FALSE;
	}
	sprintf(A1,"%s:%s:%s",pUser,Realm,pPwd);
	YALAP_MD5_StdEncrypt((unsigned char*)A1,strlen(A1),md5_buf);
	sprintf(md5_A1,"%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",md5_buf[0],md5_buf[1],md5_buf[2],md5_buf[3],md5_buf[4],md5_buf[5],
		md5_buf[6],md5_buf[7],md5_buf[8],md5_buf[9],md5_buf[10],md5_buf[11],md5_buf[12],md5_buf[13],md5_buf[14],md5_buf[15]);
	sprintf(A2,"GET:%s",pUrl);
	YALAP_MD5_StdEncrypt((unsigned char*)A2,strlen(A2),md5_buf);
	sprintf(md5_A2,"%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",md5_buf[0],md5_buf[1],md5_buf[2],md5_buf[3],md5_buf[4],md5_buf[5],
		md5_buf[6],md5_buf[7],md5_buf[8],md5_buf[9],md5_buf[10],md5_buf[11],md5_buf[12],md5_buf[13],md5_buf[14],md5_buf[15]);
	sprintf(contact,"%s:%s:%s:%s:%s:%s",md5_A1,Nonce,nc,Cnonce,Qop,md5_A2);
	YALAP_MD5_StdEncrypt((unsigned char*)contact,strlen(contact),md5_buf);
	sprintf(rsp,"%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",md5_buf[0],md5_buf[1],md5_buf[2],md5_buf[3],md5_buf[4],md5_buf[5],
		md5_buf[6],md5_buf[7],md5_buf[8],md5_buf[9],md5_buf[10],md5_buf[11],md5_buf[12],md5_buf[13],md5_buf[14],md5_buf[15]);
	if(pResponse!=NULL && pUrl!=NULL)
		sprintf(pResponse,"username=\"%s\",realm=\"%s\",qop=\"%s\",uri=\"%s\",nonce=\"%s\",nc=%s,cnonce=\"%s\",response=\"%s\"",pUser,Realm,Qop,pUrl,Nonce,nc,Cnonce,rsp);
	free(Cnonce);
	if(pResponse==NULL)
		return FALSE;
    return TRUE;
}

static char* Iks_cdata(iks *x)
{
	iks *child;
	char *r;
	static char *ZeroStr="";

	if(child=iks_child(x))
	{
		r=iks_cdata(child);
		if(NULL==r)
			r=ZeroStr;
	}
	else
		r=ZeroStr;
	return r;
}

static BOOL IpcLoginCapParse(char *pRecvData,DWORD DataLen,IPC_LOGIN_CAP *pLoginCap)
{
	int i=0,j=0,err=0,postnum=0;
	iks *x=NULL,*y=NULL;

	if(pRecvData==NULL||DataLen==0||pLoginCap==NULL)
		return FALSE;
	x=iks_tree(pRecvData,DataLen,&err);
	if(err!=IKS_OK)
	{
		iks_delete(x);
		ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"系统IPC LoginCap 树形分析失败...\r\n");
		return FALSE;
	}
	if(x==NULL)
		return FALSE;
	if(!(y=iks_find(x,"sessionID")))
	{
		iks_delete(x);
		return FALSE;
	}
	strncpy(pLoginCap->sessionID,Iks_cdata(y),sizeof(pLoginCap->sessionID)-1);
	if(!(y=iks_find(x,"challenge")))
	{
		iks_delete(x);
		return FALSE;
	}
	strncpy(pLoginCap->challenge,Iks_cdata(y),sizeof(pLoginCap->challenge)-1);
	if(!(y=iks_find(x,"iterations")))
	{
		iks_delete(x);
		return FALSE;
	}
	strncpy(pLoginCap->iterations,Iks_cdata(y),sizeof(pLoginCap->iterations)-1);
	if(!(y=iks_find(x,"isIrreversible")))
	{
		iks_delete(x);
		return FALSE;
	}
	strncpy(pLoginCap->isIrreversible,Iks_cdata(y),sizeof(pLoginCap->isIrreversible)-1);
	if(!(y=iks_find(x,"salt")))
	{
		iks_delete(x);
		return FALSE;
	}
	strncpy(pLoginCap->salt,Iks_cdata(y),sizeof(pLoginCap->salt)-1);
	if((y=iks_find(x,"sessionIDVersion")))
	{
		strncpy(pLoginCap->sessionIDVersion,Iks_cdata(y),sizeof(pLoginCap->sessionIDVersion)-1);
	}
	iks_delete(x);
	return TRUE;
}

static BOOL IpcSessionParse(char *pRecvData,DWORD DataLen,IPC_SESSION *pSession)
{
	int i=0,j=0,err=0,postnum=0;
	iks *x=NULL,*y=NULL;

	if(pRecvData==NULL||DataLen==0||pSession==NULL)
		return FALSE;
	GetTargetString(pRecvData,"WebSession_",';',pSession->sessionID,sizeof(pSession->sessionID));
	x=iks_tree(pRecvData,DataLen,&err);
	if(err!=IKS_OK)
	{
		iks_delete(x);
		ZFY_LES_LogPrintf("FSU_IPC",LOG_EVENT_LEVEL_INFO,"系统IPC Session 树形分析失败...\r\n");
		return FALSE;
	}
	if(x==NULL)
		return FALSE;
	if(!(y=iks_find(x,"statusValue")))
	{
		iks_delete(x);
		return FALSE;
	}
	strncpy(pSession->statusValue,Iks_cdata(y),sizeof(pSession->statusValue)-1);
	if(!(y=iks_find(x,"statusString")))
	{
		iks_delete(x);
		return FALSE;
	}
	strncpy(pSession->statusString,Iks_cdata(y),sizeof(pSession->statusString)-1);
	if((y=iks_find(x,"sessionID")))
	{
		memset(pSession->sessionID,0,sizeof(pSession->sessionID));
		strncpy(pSession->sessionID,Iks_cdata(y),sizeof(pSession->sessionID)-1);
	}
	if((y=iks_find(x,"sessionIDVersion")))
	{
		strncpy(pSession->sessionIDVersion,Iks_cdata(y),sizeof(pSession->sessionIDVersion)-1);
	}
	iks_delete(x);
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_IpcLoadRecord
 *功能描述: 下载录像文件
 *输入描述: C语言标准时间,IPC标识
 *输出描述: 无
 *返回描述: 成功(TRUE)/失败(FALSE)
 *作者日期: LJJ/2024/12/26
 *全局声明: 无
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_IpcLoadRecord(time_t StartRecTime,time_t StopRecTime,DWORD IpcIndex,char *pFileName)
{
	static BOOL				IsFsuConfig[SP_SYS_IPC_NUM_MAX];
	static BOOL				IsDahuaIpc[SP_SYS_IPC_NUM_MAX];
	static BOOL				IsDahuaIpcN[SP_SYS_IPC_NUM_MAX];
	static BOOL				IsDigestAuth[SP_SYS_IPC_NUM_MAX];
    int sockfd 				=SOCKET_INVALID_FD;
    char buffer[FSU_IPC_PIC_FILE_SIZE_MAX]="";
    struct sockaddr_in   	server_addr; 
	struct sockaddr_in  	LocalAddr;
    struct hostent   		*host; 
    int 					portnumber=80;
    int 					nbytes=0; 
    char 					host_addr[32]="192.168.8.108"; 
	char 					user[32]="admin";
	char 					passwd[32]="123456abc";	
	char					HaiKanUrl[256];
	char					DahuaUrl[256];
    char 					request[2048]=""; 
    int 					send=0;
    int 					totalsend=0;
	struct in_addr 			tmpAddr;
	unsigned int 			readBufSize=sizeof(buffer)-1;
	int 					bytesRead;
	char* 					firstLine;
	char* 					lineStart;
	char* 					nextLineStart=NULL;
	unsigned 				responseCode;
	char					KeyAuth[256];
	int 					CmdLen=0;
	char*					pBase64Buf=NULL;
	char					FsuNameStr[1024];
	int						FsuNameLen=1024;
	char					start_string[256];
	char					stop_string[256];
	int						RetryCnt=0;
	struct tm 				*p=NULL;
	BOOL					IsSecurityMode=FALSE;
	BOOL					HaveLoginCap=FALSE;
	BOOL					HaveSession=FALSE;
	BOOL					HaveLogoutSession=FALSE;
	BOOL					HaveUnlockSession=FALSE;
	IPC_LOGIN_CAP			LoginCap;
	IPC_SESSION				Session;
	FILE					*pFile=NULL;
	char					DigestResponse[512];
	char					SecurityPassWord[128];
	
	if(IpcIndex>=SP_SYS_IPC_NUM_MAX)
		return FALSE;
	printf("-------ZFY_IpcLoadRecord-----\r\n");
    memset(&LocalAddr,0,sizeof(LocalAddr));
    bzero(&server_addr,sizeof(server_addr));
	memset(HaiKanUrl,0,sizeof(HaiKanUrl));
	memset(DahuaUrl,0,sizeof(DahuaUrl));
	memset(host_addr,0,sizeof(host_addr));
	strncpy(host_addr,sIpcServer.m_strIpcServerIP,sizeof(host_addr)-1);
	portnumber=sIpcServer.m_IpcServerPort;
	memset(user,0,sizeof(user));
	strncpy(user,sIpcServer.m_strLoginUser,sizeof(user)-1);
	memset(passwd,0,sizeof(passwd));
	strncpy(passwd,sIpcServer.m_strLoginPwd,sizeof(passwd)-1);
	
SnapRetry:
	server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=inet_addr(host_addr);
    server_addr.sin_port=htons(portnumber);
 	ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"try to connect IPC(%s).....\r\n",host_addr);
	sockfd=CreateSocket(SOCKET_TYPE_TCP,SOCKET_TCP_MODE_CLIENT,&LocalAddr,&server_addr,INVALID_DEV_NET_ID);
	if(sockfd==SOCKET_INVALID_FD)
		return FALSE;
	sprintf(KeyAuth,"%s:%s",user,passwd);
	CmdLen=strlen(KeyAuth);
	pBase64Buf=Base64Encode(KeyAuth,&CmdLen);
	if(pBase64Buf==NULL)
	{
		KillSocket(&sockfd);
		return FALSE;
	}
	IsDahuaIpc[IpcIndex]=TRUE;
	p=localtime(&StartRecTime);
	if(p->tm_hour+8<24)
	{
		p->tm_hour+=8;
	}
	else
	{
		p->tm_hour-=8;
		p->tm_mday++;
	}
	sprintf(start_string,"%d-%d-%d%%20%.2d:%.2d:%.2d",1900+p->tm_year,1+p->tm_mon,p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec);
	p=localtime(&StopRecTime);
	if(p->tm_hour+8<24)
	{
		p->tm_hour+=8;
	}
	else
	{
		p->tm_hour-=8;
		p->tm_mday++;
	}
	sprintf(stop_string,"%d-%d-%d%%20%.2d:%.2d:%.2d",1900+p->tm_year,1+p->tm_mon,p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec);
	if(!IsDahuaIpc[IpcIndex])
		sprintf(HaiKanUrl,"/cgi-bin/loadfile.cgi?action=startLoad&channel=1&startTime=%s&endTime=%s HTTP/1.1",start_string,stop_string);
	else
		sprintf(DahuaUrl,"/cgi-bin/loadfile.cgi?action=startLoad&channel=1&startTime=%s&endTime=%s HTTP/1.1",start_string,stop_string);
	if(IsDigestAuth[IpcIndex])
	{
		sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Cache-Control: max-age=0\r\n\
Authorization: Digest %s\r\n\
\r\n\r\n",DahuaUrl,host_addr,DigestResponse);
	}
	else
	{
		sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Cache-Control: max-age=0\r\n\
Authorization: Basic %s\r\n\
\r\n\r\n",DahuaUrl,host_addr,pBase64Buf);
	}
	send=0;
	totalsend=0; 
	nbytes=strlen(request);
	while(totalsend < nbytes)  
    {
		send=write(sockfd,request+totalsend,nbytes-totalsend); 
        if(send == -1)     
        {
			ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"httpclient send error(%s)\r\n",strerror(errno));
			BASE64_BUF_FREE(pBase64Buf);
			KillSocket(&sockfd);
			return FALSE;
        }
		totalsend += send;
    }
	memset(buffer,0,sizeof(buffer));
	bytesRead=getResponse(sockfd,buffer,readBufSize);
	if(bytesRead <= 0) 
	{
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		return FALSE;
	}
	else
	{
		ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_DEBUG,"recv:%s\r\n",buffer);
	}
	firstLine=buffer;
	nextLineStart=getLine(firstLine);
	if(sscanf(firstLine,"%*s%u",&responseCode) != 1) 
	{
		ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"no response code in firstLine\r\n");
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		return FALSE;
	}
	if(responseCode==200) 
	{
		for(;;)
		{
			unsigned int contentLength=0;
			
			lineStart=nextLineStart;
			if(lineStart==NULL || lineStart[0]=='\0')
				break;
			nextLineStart=getLine(lineStart);
			if(sscanf(lineStart,"Content-Length: %d",&contentLength)!=1 && sscanf(lineStart,"CONTENT-LENGTH: %d",&contentLength)!=1)
				continue;
			if(contentLength>0)
			{
				lineStart=nextLineStart;
				if(lineStart==NULL || lineStart[0]=='\0')
					break;
				nextLineStart=getLine(lineStart);
				lineStart=nextLineStart;
				if(lineStart==NULL || lineStart[0]=='\0')
					break;
				nextLineStart=getLine(lineStart);
				if(nextLineStart!=NULL)
				{
					char					BasePathName[64];
					char					FilePathName[64];
					time_t 					rawtime;
					int						OldStatus;
					struct 					tm *timeinfo;
					static DWORD			CurrPicIndex=0;
					static time_t			LastPicTime=0;
					unsigned int 			Total_len=0;
					unsigned int 			write_len=0;

					if(contentLength>(unsigned int)(bytesRead-(nextLineStart-buffer)))
					{
						write_len=bytesRead-(nextLineStart-buffer);
						ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"try record truncation2...\r\n");
					}
					else
					{
						write_len=contentLength;
					}
					nextLineStart+=2;
					write_len-=2;
					Total_len+=write_len;
					PTHREAD_MUTEX_SAFE_LOCK(sPicFileOpMutex,OldStatus);
					time(&rawtime);
					timeinfo=localtime(&rawtime);
					if(LastPicTime!=rawtime)
					{
						LastPicTime=rawtime;
						CurrPicIndex=1;
					}
					else
						CurrPicIndex++;
					sprintf(BasePathName,"%s_%.4d%.2d%.2d_%.2d%.2d%.2d_%.2d.%s","11",1900+timeinfo->tm_year,
						1+timeinfo->tm_mon,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,CurrPicIndex,"dav");
					PTHREAD_MUTEX_SAFE_UNLOCK(sPicFileOpMutex,OldStatus);
					sprintf(FilePathName,"%s%s",sIpcServer.m_strRecPath,BasePathName);
					ZFY_LES_LogPrintf("FSU_IPC",LOG_EVENT_LEVEL_INFO,"record file(%s)...\r\n",FilePathName);
					pFile=fopen(FilePathName,"wb");
					if(NULL!=pFile)
					{
						if(fwrite(nextLineStart,1,write_len,pFile)!=write_len)
						{
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"record failed 3...\r\n");
							fclose(pFile);
							pFile=NULL;
							goto err;
						}
						else
						{
							if(Total_len>=contentLength)
							{
								ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"record success 2...\r\n");
								fflush(pFile);
								fsync(fileno(pFile));
								fclose(pFile);
								pFile=NULL;
								if(NULL!=pFileName)
									strcpy(pFileName,FilePathName);
								break;
							}
							else
							{
								while(Total_len<contentLength)
								{
									memset(buffer,0,sizeof(buffer));
									bytesRead=getResponse(sockfd,buffer,readBufSize);
									if(bytesRead <= 0) 
									{
										ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"record success 3...\r\n");
										if(NULL!=pFileName)
											strcpy(pFileName,FilePathName);
										break;
									}
									else
									{
										ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_DEBUG,"recv2:%s\r\n",buffer);
										Total_len+=bytesRead;
										if(fwrite(buffer,1,bytesRead,pFile)!=bytesRead)
										{
											ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"record2 failed 3...\r\n");
											fclose(pFile);
											pFile=NULL;
											goto err;
										}
										
									}
								}
								if(NULL!=pFileName)
									strcpy(pFileName,FilePathName);
								fflush(pFile);
								fsync(fileno(pFile));
								fclose(pFile);
								pFile=NULL;
								break;
							}
						}
					}
					else
					{
						ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"record failed 4...\r\n");
						goto err;
					}
				}
			}
		}
	}
	else if(responseCode==401)
	{
		if(HaveLogoutSession)
		{
			BASE64_BUF_FREE(pBase64Buf);
			KillSocket(&sockfd);
			return TRUE;
		}
		for(;;)
		{
			unsigned int contentLength=0;
			BOOL retDigest=FALSE;

			lineStart=nextLineStart;
			if(lineStart==NULL || lineStart[0]=='\0')
				break;
			if(strstr(lineStart,"WWW-Authenticate: Digest") != NULL)
				IsDigestAuth[IpcIndex]=TRUE;
			else
				IsDigestAuth[IpcIndex]=FALSE;
			retDigest=GreateDigestAuthResponse(lineStart,strlen(lineStart),IsDahuaIpc[IpcIndex]?DahuaUrl:HaiKanUrl,user,passwd,DigestResponse);
			if(!retDigest)
				IsDigestAuth[IpcIndex]=FALSE;
			RetryCnt++;
			BASE64_BUF_FREE(pBase64Buf);
			KillSocket(&sockfd);
			if(RetryCnt<=5)
			{
				goto SnapRetry;
			}
			IsDigestAuth[IpcIndex]=FALSE;
			return FALSE;
		}
	}
	else if(responseCode==400)
	{
		if(IsDahuaIpc[IpcIndex])
		{
			IsDahuaIpcN[IpcIndex]=!IsDahuaIpcN[IpcIndex];
		}
		RetryCnt++;
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		if(RetryCnt<=5)
		{
			goto SnapRetry;
		}
		return FALSE;
	}
	else
	{
		RetryCnt++;
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		if(RetryCnt<=1)
		{
			IsDahuaIpc[IpcIndex] = !IsDahuaIpc[IpcIndex];
			goto SnapRetry;
		}
		return FALSE;
	}
	BASE64_BUF_FREE(pBase64Buf);
	KillSocket(&sockfd);
	if(HaveSession&&!HaveLogoutSession)
	{
		HaveLogoutSession=TRUE;
		goto SnapRetry;
	}
	return TRUE;
err:
	BASE64_BUF_FREE(pBase64Buf);
	KillSocket(&sockfd);
	return FALSE;
}

static BOOL IpcStopRecordEx(DWORD IpcIndex)
{
	static BOOL				IsFsuConfig[SP_SYS_IPC_NUM_MAX];
	static BOOL				IsDahuaIpc[SP_SYS_IPC_NUM_MAX];
	static BOOL				IsDahuaIpcN[SP_SYS_IPC_NUM_MAX];
	static BOOL				IsDigestAuth[SP_SYS_IPC_NUM_MAX];
    int sockfd 				=SOCKET_INVALID_FD;
    char buffer[FSU_IPC_PIC_FILE_SIZE_MAX]="";
    struct sockaddr_in   	server_addr; 
	struct sockaddr_in  	LocalAddr;
    struct hostent   		*host; 
    int 					portnumber=80;
    int 					nbytes=0; 
    char 					host_addr[32]="192.168.8.108"; 
	char 					user[32]="admin";
	char 					passwd[32]="123456abc";	
	char					HaiKanUrl[256];
	char					DahuaUrl[256];
    char 					request[2048]=""; 
    int 					send=0;
    int 					totalsend=0;
	struct in_addr 			tmpAddr;
	unsigned int 			readBufSize=sizeof(buffer)-1;
	int 					bytesRead;
	char* 					firstLine;
	char* 					lineStart;
	char* 					nextLineStart=NULL;
	unsigned 				responseCode;
	char					KeyAuth[256];
	int 					CmdLen=0;
	char*					pBase64Buf=NULL;
	char					FsuNameStr[1024];
	int						FsuNameLen=1024;
	int						RetryCnt=0;
	BOOL					IsSecurityMode=FALSE;
	BOOL					HaveLoginCap=FALSE;
	BOOL					HaveSession=FALSE;
	BOOL					HaveLogoutSession=FALSE;
	BOOL					HaveUnlockSession=FALSE;
	IPC_LOGIN_CAP			LoginCap;
	IPC_SESSION				Session;
	FILE					*pFile=NULL;
	char					DigestResponse[512];
	char					SecurityPassWord[128];
	
	if(IpcIndex>=SP_SYS_IPC_NUM_MAX)
		return FALSE;
	printf("-------IpcStopRecordEx-----\r\n");
    memset(&LocalAddr,0,sizeof(LocalAddr));
    bzero(&server_addr,sizeof(server_addr));
	memset(HaiKanUrl,0,sizeof(HaiKanUrl));
	memset(DahuaUrl,0,sizeof(DahuaUrl));
	memset(host_addr,0,sizeof(host_addr));
	strncpy(host_addr,sIpcServer.m_strIpcServerIP,sizeof(host_addr)-1);
	portnumber=sIpcServer.m_IpcServerPort;
	memset(user,0,sizeof(user));
	strncpy(user,sIpcServer.m_strLoginUser,sizeof(user)-1);
	memset(passwd,0,sizeof(passwd));
	strncpy(passwd,sIpcServer.m_strLoginPwd,sizeof(passwd)-1);
	
SnapRetry:
	server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=inet_addr(host_addr);
    server_addr.sin_port=htons(portnumber);
 	ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"try to connect IPC(%s).....\r\n",host_addr);
	sockfd=CreateSocket(SOCKET_TYPE_TCP,SOCKET_TCP_MODE_CLIENT,&LocalAddr,&server_addr,INVALID_DEV_NET_ID);
	if(sockfd==SOCKET_INVALID_FD)
		return FALSE;
	sprintf(KeyAuth,"%s:%s",user,passwd);
	CmdLen=strlen(KeyAuth);
	pBase64Buf=Base64Encode(KeyAuth,&CmdLen);
	if(pBase64Buf==NULL)
	{
		KillSocket(&sockfd);
		return FALSE;
	}
	IsDahuaIpc[IpcIndex]=TRUE;
	if(!IsDahuaIpc[IpcIndex])
		sprintf(HaiKanUrl,"/cgi-bin/configManager.cgi?action=setConfig&RecordMode[0].Mode=2 HTTP/1.1");
	else
		sprintf(DahuaUrl,"/cgi-bin/configManager.cgi?action=setConfig&RecordMode[0].Mode=2 HTTP/1.1");
	if(!IsDahuaIpc[IpcIndex])
	{
		if(IsDigestAuth[IpcIndex])
		{
			if(RetryCnt==2)
			{
				if(HaveLogoutSession)
				{
					if(atoi(Session.sessionIDVersion)==2)
					sprintf(request,"PUT /ISAPI/Security/sessionLogout HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
\r\n\r\n",host_addr,host_addr,Session.sessionID);
					else
					sprintf(request,"PUT /ISAPI/Security/sessionLogout HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN\r\n\
Cookie: language=zh; WebSession=%s;\r\n\r\n\
\r\n\r\n",host_addr,host_addr,Session.sessionID);
				}
				else if(HaveUnlockSession)
				{
					if(atoi(Session.sessionIDVersion)==2)
					sprintf(request,"PUT /ISAPI/Security/illegalLoginLock HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 108\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
<IllegalLoginLock><enabled>false</enabled><maxIllegalLoginTimes>16</maxIllegalLoginTimes></IllegalLoginLock>\r\n\
\r\n\r\n",host_addr,host_addr,Session.sessionID);
					else
					sprintf(request,"PUT /ISAPI/Security/illegalLoginLock HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 99\r\n\
Cache-Control: max-age=0\r\n\
Accept: */*\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
Origin: http://%s\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN,zh;q=0.9\r\n\
Cookie: language=zh; WebSession=%s;\r\n\r\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?><IllegalLoginLock><enabled>false</enabled></IllegalLoginLock>\r\n\
\r\n\r\n",host_addr,host_addr,host_addr,Session.sessionID);
				}
				else if(HaveSession)
				{
					if(atoi(Session.sessionIDVersion)==2)
					sprintf(request,"GET %s\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Host: %s\r\n\
Content-Length: 0\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
\r\n\
\r\n\r\n",HaiKanUrl,host_addr,host_addr,Session.sessionID);
					else
					sprintf(request,"GET %s\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Host: %s\r\n\
Content-Length: 0\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
\r\n\
\r\n\r\n",HaiKanUrl,host_addr,host_addr,Session.sessionID);
				}
				else if(HaveLoginCap)
				{
					if(atoi(LoginCap.sessionIDVersion)==2)
					sprintf(request,"POST /ISAPI/Security/sessionLogin?timeStamp=%d HTTP/1.1\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/login.asp\r\n\
Accept-Language: zh-CN\r\n\
Accept-Encoding: gzip, deflate\r\n\
Host: %s\r\n\
Content-Length: 323\r\n\r\n\
<SessionLogin><userName>%s</userName><password>%s</password><sessionID>%s</sessionID><isSessionIDValidLongTerm>false</isSessionIDValidLongTerm><sessionIDVersion>2</sessionIDVersion></SessionLogin>\r\n\
\r\n\r\n",(unsigned)time(NULL),host_addr,host_addr,user,passwd,LoginCap.sessionID);
					else
					sprintf(request,"POST /ISAPI/Security/sessionLogin?timeStamp=%d HTTP/1.1\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/login.asp\r\n\
Accept-Language: zh-CN\r\n\
Accept-Encoding: gzip, deflate\r\n\
Host: %s\r\n\
Content-Length: 183\r\n\r\n\
<SessionLogin><userName>%s</userName><password>%s</password><sessionID>%s</sessionID></SessionLogin>\r\n\
\r\n\r\n",(unsigned)time(NULL),host_addr,host_addr,user,passwd,LoginCap.sessionID);
				}
				else
				{
					IsSecurityMode=TRUE;
					sprintf(request,"GET /ISAPI/Security/sessionLogin/capabilities?username=%s&random=%d HTTP/1.1\r\n\
Host: %s\r\n\
\r\n\r\n",user,(unsigned)time(NULL)%100000000,host_addr);
				}
			}
			else
			{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Authorization: Digest %s\r\n\
\r\n\r\n",HaiKanUrl,host_addr,DigestResponse);
			}
		}
		else
		{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Authorization: Basic %s\r\n\
\r\n\r\n",HaiKanUrl,host_addr,pBase64Buf);
		}
	}
	else
	{
		if(IsDigestAuth[IpcIndex])
		{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Cache-Control: max-age=0\r\n\
Authorization: Digest %s\r\n\
\r\n\r\n",DahuaUrl,host_addr,DigestResponse);
		}
		else
		{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Cache-Control: max-age=0\r\n\
Authorization: Basic %s\r\n\
\r\n\r\n",DahuaUrl,host_addr,pBase64Buf);
		}
	}
	send=0;
	totalsend=0; 
	nbytes=strlen(request);
	while(totalsend < nbytes)  
    {
		send=write(sockfd,request+totalsend,nbytes-totalsend); 
        if(send == -1)     
        {
			ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"httpclient send error(%s)\r\n",strerror(errno));
			BASE64_BUF_FREE(pBase64Buf);
			KillSocket(&sockfd);
			return FALSE;
        }
		totalsend += send;
    }
	memset(buffer,0,sizeof(buffer));
	bytesRead=getResponse(sockfd,buffer,readBufSize);
	if(bytesRead <= 0) 
	{
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		return FALSE;
	}
	else
	{
		ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_DEBUG,"recv:%s\r\n",buffer);
	}
	firstLine=buffer;
	nextLineStart=getLine(firstLine);
	if(sscanf(firstLine,"%*s%u",&responseCode) != 1) 
	{
		ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"no response code in firstLine\r\n");
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		return FALSE;
	}
	if(responseCode==200) 
	{
		if(IsSecurityMode)
		{
			for(;;)
			{
				unsigned int contentLength=0;
				
				lineStart=nextLineStart;
				if(lineStart==NULL || lineStart[0]=='\0')
					break;
				nextLineStart=getLine(lineStart);
				if(sscanf(lineStart,"Content-Length: %d",&contentLength)!=1 && sscanf(lineStart,"CONTENT-LENGTH: %d",&contentLength)!=1)
					continue;
				if(contentLength>0)
				{
ReadNext:
					lineStart=nextLineStart;
					if(lineStart==NULL || lineStart[0]=='\0')
						break;
					nextLineStart=getLine(lineStart);
					if(nextLineStart!=NULL)
					{
						BOOL ret=FALSE;
						if(contentLength>(unsigned int)(bytesRead-(nextLineStart-buffer)))
						{
							contentLength=bytesRead-(nextLineStart-buffer);
						}
						if(HaveLogoutSession)
						{
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"IPC Logout ok\r\n");
						}
						else if(HaveUnlockSession)
						{
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"IPC Unlock ok\r\n");
						}
						else if(HaveSession)
						{
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"IPC Setsnap ok\r\n");
							char					BasePathName[1024];
							char					FilePathName[1024];
							time_t 					rawtime;
							int						OldStatus;
							struct 					tm *timeinfo;
							static DWORD			CurrPicIndex=0;
							static time_t			LastPicTime=0;

							if(contentLength>(unsigned int)(bytesRead-(nextLineStart-buffer)))
							{
								contentLength=bytesRead-(nextLineStart-buffer);
								ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"try to truncation...\r\n");
							}
							
						}
						else if(HaveLoginCap)
						{
							ret=IpcSessionParse(nextLineStart,contentLength,&Session);
							if(ret)
							{
								if(atoi(Session.statusValue)==200)
									HaveSession=TRUE;
								else
									break;
								BASE64_BUF_FREE(pBase64Buf);
								KillSocket(&sockfd);
								goto SnapRetry;
							}
							else
								goto ReadNext;
						}
						else
						{
							ret=IpcLoginCapParse(nextLineStart,contentLength,&LoginCap);
						}
						
					}
				}
			}
		}
		else
		{
			for(;;)
			{
				unsigned int contentLength=0;
				
				lineStart=nextLineStart;
				if(lineStart==NULL || lineStart[0]=='\0')
					break;
				nextLineStart=getLine(lineStart);
				if(sscanf(lineStart,"Content-Length: %d",&contentLength)!=1 && sscanf(lineStart,"CONTENT-LENGTH: %d",&contentLength)!=1)
					continue;
				if(contentLength>0)
				{
					lineStart=nextLineStart;
					if(lineStart==NULL || lineStart[0]=='\0')
						break;
					nextLineStart=getLine(lineStart);
					if(nextLineStart!=NULL)
					{
						char					BasePathName[1024];
						char					FilePathName[1024];
						time_t 					rawtime;
						int						OldStatus;
						struct 					tm *timeinfo;
						static DWORD			CurrPicIndex=0;
						static time_t			LastPicTime=0;

						if(contentLength>(unsigned int)(bytesRead-(nextLineStart-buffer)))
						{
							contentLength=bytesRead-(nextLineStart-buffer);
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"try truncation2...\r\n");
						}
						
					}
				}
			}
		}
	}
	else if(responseCode==401)
	{
		if(HaveLogoutSession)
		{
			BASE64_BUF_FREE(pBase64Buf);
			KillSocket(&sockfd);
			return TRUE;
		}
		for(;;)
		{
			unsigned int contentLength=0;
			BOOL retDigest=FALSE;

			lineStart=nextLineStart;
			if(lineStart==NULL || lineStart[0]=='\0')
				break;
			if(strstr(lineStart,"WWW-Authenticate: Digest") != NULL)
				IsDigestAuth[IpcIndex]=TRUE;
			else
				IsDigestAuth[IpcIndex]=FALSE;
			retDigest=GreateDigestAuthResponse(lineStart,strlen(lineStart),IsDahuaIpc[IpcIndex]?DahuaUrl:HaiKanUrl,user,passwd,DigestResponse);
			if(!retDigest)
				IsDigestAuth[IpcIndex]=FALSE;
			RetryCnt++;
			BASE64_BUF_FREE(pBase64Buf);
			KillSocket(&sockfd);
			if(RetryCnt<=5)
			{
				goto SnapRetry;
			}
			IsDigestAuth[IpcIndex]=FALSE;
			return FALSE;
		}
	}
	else if(responseCode==400)
	{
		if(IsDahuaIpc[IpcIndex])
		{
			IsDahuaIpcN[IpcIndex]=!IsDahuaIpcN[IpcIndex];
		}
		RetryCnt++;
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		if(RetryCnt<=5)
		{
			goto SnapRetry;
		}
		return FALSE;
	}
	else
	{
		RetryCnt++;
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		if(RetryCnt<=1)
		{
			IsDahuaIpc[IpcIndex] = !IsDahuaIpc[IpcIndex];
			goto SnapRetry;
		}
		return FALSE;
	}
	BASE64_BUF_FREE(pBase64Buf);
	KillSocket(&sockfd);
	if(HaveSession&&!HaveLogoutSession)
	{
		HaveLogoutSession=TRUE;
		goto SnapRetry;
	}
	return TRUE;
err:
	BASE64_BUF_FREE(pBase64Buf);
	KillSocket(&sockfd);
	return FALSE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_IpcStopRecord
 *功能描述: 停止IPC录像
 *输入描述: FSUID,IPC标识
 *输出描述: 无
 *返回描述: 成功(TRUE)/失败(NULL)
 *作者日期: LJJ/2024/12/30
 *全局声明: 无
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_IpcStopRecord(DWORD IpcIndex)
{
	if(IpcIndex>=SP_SYS_IPC_NUM_MAX || !sFsuIpcIsInitReady)
		return FALSE;
	return IpcStopRecordEx(IpcIndex);
}

static BOOL IpcStartRecordEx(DWORD IpcIndex)
{
	static BOOL				IsFsuConfig[SP_SYS_IPC_NUM_MAX];
	static BOOL				IsDahuaIpc[SP_SYS_IPC_NUM_MAX];
	static BOOL				IsDahuaIpcN[SP_SYS_IPC_NUM_MAX];
	static BOOL				IsDigestAuth[SP_SYS_IPC_NUM_MAX];
    int sockfd 				=SOCKET_INVALID_FD;
    char buffer[FSU_IPC_PIC_FILE_SIZE_MAX]="";
    struct sockaddr_in   	server_addr; 
	struct sockaddr_in  	LocalAddr;
    struct hostent   		*host; 
    int 					portnumber=80;
    int 					nbytes=0; 
    char 					host_addr[32]="192.168.8.108"; 
	char 					user[32]="admin";
	char 					passwd[32]="123456abc";	
	char					HaiKanUrl[256];
	char					DahuaUrl[256];
    char 					request[2048]=""; 
    int 					send=0;
    int 					totalsend=0;
	struct in_addr 			tmpAddr;
	unsigned int 			readBufSize=sizeof(buffer)-1;
	int 					bytesRead;
	char* 					firstLine;
	char* 					lineStart;
	char* 					nextLineStart=NULL;
	unsigned 				responseCode;
	char					KeyAuth[256];
	int 					CmdLen=0;
	char*					pBase64Buf=NULL;
	char					FsuNameStr[1024];
	int						FsuNameLen=1024;
	int						RetryCnt=0;
	BOOL					IsSecurityMode=FALSE;
	BOOL					HaveLoginCap=FALSE;
	BOOL					HaveSession=FALSE;
	BOOL					HaveLogoutSession=FALSE;
	BOOL					HaveUnlockSession=FALSE;
	IPC_LOGIN_CAP			LoginCap;
	IPC_SESSION				Session;
	FILE					*pFile=NULL;
	char					DigestResponse[512];
	char					SecurityPassWord[128];
	
	if(IpcIndex>=SP_SYS_IPC_NUM_MAX)
		return FALSE;
	printf("-------IpcStartRecordEx-----\r\n");
    memset(&LocalAddr,0,sizeof(LocalAddr));
    bzero(&server_addr,sizeof(server_addr));
	memset(HaiKanUrl,0,sizeof(HaiKanUrl));
	memset(DahuaUrl,0,sizeof(DahuaUrl));
	memset(host_addr,0,sizeof(host_addr));
	strncpy(host_addr,sIpcServer.m_strIpcServerIP,sizeof(host_addr)-1);
	portnumber=sIpcServer.m_IpcServerPort;
	memset(user,0,sizeof(user));
	strncpy(user,sIpcServer.m_strLoginUser,sizeof(user)-1);
	memset(passwd,0,sizeof(passwd));
	strncpy(passwd,sIpcServer.m_strLoginPwd,sizeof(passwd)-1);
	
SnapRetry:
	server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=inet_addr(host_addr);
    server_addr.sin_port=htons(portnumber);
 	ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"try to connect IPC(%s).....\r\n",host_addr);
	sockfd=CreateSocket(SOCKET_TYPE_TCP,SOCKET_TCP_MODE_CLIENT,&LocalAddr,&server_addr,INVALID_DEV_NET_ID);
	if(sockfd==SOCKET_INVALID_FD)
		return FALSE;

	sprintf(KeyAuth,"%s:%s",user,passwd);
	CmdLen=strlen(KeyAuth);
	pBase64Buf=Base64Encode(KeyAuth,&CmdLen);
	if(pBase64Buf==NULL)
	{
		KillSocket(&sockfd);
		return FALSE;
	}
	IsDahuaIpc[IpcIndex]=TRUE;
	if(!IsDahuaIpc[IpcIndex])
		sprintf(HaiKanUrl,"/cgi-bin/configManager.cgi?action=setConfig&RecordMode[0].Mode=1 HTTP/1.1");
	else
		sprintf(DahuaUrl,"/cgi-bin/configManager.cgi?action=setConfig&RecordMode[0].Mode=1 HTTP/1.1");
	if(!IsDahuaIpc[IpcIndex])
	{
		if(IsDigestAuth[IpcIndex])
		{
			if(RetryCnt==2)
			{
				if(HaveLogoutSession)
				{
					if(atoi(Session.sessionIDVersion)==2)
					sprintf(request,"PUT /ISAPI/Security/sessionLogout HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
\r\n\r\n",host_addr,host_addr,Session.sessionID);
					else
					sprintf(request,"PUT /ISAPI/Security/sessionLogout HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN\r\n\
Cookie: language=zh; WebSession=%s;\r\n\r\n\
\r\n\r\n",host_addr,host_addr,Session.sessionID);
				}
				else if(HaveUnlockSession)
				{
					if(atoi(Session.sessionIDVersion)==2)
					sprintf(request,"PUT /ISAPI/Security/illegalLoginLock HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 108\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
<IllegalLoginLock><enabled>false</enabled><maxIllegalLoginTimes>16</maxIllegalLoginTimes></IllegalLoginLock>\r\n\
\r\n\r\n",host_addr,host_addr,Session.sessionID);
					else
					sprintf(request,"PUT /ISAPI/Security/illegalLoginLock HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 99\r\n\
Cache-Control: max-age=0\r\n\
Accept: */*\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
Origin: http://%s\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN,zh;q=0.9\r\n\
Cookie: language=zh; WebSession=%s;\r\n\r\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?><IllegalLoginLock><enabled>false</enabled></IllegalLoginLock>\r\n\
\r\n\r\n",host_addr,host_addr,host_addr,Session.sessionID);
				}
				else if(HaveSession)
				{
					if(atoi(Session.sessionIDVersion)==2)
					sprintf(request,"GET %s\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Host: %s\r\n\
Content-Length: 0\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
\r\n\
\r\n\r\n",HaiKanUrl,host_addr,host_addr,Session.sessionID);
					else
					sprintf(request,"GET %s\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Host: %s\r\n\
Content-Length: 0\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
\r\n\
\r\n\r\n",HaiKanUrl,host_addr,host_addr,Session.sessionID);
				}
				else if(HaveLoginCap)
				{
					if(atoi(LoginCap.sessionIDVersion)==2)
					sprintf(request,"POST /ISAPI/Security/sessionLogin?timeStamp=%d HTTP/1.1\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/login.asp\r\n\
Accept-Language: zh-CN\r\n\
Accept-Encoding: gzip, deflate\r\n\
Host: %s\r\n\
Content-Length: 323\r\n\r\n\
<SessionLogin><userName>%s</userName><password>%s</password><sessionID>%s</sessionID><isSessionIDValidLongTerm>false</isSessionIDValidLongTerm><sessionIDVersion>2</sessionIDVersion></SessionLogin>\r\n\
\r\n\r\n",(unsigned)time(NULL),host_addr,host_addr,user,passwd,LoginCap.sessionID);
					else
					sprintf(request,"POST /ISAPI/Security/sessionLogin?timeStamp=%d HTTP/1.1\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/login.asp\r\n\
Accept-Language: zh-CN\r\n\
Accept-Encoding: gzip, deflate\r\n\
Host: %s\r\n\
Content-Length: 183\r\n\r\n\
<SessionLogin><userName>%s</userName><password>%s</password><sessionID>%s</sessionID></SessionLogin>\r\n\
\r\n\r\n",(unsigned)time(NULL),host_addr,host_addr,user,passwd,LoginCap.sessionID);
				}
				else
				{
					IsSecurityMode=TRUE;
					sprintf(request,"GET /ISAPI/Security/sessionLogin/capabilities?username=%s&random=%d HTTP/1.1\r\n\
Host: %s\r\n\
\r\n\r\n",user,(unsigned)time(NULL)%100000000,host_addr);
				}
			}
			else
			{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Authorization: Digest %s\r\n\
\r\n\r\n",HaiKanUrl,host_addr,DigestResponse);
			}
		}
		else
		{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Authorization: Basic %s\r\n\
\r\n\r\n",HaiKanUrl,host_addr,pBase64Buf);
		}
	}
	else
	{
		if(IsDigestAuth[IpcIndex])
		{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Cache-Control: max-age=0\r\n\
Authorization: Digest %s\r\n\
\r\n\r\n",DahuaUrl,host_addr,DigestResponse);
		}
		else
		{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Cache-Control: max-age=0\r\n\
Authorization: Basic %s\r\n\
\r\n\r\n",DahuaUrl,host_addr,pBase64Buf);
		}
	}
	send=0;
	totalsend=0; 
	nbytes=strlen(request);
	while(totalsend < nbytes)  
    {
		send=write(sockfd,request+totalsend,nbytes-totalsend); 
        if(send == -1)     
        {
			ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"httpclient send error(%s)\r\n",strerror(errno));
			BASE64_BUF_FREE(pBase64Buf);
			KillSocket(&sockfd);
			return FALSE;
        }
		totalsend += send;
    }
	memset(buffer,0,sizeof(buffer));
	bytesRead=getResponse(sockfd,buffer,readBufSize);
	if(bytesRead <= 0) 
	{
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		return FALSE;
	}
	else
	{
		ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_DEBUG,"recv:%s\r\n",buffer);
	}
	firstLine=buffer;
	nextLineStart=getLine(firstLine);
	if(sscanf(firstLine,"%*s%u",&responseCode) != 1) 
	{
		ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"no response code in firstLine\r\n");
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		return FALSE;
	}
	if(responseCode==200) 
	{
		if(IsSecurityMode)
		{
			for(;;)
			{
				unsigned int contentLength=0;
				
				lineStart=nextLineStart;
				if(lineStart==NULL || lineStart[0]=='\0')
					break;
				nextLineStart=getLine(lineStart);
				if(sscanf(lineStart,"Content-Length: %d",&contentLength)!=1 && sscanf(lineStart,"CONTENT-LENGTH: %d",&contentLength)!=1)
					continue;
				if(contentLength>0)
				{
ReadNext:
					lineStart=nextLineStart;
					if(lineStart==NULL || lineStart[0]=='\0')
						break;
					nextLineStart=getLine(lineStart);
					if(nextLineStart!=NULL)
					{
						BOOL ret=FALSE;
						if(contentLength>(unsigned int)(bytesRead-(nextLineStart-buffer)))
						{
							contentLength=bytesRead-(nextLineStart-buffer);
						}
						if(HaveLogoutSession)
						{
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"IPC Logout ok\r\n");
						}
						else if(HaveSession)
						{
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"IPC Setsnap ok\r\n");
							char					BasePathName[1024];
							char					FilePathName[1024];
							time_t 					rawtime;
							int						OldStatus;
							struct 					tm *timeinfo;
							static DWORD			CurrPicIndex=0;
							static time_t			LastPicTime=0;

							if(contentLength>(unsigned int)(bytesRead-(nextLineStart-buffer)))
							{
								contentLength=bytesRead-(nextLineStart-buffer);
								ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"try to truncation...\r\n");
							}
							
						}
						else if(HaveLoginCap)
						{
							ret=IpcSessionParse(nextLineStart,contentLength,&Session);
							if(ret)
							{
								if(atoi(Session.statusValue)==200)
									HaveSession=TRUE;
								else
									break;
								BASE64_BUF_FREE(pBase64Buf);
								KillSocket(&sockfd);
								goto SnapRetry;
							}
							else
								goto ReadNext;
						}
						else
						{
							ret=IpcLoginCapParse(nextLineStart,contentLength,&LoginCap);
						}
						
					}
				}
			}
		}
		else
		{
			for(;;)
			{
				unsigned int contentLength=0;
				
				lineStart=nextLineStart;
				if(lineStart==NULL || lineStart[0]=='\0')
					break;
				nextLineStart=getLine(lineStart);
				if(sscanf(lineStart,"Content-Length: %d",&contentLength)!=1 && sscanf(lineStart,"CONTENT-LENGTH: %d",&contentLength)!=1)
					continue;
				if(contentLength>0)
				{
					lineStart=nextLineStart;
					if(lineStart==NULL || lineStart[0]=='\0')
						break;
					nextLineStart=getLine(lineStart);
					if(nextLineStart!=NULL)
					{
						char					BasePathName[1024];
						char					FilePathName[1024];
						time_t 					rawtime;
						int						OldStatus;
						struct 					tm *timeinfo;
						static DWORD			CurrPicIndex=0;
						static time_t			LastPicTime=0;

						if(contentLength>(unsigned int)(bytesRead-(nextLineStart-buffer)))
						{
							contentLength=bytesRead-(nextLineStart-buffer);
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"try truncation2...\r\n");
						}
						
					}
				}
			}
		}
	}
	else if(responseCode==401)
	{
		if(HaveLogoutSession)
		{
			BASE64_BUF_FREE(pBase64Buf);
			KillSocket(&sockfd);
			return TRUE;
		}
		for(;;)
		{
			unsigned int contentLength=0;
			BOOL retDigest=FALSE;

			lineStart=nextLineStart;
			if(lineStart==NULL || lineStart[0]=='\0')
				break;
			if(strstr(lineStart,"WWW-Authenticate: Digest") != NULL)
				IsDigestAuth[IpcIndex]=TRUE;
			else
				IsDigestAuth[IpcIndex]=FALSE;
			retDigest=GreateDigestAuthResponse(lineStart,strlen(lineStart),IsDahuaIpc[IpcIndex]?DahuaUrl:HaiKanUrl,user,passwd,DigestResponse);
			if(!retDigest)
				IsDigestAuth[IpcIndex]=FALSE;
			RetryCnt++;
			BASE64_BUF_FREE(pBase64Buf);
			KillSocket(&sockfd);
			if(RetryCnt<=5)
			{
				goto SnapRetry;
			}
			IsDigestAuth[IpcIndex]=FALSE;
			return FALSE;
		}
	}
	else if(responseCode==400)
	{
		if(IsDahuaIpc[IpcIndex])
		{
			IsDahuaIpcN[IpcIndex]=!IsDahuaIpcN[IpcIndex];
		}
		RetryCnt++;
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		if(RetryCnt<=5)
		{
			goto SnapRetry;
		}
		return FALSE;
	}
	else
	{
		RetryCnt++;
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		if(RetryCnt<=1)
		{
			IsDahuaIpc[IpcIndex] = !IsDahuaIpc[IpcIndex];
			goto SnapRetry;
		}
		return FALSE;
	}
	BASE64_BUF_FREE(pBase64Buf);
	KillSocket(&sockfd);
	if(HaveSession&&!HaveLogoutSession)
	{
		HaveLogoutSession=TRUE;
		goto SnapRetry;
	}
	return TRUE;
err:
	BASE64_BUF_FREE(pBase64Buf);
	KillSocket(&sockfd);
	return FALSE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_IpcStartRecord
 *功能描述: 开始IPC录像
 *输入描述: FSUID,IPC标识
 *输出描述: 无
 *返回描述: 成功(TRUE)/失败(NULL)
 *作者日期: LJJ/2024/12/30
 *全局声明: 无
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_IpcStartRecord(DWORD IpcIndex)
{
	if(IpcIndex>=SP_SYS_IPC_NUM_MAX || !sFsuIpcIsInitReady)
		return FALSE;
	return IpcStartRecordEx(IpcIndex);
}

static BOOL IpcGetTimeEx(DWORD IpcIndex)
{
	static BOOL				IsFsuConfig[SP_SYS_IPC_NUM_MAX];
	static BOOL				IsDahuaIpc[SP_SYS_IPC_NUM_MAX];
	static BOOL				IsDahuaIpcN[SP_SYS_IPC_NUM_MAX];
	static BOOL				IsDigestAuth[SP_SYS_IPC_NUM_MAX];
    int sockfd 				=SOCKET_INVALID_FD;
    char buffer[FSU_IPC_PIC_FILE_SIZE_MAX]="";
    struct sockaddr_in   	server_addr; 
	struct sockaddr_in  	LocalAddr;
    struct hostent   		*host; 
    int 					portnumber=80;
    int 					nbytes=0; 
    char 					host_addr[32]="192.168.8.108"; 
	char 					user[32]="admin";
	char 					passwd[32]="123456abc";	
	char					HaiKanUrl[256];
	char					DahuaUrl[256];
    char 					request[2048]=""; 
    int 					send=0;
    int 					totalsend=0;
	struct in_addr 			tmpAddr;
	unsigned int 			readBufSize=sizeof(buffer)-1;
	int 					bytesRead;
	char* 					firstLine;
	char* 					lineStart;
	char* 					nextLineStart=NULL;
	unsigned 				responseCode;
	char					KeyAuth[256];
	int 					CmdLen=0;
	char*					pBase64Buf=NULL;
	char					FsuNameStr[1024];
	int						FsuNameLen=1024;
	int						RetryCnt=0;
	BOOL					IsSecurityMode=FALSE;
	BOOL					HaveLoginCap=FALSE;
	BOOL					HaveSession=FALSE;
	BOOL					HaveLogoutSession=FALSE;
	BOOL					HaveUnlockSession=FALSE;
	IPC_LOGIN_CAP			LoginCap;
	IPC_SESSION				Session;
	FILE					*pFile=NULL;
	char					DigestResponse[512];
	char					SecurityPassWord[128];
	
	if(IpcIndex>=SP_SYS_IPC_NUM_MAX)
		return FALSE;
	printf("-------IpcGetTimeEx-----\r\n");
    memset(&LocalAddr,0,sizeof(LocalAddr));
    bzero(&server_addr,sizeof(server_addr));
	memset(HaiKanUrl,0,sizeof(HaiKanUrl));
	memset(DahuaUrl,0,sizeof(DahuaUrl));
	memset(host_addr,0,sizeof(host_addr));
	strncpy(host_addr,sIpcServer.m_strIpcServerIP,sizeof(host_addr)-1);
	portnumber=sIpcServer.m_IpcServerPort;
	memset(user,0,sizeof(user));
	strncpy(user,sIpcServer.m_strLoginUser,sizeof(user)-1);
	memset(passwd,0,sizeof(passwd));
	strncpy(passwd,sIpcServer.m_strLoginPwd,sizeof(passwd)-1);
	
SnapRetry:
	server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=inet_addr(host_addr);
    server_addr.sin_port=htons(portnumber);
 	ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"try to connect IPC(%s).....\r\n",host_addr);
	sockfd=CreateSocket(SOCKET_TYPE_TCP,SOCKET_TCP_MODE_CLIENT,&LocalAddr,&server_addr,INVALID_DEV_NET_ID);
	if(sockfd==SOCKET_INVALID_FD)
		return FALSE;
	
	sprintf(KeyAuth,"%s:%s",user,passwd);
	CmdLen=strlen(KeyAuth);
	pBase64Buf=Base64Encode(KeyAuth,&CmdLen);
	if(pBase64Buf==NULL)
	{
		KillSocket(&sockfd);
		return FALSE;
	}
	IsDahuaIpc[IpcIndex]=TRUE;
	if(!IsDahuaIpc[IpcIndex])
		sprintf(HaiKanUrl,"/cgi-bin/global.cgi?action=getCurrentTime HTTP/1.1");
	else
		sprintf(DahuaUrl,"/cgi-bin/global.cgi?action=getCurrentTime HTTP/1.1");
	if(!IsDahuaIpc[IpcIndex])
	{
		if(IsDigestAuth[IpcIndex])
		{
			if(RetryCnt==2)
			{
				if(HaveLogoutSession)
				{
					if(atoi(Session.sessionIDVersion)==2)
					sprintf(request,"PUT /ISAPI/Security/sessionLogout HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
\r\n\r\n",host_addr,host_addr,Session.sessionID);
					else
					sprintf(request,"PUT /ISAPI/Security/sessionLogout HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN\r\n\
Cookie: language=zh; WebSession=%s;\r\n\r\n\
\r\n\r\n",host_addr,host_addr,Session.sessionID);
				}
				else if(HaveUnlockSession)
				{
					if(atoi(Session.sessionIDVersion)==2)
					sprintf(request,"PUT /ISAPI/Security/illegalLoginLock HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 108\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
<IllegalLoginLock><enabled>false</enabled><maxIllegalLoginTimes>16</maxIllegalLoginTimes></IllegalLoginLock>\r\n\
\r\n\r\n",host_addr,host_addr,Session.sessionID);
					else
					sprintf(request,"PUT /ISAPI/Security/illegalLoginLock HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 99\r\n\
Cache-Control: max-age=0\r\n\
Accept: */*\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
Origin: http://%s\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN,zh;q=0.9\r\n\
Cookie: language=zh; WebSession=%s;\r\n\r\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?><IllegalLoginLock><enabled>false</enabled></IllegalLoginLock>\r\n\
\r\n\r\n",host_addr,host_addr,host_addr,Session.sessionID);
				}
				else if(HaveSession)
				{
					if(atoi(Session.sessionIDVersion)==2)
					sprintf(request,"GET %s\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Host: %s\r\n\
Content-Length: 0\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
\r\n\
\r\n\r\n",HaiKanUrl,host_addr,host_addr,Session.sessionID);
					else
					sprintf(request,"GET %s\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Host: %s\r\n\
Content-Length: 0\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
\r\n\
\r\n\r\n",HaiKanUrl,host_addr,host_addr,Session.sessionID);
				}
				else if(HaveLoginCap)
				{
					if(atoi(LoginCap.sessionIDVersion)==2)
					sprintf(request,"POST /ISAPI/Security/sessionLogin?timeStamp=%d HTTP/1.1\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/login.asp\r\n\
Accept-Language: zh-CN\r\n\
Accept-Encoding: gzip, deflate\r\n\
Host: %s\r\n\
Content-Length: 323\r\n\r\n\
<SessionLogin><userName>%s</userName><password>%s</password><sessionID>%s</sessionID><isSessionIDValidLongTerm>false</isSessionIDValidLongTerm><sessionIDVersion>2</sessionIDVersion></SessionLogin>\r\n\
\r\n\r\n",(unsigned)time(NULL),host_addr,host_addr,user,passwd,LoginCap.sessionID);
					else
					sprintf(request,"POST /ISAPI/Security/sessionLogin?timeStamp=%d HTTP/1.1\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/login.asp\r\n\
Accept-Language: zh-CN\r\n\
Accept-Encoding: gzip, deflate\r\n\
Host: %s\r\n\
Content-Length: 183\r\n\r\n\
<SessionLogin><userName>%s</userName><password>%s</password><sessionID>%s</sessionID></SessionLogin>\r\n\
\r\n\r\n",(unsigned)time(NULL),host_addr,host_addr,user,passwd,LoginCap.sessionID);
				}
				else
				{
					IsSecurityMode=TRUE;
					sprintf(request,"GET /ISAPI/Security/sessionLogin/capabilities?username=%s&random=%d HTTP/1.1\r\n\
Host: %s\r\n\
\r\n\r\n",user,(unsigned)time(NULL)%100000000,host_addr);
				}
			}
			else
			{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Authorization: Digest %s\r\n\
\r\n\r\n",HaiKanUrl,host_addr,DigestResponse);
			}
		}
		else
		{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Authorization: Basic %s\r\n\
\r\n\r\n",HaiKanUrl,host_addr,pBase64Buf);
		}
	}
	else
	{
		if(IsDigestAuth[IpcIndex])
		{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Cache-Control: max-age=0\r\n\
Authorization: Digest %s\r\n\
\r\n\r\n",DahuaUrl,host_addr,DigestResponse);
		}
		else
		{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Cache-Control: max-age=0\r\n\
Authorization: Basic %s\r\n\
\r\n\r\n",DahuaUrl,host_addr,pBase64Buf);
		}
	}
	send=0;
	totalsend=0; 
	nbytes=strlen(request);
	while(totalsend < nbytes)  
    {
		send=write(sockfd,request+totalsend,nbytes-totalsend); 
        if(send == -1)     
        {
			ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"httpclient send error(%s)\r\n",strerror(errno));
			BASE64_BUF_FREE(pBase64Buf);
			KillSocket(&sockfd);
			return FALSE;
        }
		totalsend += send;
    }
	memset(buffer,0,sizeof(buffer));
	bytesRead=getResponse(sockfd,buffer,readBufSize);
	if(bytesRead <= 0) 
	{
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		return FALSE;
	}
	else
	{
		ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_DEBUG,"recv:%s\r\n",buffer);
	}
	firstLine=buffer;
	nextLineStart=getLine(firstLine);
	if(sscanf(firstLine,"%*s%u",&responseCode) != 1) 
	{
		ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"no response code in firstLine\r\n");
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		return FALSE;
	}
	if(responseCode==200) 
	{
		if(IsSecurityMode)
		{
			for(;;)
			{
				unsigned int contentLength=0;
				
				lineStart=nextLineStart;
				if(lineStart==NULL || lineStart[0]=='\0')
					break;
				nextLineStart=getLine(lineStart);
				if(sscanf(lineStart,"Content-Length: %d",&contentLength)!=1 && sscanf(lineStart,"CONTENT-LENGTH: %d",&contentLength)!=1)
					continue;
				if(contentLength>0)
				{
ReadNext:
					lineStart=nextLineStart;
					if(lineStart==NULL || lineStart[0]=='\0')
						break;
					nextLineStart=getLine(lineStart);
					if(nextLineStart!=NULL)
					{
						BOOL ret=FALSE;
						if(contentLength>(unsigned int)(bytesRead-(nextLineStart-buffer)))
						{
							contentLength=bytesRead-(nextLineStart-buffer);
						}
						if(HaveLogoutSession)
						{
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"IPC Logout ok\r\n");
						}
						else if(HaveUnlockSession)
						{
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"IPC Unlock ok\r\n");
						}
						else if(HaveSession)
						{
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"IPC Setsnap ok\r\n");
							char					BasePathName[1024];
							char					FilePathName[1024];
							time_t 					rawtime;
							int						OldStatus;
							struct 					tm *timeinfo;
							static DWORD			CurrPicIndex=0;
							static time_t			LastPicTime=0;

							if(contentLength>(unsigned int)(bytesRead-(nextLineStart-buffer)))
							{
								contentLength=bytesRead-(nextLineStart-buffer);
								ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"try to truncation...\r\n");
							}
							
						}
						else if(HaveLoginCap)
						{
							ret=IpcSessionParse(nextLineStart,contentLength,&Session);
							if(ret)
							{
								if(atoi(Session.statusValue)==200)
									HaveSession=TRUE;
								else
									break;
								BASE64_BUF_FREE(pBase64Buf);
								KillSocket(&sockfd);
								goto SnapRetry;
							}
							else
								goto ReadNext;
						}
						else
						{
							ret=IpcLoginCapParse(nextLineStart,contentLength,&LoginCap);
						}
						
					}
				}
			}
		}
		else
		{
			for(;;)
			{
				unsigned int contentLength=0;
				
				lineStart=nextLineStart;
				if(lineStart==NULL || lineStart[0]=='\0')
					break;
				nextLineStart=getLine(lineStart);
				if(sscanf(lineStart,"Content-Length: %d",&contentLength)!=1 && sscanf(lineStart,"CONTENT-LENGTH: %d",&contentLength)!=1)
					continue;
				if(contentLength>0)
				{
					lineStart=nextLineStart;
					if(lineStart==NULL || lineStart[0]=='\0')
						break;
					nextLineStart=getLine(lineStart);
					if(nextLineStart!=NULL)
					{
						char					BasePathName[1024];
						char					FilePathName[1024];
						time_t 					rawtime;
						int						OldStatus,ret;
						struct 					tm *timeinfo;
						static DWORD			CurrPicIndex=0;
						static time_t			LastPicTime=0;
						struct tm tmpTm;
						time_t tmpTime_t=0;
						char szTime[64]={0};
						char szTime2[64]={0};
						char szTime3[64]={0};

						if(contentLength>(unsigned int)(bytesRead-(nextLineStart-buffer)))
						{
							contentLength=bytesRead-(nextLineStart-buffer);
							ZFY_LES_LogPrintf("FSU_IPC",LOG_EVENT_LEVEL_NOTICE,"try truncation2...\r\n");
						}
						PTHREAD_MUTEX_SAFE_LOCK(sPicFileOpMutex,OldStatus);
						time(&rawtime);
						timeinfo=localtime(&rawtime);
						ret=sscanf(nextLineStart+7,"%19s %15s",szTime,szTime2);
						if(ret)
						{
							sprintf(szTime3,"%s_%s",szTime,szTime2);
							strptime(szTime3,"%Y-%m-%d_%H:%M:%S",&tmpTm);
							tmpTime_t=mktime(&tmpTm);
							tmpTime_t-=8*3600;
							stime(&tmpTime_t);
						}
						PTHREAD_MUTEX_SAFE_UNLOCK(sPicFileOpMutex,OldStatus);
						
					}
				}
			}
		}
	}
	else if(responseCode==401)
	{
		if(HaveLogoutSession)
		{
			BASE64_BUF_FREE(pBase64Buf);
			KillSocket(&sockfd);
			return TRUE;
		}
		for(;;)
		{
			unsigned int contentLength=0;
			BOOL retDigest=FALSE;

			lineStart=nextLineStart;
			if(lineStart==NULL || lineStart[0]=='\0')
				break;
			if(strstr(lineStart,"WWW-Authenticate: Digest") != NULL)
				IsDigestAuth[IpcIndex]=TRUE;
			else
				IsDigestAuth[IpcIndex]=FALSE;
			retDigest=GreateDigestAuthResponse(lineStart,strlen(lineStart),IsDahuaIpc[IpcIndex]?DahuaUrl:HaiKanUrl,user,passwd,DigestResponse);
			if(!retDigest)
				IsDigestAuth[IpcIndex]=FALSE;
			RetryCnt++;
			BASE64_BUF_FREE(pBase64Buf);
			KillSocket(&sockfd);
			if(RetryCnt<=5)
			{
				goto SnapRetry;
			}
			IsDigestAuth[IpcIndex]=FALSE;
			return FALSE;
		}
	}
	else if(responseCode==400)
	{
		if(IsDahuaIpc[IpcIndex])
		{
			IsDahuaIpcN[IpcIndex]=!IsDahuaIpcN[IpcIndex];
		}
		RetryCnt++;
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		if(RetryCnt<=5)
		{
			goto SnapRetry;
		}
		return FALSE;
	}
	else
	{
		RetryCnt++;
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		if(RetryCnt<=1)
		{
			IsDahuaIpc[IpcIndex] = !IsDahuaIpc[IpcIndex];
			goto SnapRetry;
		}
		return FALSE;
	}
	BASE64_BUF_FREE(pBase64Buf);
	KillSocket(&sockfd);
	if(HaveSession&&!HaveLogoutSession)
	{
		HaveLogoutSession=TRUE;
		goto SnapRetry;
	}
	return TRUE;
err:
	BASE64_BUF_FREE(pBase64Buf);
	KillSocket(&sockfd);
	return FALSE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_IpcGetTime
 *功能描述: 获取IPC时间
 *输入描述: IPC标识
 *输出描述: 无
 *返回描述: 成功(TRUE)/失败(NULL)
 *作者日期: LJJ/2024/12/30
 *全局声明: 无
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_IpcGetTime(DWORD IpcIndex)
{
	if(IpcIndex>=SP_SYS_IPC_NUM_MAX || !sFsuIpcIsInitReady)
		return FALSE;
	return IpcGetTimeEx(IpcIndex);
}

static BOOL IpcSetSnapPicEx(char *FsuID,DWORD IpcIndex,BOOL IsSubStream,char *pSnapFileName,DWORD AccountIndex) 
{
	static BOOL				IsFsuConfig[SP_SYS_IPC_NUM_MAX];
	static BOOL				IsDahuaIpc[SP_SYS_IPC_NUM_MAX];
	static BOOL				IsDahuaIpcN[SP_SYS_IPC_NUM_MAX];
	static BOOL				IsDigestAuth[SP_SYS_IPC_NUM_MAX];
    int sockfd 				=SOCKET_INVALID_FD;
    char buffer[FSU_IPC_PIC_FILE_SIZE_MAX]="";
    struct sockaddr_in   	server_addr; 
	struct sockaddr_in  	LocalAddr;
    struct hostent   		*host; 
    int 					portnumber=80;
    int 					nbytes=0; 
    char 					host_addr[32]="192.168.8.108"; 
	char 					user[32]="admin";
	char 					passwd[32]="123456abc";	
	char					HaiKanUrl[256];
	char					DahuaUrl[256];
    char 					request[2048]=""; 
    int 					send=0;
    int 					totalsend=0;
	struct in_addr 			tmpAddr;
	unsigned int 			readBufSize=sizeof(buffer)-1;
	int 					bytesRead;
	char* 					firstLine;
	char* 					lineStart;
	char* 					nextLineStart=NULL;
	unsigned 				responseCode;
	char					KeyAuth[256];
	int 					CmdLen=0;
	char*					pBase64Buf=NULL;
	char					FsuNameStr[1024];
	int						FsuNameLen=1024;
	int						RetryCnt=0;
	BOOL					IsSecurityMode=FALSE;
	BOOL					HaveLoginCap=FALSE;
	BOOL					HaveSession=FALSE;
	BOOL					HaveLogoutSession=FALSE;
	BOOL					HaveUnlockSession=FALSE;
	IPC_LOGIN_CAP			LoginCap;
	IPC_SESSION				Session;
	FILE					*pFile=NULL;
	char					DigestResponse[512];
	char					SecurityPassWord[128];
	
	if(IpcIndex>=SP_SYS_IPC_NUM_MAX)
		return FALSE;
	printf("-------IpcSetSnapPicEx-----\r\n");
    memset(&LocalAddr,0,sizeof(LocalAddr));
    bzero(&server_addr,sizeof(server_addr));
	memset(HaiKanUrl,0,sizeof(HaiKanUrl));
	memset(DahuaUrl,0,sizeof(DahuaUrl));
	memset(host_addr,0,sizeof(host_addr));
	strncpy(host_addr,sIpcServer.m_strIpcServerIP,sizeof(host_addr)-1);
	portnumber=sIpcServer.m_IpcServerPort;
	memset(user,0,sizeof(user));
	strncpy(user,sIpcServer.m_strLoginUser,sizeof(user)-1);
	memset(passwd,0,sizeof(passwd));
	strncpy(passwd,sIpcServer.m_strLoginPwd,sizeof(passwd)-1);
	
SnapRetry:
	server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=inet_addr(host_addr);
    server_addr.sin_port=htons(portnumber);
 	ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"try to connect IPC(%s).....\r\n",host_addr);
	sockfd=CreateSocket(SOCKET_TYPE_TCP,SOCKET_TCP_MODE_CLIENT,&LocalAddr,&server_addr,INVALID_DEV_NET_ID);
	if(sockfd==SOCKET_INVALID_FD)
		return FALSE;
	
	sprintf(KeyAuth,"%s:%s",user,passwd);
	CmdLen=strlen(KeyAuth);
	pBase64Buf=Base64Encode(KeyAuth,&CmdLen);
	if(pBase64Buf==NULL)
	{
		KillSocket(&sockfd);
		return FALSE;
	}
	IsDahuaIpc[IpcIndex]=TRUE;
	if(!IsDahuaIpcN[IpcIndex])
		sprintf(DahuaUrl,"/cgi-bin/snapshot.cgi?channel=0 HTTP/1.1");
	else
		sprintf(DahuaUrl,"/cgi-bin/snapshot.cgi?channel=1 HTTP/1.1");
	if(IsSubStream)
		sprintf(HaiKanUrl,"/ISAPI/Streaming/channels/102/picture?snapShotImageType=JPEG HTTP/1.1");
	else
		sprintf(HaiKanUrl,"/ISAPI/Streaming/channels/101/picture?snapShotImageType=JPEG HTTP/1.1");
	if(!IsDahuaIpc[IpcIndex])
	{
		if(IsDigestAuth[IpcIndex])
		{
			if(RetryCnt==2)
			{
				if(HaveLogoutSession)
				{
					if(atoi(Session.sessionIDVersion)==2)
					sprintf(request,"PUT /ISAPI/Security/sessionLogout HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
\r\n\r\n",host_addr,host_addr,Session.sessionID);
					else
					sprintf(request,"PUT /ISAPI/Security/sessionLogout HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN\r\n\
Cookie: language=zh; WebSession=%s;\r\n\r\n\
\r\n\r\n",host_addr,host_addr,Session.sessionID);
				}
				else if(HaveUnlockSession)
				{
					if(atoi(Session.sessionIDVersion)==2)
					sprintf(request,"PUT /ISAPI/Security/illegalLoginLock HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 108\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
<IllegalLoginLock><enabled>false</enabled><maxIllegalLoginTimes>16</maxIllegalLoginTimes></IllegalLoginLock>\r\n\
\r\n\r\n",host_addr,host_addr,Session.sessionID);
					else
					sprintf(request,"PUT /ISAPI/Security/illegalLoginLock HTTP/1.1\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: 99\r\n\
Cache-Control: max-age=0\r\n\
Accept: */*\r\n\
X-Requested-With: XMLHttpRequest\r\n\
If-Modified-Since: 0\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
Origin: http://%s\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Accept-Encoding: gzip, deflate\r\n\
Accept-Language: zh-CN,zh;q=0.9\r\n\
Cookie: language=zh; WebSession=%s;\r\n\r\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?><IllegalLoginLock><enabled>false</enabled></IllegalLoginLock>\r\n\
\r\n\r\n",host_addr,host_addr,host_addr,Session.sessionID);
				}
				else if(HaveSession)
				{
					if(atoi(Session.sessionIDVersion)==2)
					sprintf(request,"GET %s\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Host: %s\r\n\
Content-Length: 0\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
\r\n\
\r\n\r\n",HaiKanUrl,host_addr,host_addr,Session.sessionID);
					else
					sprintf(request,"GET %s\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/config.asp\r\n\
Host: %s\r\n\
Content-Length: 0\r\n\
Cookie: language=zh; WebSession_%s;\r\n\r\n\
\r\n\
\r\n\r\n",HaiKanUrl,host_addr,host_addr,Session.sessionID);
				}
				else if(HaveLoginCap)
				{
					if(atoi(LoginCap.sessionIDVersion)==2)
					sprintf(request,"POST /ISAPI/Security/sessionLogin?timeStamp=%d HTTP/1.1\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/login.asp\r\n\
Accept-Language: zh-CN\r\n\
Accept-Encoding: gzip, deflate\r\n\
Host: %s\r\n\
Content-Length: 323\r\n\r\n\
<SessionLogin><userName>%s</userName><password>%s</password><sessionID>%s</sessionID><isSessionIDValidLongTerm>false</isSessionIDValidLongTerm><sessionIDVersion>2</sessionIDVersion></SessionLogin>\r\n\
\r\n\r\n",(unsigned)time(NULL),host_addr,host_addr,user,passwd,LoginCap.sessionID);
					else
					sprintf(request,"POST /ISAPI/Security/sessionLogin?timeStamp=%d HTTP/1.1\r\n\
Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n\
If-Modified-Since: 0\r\n\
X-Requested-With: XMLHttpRequest\r\n\
Referer: http://%s/doc/page/login.asp\r\n\
Accept-Language: zh-CN\r\n\
Accept-Encoding: gzip, deflate\r\n\
Host: %s\r\n\
Content-Length: 183\r\n\r\n\
<SessionLogin><userName>%s</userName><password>%s</password><sessionID>%s</sessionID></SessionLogin>\r\n\
\r\n\r\n",(unsigned)time(NULL),host_addr,host_addr,user,passwd,LoginCap.sessionID);
				}
				else
				{
					IsSecurityMode=TRUE;
					sprintf(request,"GET /ISAPI/Security/sessionLogin/capabilities?username=%s&random=%d HTTP/1.1\r\n\
Host: %s\r\n\
\r\n\r\n",user,(unsigned)time(NULL)%100000000,host_addr);
				}
			}
			else
			{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Authorization: Digest %s\r\n\
\r\n\r\n",HaiKanUrl,host_addr,DigestResponse);
			}
		}
		else
		{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Authorization: Basic %s\r\n\
\r\n\r\n",HaiKanUrl,host_addr,pBase64Buf);
		}
	}
	else
	{
		if(IsDigestAuth[IpcIndex])
		{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Cache-Control: max-age=0\r\n\
Authorization: Digest %s\r\n\
\r\n\r\n",DahuaUrl,host_addr,DigestResponse);
		}
		else
		{
			sprintf(request,"GET %s\r\n\
Host: %s\r\n\
Connection: close\r\n\
Cache-Control: max-age=0\r\n\
Authorization: Basic %s\r\n\
\r\n\r\n",DahuaUrl,host_addr,pBase64Buf);
		}
	}
	send=0;
	totalsend=0; 
	nbytes=strlen(request);
	while(totalsend < nbytes)  
    {
		send=write(sockfd,request+totalsend,nbytes-totalsend); 
        if(send == -1)     
        {
			ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"httpclient send error(%s)\r\n",strerror(errno));
			BASE64_BUF_FREE(pBase64Buf);
			KillSocket(&sockfd);
			return FALSE;
        }
		totalsend += send;
    }
	memset(buffer,0,sizeof(buffer));
	bytesRead=getResponse(sockfd,buffer,readBufSize);
	if(bytesRead <= 0) 
	{
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		return FALSE;
	}
	else
	{
		ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_DEBUG,"recv:%s\r\n",buffer);
	}
	firstLine=buffer;
	nextLineStart=getLine(firstLine);
	if(sscanf(firstLine,"%*s%u",&responseCode) != 1) 
	{
		ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"no response code in firstLine\r\n");
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		return FALSE;
	}
	if(responseCode==200) 
	{
		if(IsSecurityMode)
		{
			for(;;)
			{
				unsigned int contentLength=0;
				
				lineStart=nextLineStart;
				if(lineStart==NULL || lineStart[0]=='\0')
					break;
				nextLineStart=getLine(lineStart);
				if(sscanf(lineStart,"Content-Length: %d",&contentLength)!=1 && sscanf(lineStart,"CONTENT-LENGTH: %d",&contentLength)!=1)
					continue;
				if(contentLength>0)
				{
ReadNext:
					lineStart=nextLineStart;
					if(lineStart==NULL || lineStart[0]=='\0')
						break;
					nextLineStart=getLine(lineStart);
					if(nextLineStart!=NULL)
					{
						BOOL ret=FALSE;
						if(contentLength>(unsigned int)(bytesRead-(nextLineStart-buffer)))
						{
							contentLength=bytesRead-(nextLineStart-buffer);
						}
						if(HaveLogoutSession)
						{
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"IPC Logout ok\r\n");
						}
						else if(HaveUnlockSession)
						{
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"IPC Unlock ok\r\n");
						}
						else if(HaveSession)
						{
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"IPC Setsnap ok\r\n");
							char					BasePathName[64];
							char					FilePathName[64];
							time_t 					rawtime;
							int						OldStatus;
							struct 					tm *timeinfo;
							static DWORD			CurrPicIndex=0;
							static time_t			LastPicTime=0;

							if(contentLength>(unsigned int)(bytesRead-(nextLineStart-buffer)))
							{
								contentLength=bytesRead-(nextLineStart-buffer);
								ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"try to truncation...\r\n");
							}
							PTHREAD_MUTEX_SAFE_LOCK(sPicFileOpMutex,OldStatus);
							time(&rawtime);
							timeinfo=localtime(&rawtime);
							if(LastPicTime!=rawtime)
							{
								LastPicTime=rawtime;
								CurrPicIndex=1;
							}
							else
								CurrPicIndex++;
							sprintf(BasePathName,"%s_%.4d%.2d%.2d_%.2d%.2d%.2d_%.2d.%s",FsuID,1900+timeinfo->tm_year,
								1+timeinfo->tm_mon,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,CurrPicIndex,"jpg");
							PTHREAD_MUTEX_SAFE_UNLOCK(sPicFileOpMutex,OldStatus);
							sprintf(FilePathName,"%s%s",FSU_IPC_ALARM_CAP_BASE_DIR,BasePathName);
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"cap file (%s)...\r\n",FilePathName);
							
							pFile=fopen(FilePathName,"wb");
							if(NULL!=pFile)
							{
								if(fwrite(nextLineStart,1,contentLength,pFile)!=contentLength)
								{
									ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"cap failed...\r\n");
									fclose(pFile);
									pFile=NULL;
									goto err;
								}
								else
								{
									ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"cap success...\r\n");
									fflush(pFile);
									fsync(fileno(pFile));
									fclose(pFile);
									pFile=NULL;
									if(pSnapFileName!=NULL)
										strcpy(pSnapFileName,FilePathName);
									break;
								}
							}
							else
							{
								ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"cap failed2...\r\n");
								goto err;
							}
						}
						else if(HaveLoginCap)
						{
							ret=IpcSessionParse(nextLineStart,contentLength,&Session);
							if(ret)
							{
								if(atoi(Session.statusValue)==200)
									HaveSession=TRUE;
								else
									break;
								BASE64_BUF_FREE(pBase64Buf);
								KillSocket(&sockfd);
								goto SnapRetry;
							}
							else
								goto ReadNext;
						}
						else
						{
							ret=IpcLoginCapParse(nextLineStart,contentLength,&LoginCap);
						}
					}
				}
			}
		}
		else
		{
			for(;;)
			{
				unsigned int contentLength=0;
				
				lineStart=nextLineStart;
				if(lineStart==NULL || lineStart[0]=='\0')
					break;
				nextLineStart=getLine(lineStart);
				if(sscanf(lineStart,"Content-Length: %d",&contentLength)!=1 && sscanf(lineStart,"CONTENT-LENGTH: %d",&contentLength)!=1)
					continue;
				if(contentLength>0)
				{
					lineStart=nextLineStart;
					if(lineStart==NULL || lineStart[0]=='\0')
						break;
					nextLineStart=getLine(lineStart);
					if(nextLineStart!=NULL)
					{
						char					BasePathName[1024];
						char					FilePathName[1024];
						time_t 					rawtime;
						int						OldStatus;
						struct 					tm *timeinfo;
						static DWORD			CurrPicIndex=0;
						static time_t			LastPicTime=0;

						if(contentLength>(unsigned int)(bytesRead-(nextLineStart-buffer)))
						{
							contentLength=bytesRead-(nextLineStart-buffer);
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"try truncation2...\r\n");
						}
						PTHREAD_MUTEX_SAFE_LOCK(sPicFileOpMutex,OldStatus);
						time(&rawtime);
						timeinfo=localtime(&rawtime);
						if(LastPicTime!=rawtime)
						{
							LastPicTime=rawtime;
							CurrPicIndex=1;
						}
						else
							CurrPicIndex++;
						sprintf(BasePathName,"%s_%.4d%.2d%.2d_%.2d%.2d%.2d_%.2d.%s",FsuID,1900+timeinfo->tm_year,
							1+timeinfo->tm_mon,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,CurrPicIndex,"jpg");
						PTHREAD_MUTEX_SAFE_UNLOCK(sPicFileOpMutex,OldStatus);
						sprintf(FilePathName,"%s%s",sIpcServer.m_strCapPath,BasePathName);
						ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"cap file(%s)...\r\n",FilePathName);
						
						pFile=fopen(FilePathName,"wb");
						if(NULL!=pFile)
						{
							if(fwrite(nextLineStart,1,contentLength,pFile)!=contentLength)
							{
								ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"cap failed 3...\r\n");
								fclose(pFile);
								pFile=NULL;
								goto err;
							}
							else
							{
								ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"cap success 2...\r\n");
								fflush(pFile);
								fsync(fileno(pFile));
								fclose(pFile);
								pFile=NULL;
								if(pSnapFileName!=NULL)
									strcpy(pSnapFileName,FilePathName);
								break;
							}
						}
						else
						{
							ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_NOTICE,"cap failed 4...\r\n");
							goto err;
						}
					}
				}
			}
		}
	}
	else if(responseCode==401)
	{
		if(HaveLogoutSession)
		{
			BASE64_BUF_FREE(pBase64Buf);
			KillSocket(&sockfd);
			return TRUE;
		}
		for(;;)
		{
			unsigned int contentLength=0;
			BOOL retDigest=FALSE;

			lineStart=nextLineStart;
			if(lineStart==NULL || lineStart[0]=='\0')
				break;
			if(strstr(lineStart,"WWW-Authenticate: Digest") != NULL)
				IsDigestAuth[IpcIndex]=TRUE;
			else
				IsDigestAuth[IpcIndex]=FALSE;
			retDigest=GreateDigestAuthResponse(lineStart,strlen(lineStart),IsDahuaIpc[IpcIndex]?DahuaUrl:HaiKanUrl,user,passwd,DigestResponse);
			if(!retDigest)
				IsDigestAuth[IpcIndex]=FALSE;
			RetryCnt++;
			BASE64_BUF_FREE(pBase64Buf);
			KillSocket(&sockfd);
			if(RetryCnt<=5)
			{
				goto SnapRetry;
			}
			IsDigestAuth[IpcIndex]=FALSE;
			return FALSE;
		}
	}
	else if(responseCode==400)
	{
		if(IsDahuaIpc[IpcIndex])
		{
			IsDahuaIpcN[IpcIndex]=!IsDahuaIpcN[IpcIndex];
		}
		RetryCnt++;
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		if(RetryCnt<=5)
		{
			goto SnapRetry;
		}
		return FALSE;
	}
	else
	{
		RetryCnt++;
		BASE64_BUF_FREE(pBase64Buf);
		KillSocket(&sockfd);
		if(RetryCnt<=1)
		{
			IsDahuaIpc[IpcIndex] = !IsDahuaIpc[IpcIndex];
			goto SnapRetry;
		}
		return FALSE;
	}
	BASE64_BUF_FREE(pBase64Buf);
	KillSocket(&sockfd);
	if(HaveSession&&!HaveLogoutSession)
	{
		HaveLogoutSession=TRUE;
		goto SnapRetry;
	}
	return TRUE;
err:
	BASE64_BUF_FREE(pBase64Buf);
	KillSocket(&sockfd);
	return FALSE;
}

static BOOL IpcSetSnapPic(char *ID,DWORD IpcIndex,BOOL IsSubStream,char *pSnapFileName)
{
	if(IpcIndex>=SP_SYS_IPC_NUM_MAX || !sFsuIpcIsInitReady)
		return FALSE;
	return IpcSetSnapPicEx(ID,IpcIndex,IsSubStream,pSnapFileName,gIpcAccountIndex[IpcIndex]);
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_IpcSnapShot
 *功能描述: 抓拍IPC图像
 *输入描述: ID,IPC标识
 *输出描述: 无
 *返回描述: 成功(TRUE)/失败(NULL)
 *作者日期: LJJ/2024/12/30
 *全局声明: 无
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_IpcSnapShot(char *ID,DWORD IpcIndex,char *pFileName)
{
	if(!sFsuIpcIsInitReady)
		return FALSE;
	return IpcSetSnapPic(ID,IpcIndex,FALSE,pFileName);
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_LprProcess
 *功能描述: 车牌识别
 *输入描述: Model路径,Img路径
 *输出描述: 无
 *返回描述: 成功(TRUE)/失败(NULL)
 *作者日期: LJJ/2024/12/30
 *全局声明: 无
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_LprProcess(char *ModelPath,char *ImgPath,PCODE_LIST pCodeList) 
{
	if(!sFsuIpcIsInitReady)
		return FALSE;
	return ZFY_LprProcessEx(ModelPath,ImgPath,pCodeList);
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_IpcInit
 *功能描述: IPC抓拍模块初始化
 *输入描述: IPC配置
 *输出描述: 无
 *返回描述: 成功(TRUE)/失败(NULL)
 *作者日期: LJJ/2024/12/26
 *全局声明: sPicFileOpMutex,gFileHead,gFileList
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern BOOL ZFY_IpcInit(PIPC_API_CONF pConf)
{
	
	int				OldStatus;

	if(NULL==pConf || NULL==pConf->strIpcServerIP || NULL==pConf->strLoginUser || NULL==pConf->strLoginPwd)
		return FALSE;
	PTHREAD_MUTEX_SAFE_LOCK(sPicFileOpMutex,OldStatus);
	if(!sFsuIpcIsInitReady)
	{
		strncpy(sIpcServer.m_strIpcServerIP,pConf->strIpcServerIP,sizeof(sIpcServer.m_strIpcServerIP)-1);
		sIpcServer.m_IpcServerPort=pConf->IpcServerPort==0?80:pConf->IpcServerPort;
		strncpy(sIpcServer.m_strLoginUser,pConf->strLoginUser,sizeof(sIpcServer.m_strLoginUser)-1);
		strncpy(sIpcServer.m_strLoginPwd,pConf->strLoginPwd,sizeof(sIpcServer.m_strLoginPwd)-1);
		if(pConf->strPicPath!=NULL)
			strncpy(sIpcServer.m_strCapPath,pConf->strPicPath,sizeof(sIpcServer.m_strCapPath)-1);
		if(pConf->strRecPath!=NULL)
			strncpy(sIpcServer.m_strRecPath,pConf->strRecPath,sizeof(sIpcServer.m_strRecPath)-1);
		sFsuIpcIsInitReady=TRUE;
		ZFY_LES_LogPrintf("ZFY_IPC",LOG_EVENT_LEVEL_INFO,"IPC process unit is ready...\r\n");
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sPicFileOpMutex,OldStatus);
	return TRUE;
}

/*
 ************************************************************************************************************************************************************************     
 *函数名称: ZFY_IpcUnInit
 *功能描述: IPC抓拍模块反初始化
 *输入描述: 无
 *输出描述: 无
 *返回描述: 无
 *作者日期: LJJ/2024/12/02
 *全局声明: sPicFileOpMutex,sIpcServer
 *特殊说明: 无
 ************************************************************************************************************************************************************************       
*/
extern void ZFY_IpcUnInit(void)
{
	int	OldStatus;

	PTHREAD_MUTEX_SAFE_LOCK(sPicFileOpMutex,OldStatus);
	if(sIpcServer.IsInitReady)
	{
		sIpcServer.IsReqWatchQuit=TRUE;
		usleep(2*PTHREAD_DEFAULT_QUIT_TIMEOUT_US);
		sIpcServer.IsInitReady=FALSE;
	}
	PTHREAD_MUTEX_SAFE_UNLOCK(sPicFileOpMutex,OldStatus);
}

/********************************************ZFY_Ipc.c File End******************************************************/


