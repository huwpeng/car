#include<stdio.h>        
#include<stdlib.h>      
#include<unistd.h>       
#include<sys/types.h>   
#include<sys/stat.h>     
#include<fcntl.h>         
#include<errno.h>        
#include<string.h>  
   
#include "ZFY_Ipc.h"
#include "ZFY_Lpr.h"
#include "zfy_Conf.h"
#include "zfy_Rola.h"
#include "zfy_Platform.h"

   
int main(int argc, char **argv)  
{                        
    int i;  
	time_t start_time;
	time_t stop_time;
	char car_code[5][20];   
	char snap_path[64]={0};
	char rec_path[64]={0};
	CODE_LIST code_list;
	IPC_API_CONF IpcApiConf;
	
	memset(&IpcApiConf,0,sizeof(IpcApiConf));
	IpcApiConf.strIpcServerIP="192.168.8.108";
	IpcApiConf.IpcServerPort=80;
	IpcApiConf.strLoginUser="admin";
	IpcApiConf.strLoginPwd="123456abc";
	IpcApiConf.strPicPath="/opt/car/pic/";
	IpcApiConf.strRecPath="/opt/car/rec/";
	ZFY_IpcInit(&IpcApiConf);
	
#if 0
	ZFY_IpcGetTime(0);
	ZFY_IpcStartRecord(0);
	time(&start_time);
	sleep(10);
	time(&stop_time);
	ZFY_IpcStopRecord(0);
	if(ZFY_IpcSnapShot("alarm",0,snap_path))
		printf("-----snap_path=%s-----\r\n",snap_path);
	sleep(5);
	if(ZFY_IpcLoadRecord(start_time,stop_time,0,rec_path))
		printf("-----rec_path=%s-----\r\n",rec_path);
#endif

#if 0
	memset(&code_list,0,sizeof(code_list));
	ZFY_LprProcess("/home/resource/models/r2_mobile","/home/resource/images/test_img.jpg",&code_list);
	for(i=0;i<code_list.CodeNum;i++)
		printf("-----code=%s-----\r\n",code_list.CodeList[i]);
	
	memset(&code_list,0,sizeof(code_list));
	ZFY_LprProcess("/home/resource/models/r2_mobile","/home/car/a.jpg",&code_list);
	for(i=0;i<code_list.CodeNum;i++)
		printf("-----code=%s-----\r\n",code_list.CodeList[i]);
	
	memset(&code_list,0,sizeof(code_list));
	ZFY_LprProcess("/home/resource/models/r2_mobile","/home/car/b.jpg",&code_list);
	for(i=0;i<code_list.CodeNum;i++)
		printf("-----code=%s-----\r\n",code_list.CodeList[i]);
	
	memset(&code_list,0,sizeof(code_list));
	ZFY_LprProcess("/home/resource/models/r2_mobile","/home/car/c.jpg",&code_list);
	for(i=0;i<code_list.CodeNum;i++)
		printf("-----code=%s-----\r\n",code_list.CodeList[i]);
#endif

	{
		IPC_CONFIG ipc_conf;
		struct in_addr Addr;
		
		ZFY_ConfConfigDataBaseInit();
		memset(&ipc_conf,0,sizeof(ipc_conf));
		ZFY_ConfIpcConfig(FALSE,&ipc_conf);
		Addr.s_addr=htonl(ipc_conf.IpcIp);
		printf("-------ipc--ip=%s--\r\n",(char *)inet_ntoa(Addr));
		printf("-------ipc--port=%d--\r\n",ipc_conf.IpcPort);
		printf("-------ipc--user=%s--\r\n",ipc_conf.IpcUser);
		printf("-------ipc--pass=%s--\r\n",ipc_conf.IpcPwd);
		printf("-------AlarmFlag=%d-------------\r\n",ipc_conf.AlarmFlag);
	}
	
	{
		ZFY_RolaPowerCtrl(TRUE);
		ZFY_RolaDevOpen();
		sleep(900);
		ZFY_RolaDevClose();
	}
	
#if 0
	{
		CLOAD_PLAT_CONF CloadConf;
		
		memset(&CloadConf,0,sizeof(CloadConf));
		CloadConf.strCloadServerIP="192.168.8.11";
		CloadConf.CloadServerPort=1234;
		CloadConf.strLoginUser="admin";
		CloadConf.strLoginPwd="admin";
		CloadConf.strDevIMEI="868591050607174";
		CloadConf.strDevIMSI="460042187616489";
		
		ZFY_4GModelPowerCtrl(TRUE);
		ZFY_4GModelStartCtrl();
		ZFY_4GModelResetCtrl();
		ZFY_CloadServerOpen(&CloadConf);
		sleep(30);
		ZFY_CloadServerClose();
	}
#endif
} 