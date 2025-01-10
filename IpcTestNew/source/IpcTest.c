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
	
} 