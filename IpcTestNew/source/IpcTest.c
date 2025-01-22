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
	ZFY_IpcGetTime(0);
	ZFY_IpcStartRecord(0);
	time(&start_time);
	sleep(10);
	time(&stop_time);
	ZFY_IpcStopRecord(0);
	ZFY_IpcSnapShot("alarm",0);
	sleep(5);
	ZFY_IpcLoadRecord(start_time,stop_time,0);
	memset(&code_list,0,sizeof(code_list));
	ZFY_LprProcess("/opt/resource/models/r2_mobile","/opt/resource/images/test_img.jpg",&code_list);
	for(i=0;i<code_list.CodeNum;i++)
		printf("-----code=%s-----\r\n",code_list.CodeList[i]);
	
	memset(&code_list,0,sizeof(code_list));
	ZFY_LprProcess("/opt/resource/models/r2_mobile","/opt/car/a.jpg",&code_list);
	for(i=0;i<code_list.CodeNum;i++)
		printf("-----code=%s-----\r\n",code_list.CodeList[i]);
	
	memset(&code_list,0,sizeof(code_list));
	ZFY_LprProcess("/opt/resource/models/r2_mobile","/opt/car/b.jpg",&code_list);
	for(i=0;i<code_list.CodeNum;i++)
		printf("-----code=%s-----\r\n",code_list.CodeList[i]);
	
	memset(&code_list,0,sizeof(code_list));
	ZFY_LprProcess("/opt/resource/models/r2_mobile","/opt/car/c.jpg",&code_list);
	for(i=0;i<code_list.CodeNum;i++)
		printf("-----code=%s-----\r\n",code_list.CodeList[i]);
	
	{
		IPC_CONFIG ipc_conf;
		
		ZFY_ConfConfigDataBaseInit();
		
		memset(&ipc_conf,0,sizeof(ipc_conf));
		ZFY_ConfIpcConfig(FALSE,&ipc_conf);
		printf("-------ipc--ip=0x%x--\r\n",ipc_conf.IpcIp);
		printf("-------ipc--port=%d--\r\n",ipc_conf.IpcPort);
		printf("-------ipc--user=%s--\r\n",ipc_conf.IpcUser);
		printf("-------ipc--pass=%s--\r\n",ipc_conf.IpcPwd);
		ipc_conf.IpcPort=88;
		strcpy(ipc_conf.IpcUser,"admin123");
		strcpy(ipc_conf.IpcPwd,"123456");
		ZFY_ConfIpcConfig(TRUE,&ipc_conf);
		ZFY_ConfIpcConfig(FALSE,&ipc_conf);
		printf("----2---ipc--ip=0x%x--\r\n",ipc_conf.IpcIp);
		printf("----2---ipc--port=%d--\r\n",ipc_conf.IpcPort);
		printf("----2---ipc--user=%s--\r\n",ipc_conf.IpcUser);
		printf("----2---ipc--pass=%s--\r\n",ipc_conf.IpcPwd);
	}
	
	{
		ZFY_RolaDevOpen();
		sleep(10);
		ZFY_RolaDevClose();
	}
	
	{
		CLOAD_PLAT_CONF CloadConf;
		
		memset(&CloadConf,0,sizeof(CloadConf));
		CloadConf.strCloadServerIP="192.168.8.11";
		CloadConf.CloadServerPort=1234;
		CloadConf.strLoginUser="admin";
		CloadConf.strLoginPwd="admin";
		CloadConf.strDevIMEI="868591050607174";
		CloadConf.strDevIMSI="460042187616489";
		
		ZFY_CloadServerOpen(&CloadConf);
		sleep(30);
		ZFY_CloadServerClose();
	}
} 