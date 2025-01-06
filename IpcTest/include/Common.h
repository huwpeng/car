#ifndef _COMMON_H_
#define _COMMON_H_

#ifdef	__cplusplus
#if		__cplusplus
extern "C" {
#endif
#endif
/*
************************************************************************************************************************************************************************
************************************************************************************************************************************************************************
*			程序模块声明
*
*			文件:		Common.h
*			版本:		1.0
*			作者:		曾春桥
*			日期:		2006/10/08
*
*			亚奥数码技术有限公司版权所有 2006-2007
*
************************************************************************************************************************************************************************
************************************************************************************************************************************************************************
*/
	
/*
************************************************************************************************************************************************************************
* 程序模块功能描述
*
*
************************************************************************************************************************************************************************
*/

/*
************************************************************************************************************************************************************************
* 版本 1.0   2006/10/08
* 初始化版本 
************************************************************************************************************************************************************************
*/

/*
************************************************************************************************************************************************************************
* 引用文件
************************************************************************************************************************************************************************
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <crypt.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <termios.h>
#include <signal.h>
#include <sched.h>
#include <netdb.h>
#include <dirent.h>
#include <utmp.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/reboot.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <linux/hdreg.h>
#include <linux/fs.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>

#include "Pm5kType.h"




/*
************************************************************************************************************************************************************************
* 常数与宏定义
************************************************************************************************************************************************************************
*/


/*公共常数定义*/
#ifndef TRUE
#define	TRUE					1
#endif
#ifndef FALSE
#define FALSE					0
#endif
#define True					1
#define False					0

#define STD_SUCCESS				0
#define STD_FAILURE				(-1)
#define STD_INVALID_HANDLE		(-1)
#define INVALID_PTHREAD_ID		((DWORD)(-1))		

#define PTHREAD_DEFAULT_SCHED_PRIORITY		0
#define PTHREAD_DEFAULT_QUIT_TIMEOUT_US		(200*1000)


#define SYS_WATCHDOG_SCHED_PRIORITY			16
#define SYS_UPDATE_SCHED_PRIORITY			15
#define SYS_RESET_SCHED_PRIORITY			14
#define SYS_APP_SCHED_PRIORITY				0

#define APP_REALTIME_SCHED_PRIORITY			8
#define APP_NORMAL_SCHED_PRIORITY			0

#define SYNC_APP_SCHED_PRIORITY				4
#define BUSY_LOOP_SCHED_PRIORITY			0

#define FOREGROUND_APP_SCHED_PRIORITY		1
#define BACKGROUND_APP_SCHED_PRIORITY		0

#define CODEC_SCHED_PRIORITY				1
#define TRANSMIT_SCHED_PRIORITY				0

#define RS485_SCHED_PRIORITY				1
#define RS232_SCHED_PRIORITY				0

#define DATA_SCHED_PRIORITY					3
#define AUDIO_SCHED_PRIORITY				2
#define VIDEO_SCHED_PRIORITY				1
#define CMD_SCHED_PRIORITY					0




/*通讯协议相关公共常数定义*/
#define	AVCM_SERVER_PORT					9000
#define	CBC_SEND_PORT						(AVCM_SERVER_PORT+1)			
#define	CBC_RECV_PORT						(AVCM_SERVER_PORT+2)		
#define AVCM_SERVER_AUDIO_PORT				(AVCM_SERVER_PORT+3)
#define	AVCM_SERVER_BPORT					(AVCM_SERVER_PORT+4)
#define AVCM_LOG_SERVER_PORT				(AVCM_SERVER_PORT+5)
#define AVCM_MANAGER_SERVER_PORT			(AVCM_SERVER_PORT+6)
#define	AVCM_RECORD_PORT					(AVCM_SERVER_PORT+7)
#define AVCM_ADDR_SERVER_PORT				9518
#define UDP_PRINTF_DEFAULT_PORT				50011
#define NTP_SERVER_STD_PORT					123
#define	AVCM_FIRMWARE_UPDATE_PORT			9090

#define DEV_MCAST_TO_CH_MCAST(mc,ch)		(((DWORD)(mc)>>24)<232?((DWORD)(mc)+(((ch)&0x07)<<24)):((DWORD)(mc)-(((ch)&0x07)<<24)))

#define SYS_MAX_USER_NUM					32
#define SYS_LOGIN_SERVER_MIN_CH_NUM			1
#define SYS_LOGIN_SERVER_MAX_CH_NUM			(SYS_LOGIN_SERVER_MIN_CH_NUM+16)
#define INVALID_DEV_NET_ID					0xFFFFFFFF
#define MIN_SERDEV_UNIT						1
#define MAX_VIDEO_MD_REGION_NUM				4
#define	DEV_NET_MAX_SWITCH					4
#define SYS_MEDIA_STREAM_MAX_CH_NUM			4
#define SYS_MEDIA_STREAM_MAX_VIRTUAL_CH_NUM	16

#define VIDEO_MD_MODE_NORMAL				0
#define VIDEO_MD_MODE_AND					1
#define VIDEO_MD_MODE_SEQ_MOTION			2
#define VIDEO_MD_MODE_REALTIME_SEQ_MOTION	3
#define VIDEO_MD_MODE_ANY_REGION			4

#define SYS_MOUNT_BASE_DIR					"/mnt/media"
#define SYS_MOUNT_FAIL_FLAG					".MountFailFlag."
#define SYS_MOUNT_HD_DIR					"hd/record"
#define SYS_MOUNT_USB_DIR					"usb"
#define SYS_MOUNT_SD_DIR					"sd"
#define SYS_MOUNT_NFS_DIR					"nfs"
#define SYS_MOUNT_ISCSI_DIR					"iscsi"
#define SYS_PIC_AUTO_SAVE_DIR				"pic-auto"
#define	SYS_PIC_MANUAL_SAVE_DIR				"pic-manual"

#define SYS_DEFAULT_IMAGE_BRIGHTNESS		0x80
#define SYS_DEFAULT_IMAGE_SATURATION		0x74		
#define SYS_DEFAULT_IMAGE_CONTRAST			0x5E
#define SYS_DEFAULT_IMAGE_HUE				0x00

#define SYS_DEFAULT_DVR_IMAGE_CONTRAST		0x80
#define SYS_DEFAULT_DVR_IMAGE_HUE			0x80

#define SYS_DEFAULT_ED_IMAGE_BRIGHTNESS		0x80
#define SYS_DEFAULT_ED_IMAGE_SATURATION		0x74					
#define SYS_DEFAULT_ED_IMAGE_CONTRAST		0x5E
#define SYS_DEFAULT_ED_IMAGE_HUE			0x00



#define MMNET_INVALID_FORMAT	0xFFFF
	
#define VIDEO_MJPG				0x0001
#define VIDEO_MPEGI				0x0002
#define VIDEO_H263				0x0004
#define VIDEO_MPEGII_TS			0x0005
#define VIDEO_MPEGII_PS			0x0006
#define VIDEO_MPEG4				0x0007
#define VIDEO_H264				0x0008

#define VIDEO_FORMAT_SIF		0x20
#define VIDEO_FORMAT_QSIF		0x30
#define	VIDEO_FORMAT_HDI		0x50
#define	VIDEO_FORMAT_DI			0x60
#define	VIDEO_FORMAT_DIA		0x70
#define	VIDEO_FORMAT_4SIF		0x80
#define VIDEO_FORMAT_2SIF		0x90
#define VIDEO_FORMAT_VGA		0xA0
#define VIDEO_FORMAT_QVGA		0xB0
#define VIDEO_FORMAT_SXGA		0xC0
#define VIDEO_FORMAT_720P		0xD0
#define VIDEO_FORMAT_1080P		0xE0


#define VIDEO_FORMAT_NTSC		0x100
#define VIDEO_FORMAT_PAL		0x300
#define VIDEO_FORMAT_DC			0x400


#define VIDEO_STD_WIDTH				720
#define VIDEO_STD_HEIGHT_PAL		576
#define VIDEO_STD_HEIGHT_NTSC		480
#define VIDEO_CAPTURE_MAX_WIDTH		VIDEO_STD_WIDTH
#define VIDEO_CAPTURE_MAX_HEIGHT	VIDEO_STD_HEIGHT_PAL

#define VIDEO_LS_MJ_MASK        0x0000000F
#define VIDEO_FORMAT_SIF_MASK   0x000000F0
#define VIDEO_FORMAT_NTSC_MASK  0x00000F00


#define AUDIO_FORMAT_G711U			0x00
#define AUDIO_FORMAT_G711A			0x01
#define AUDIO_FORMAT_ADPCM			0x02
#define AUDIO_FORMAT_G726			0x03
#define AUDIO_FORMAT_AMR			0x04
#define AUDIO_FORMAT_AAC			0x05
#define AUDIO_FORMAT_MASK			0x0F

#define AUDIO_SAMPLE_BITRATE_8K		0x00
#define AUDIO_SAMPLE_BITRATE_32K	0x10
#define AUDIO_SAMPLE_BITRATE_48K	0x20
#define AUDIO_SAMPLE_BITRATE_MASK	0xF0

#define AUDIO_AMR_FORMAT_MMS		0x000
#define AUDIO_AMR_FORMAT_IF1		0x100
#define AUDIO_AMR_FORMAT_IF2		0x200
#define AUDIO_AMR_FORMAT_MASK		0xF00

#define AUDIO_AMR_BITRATE_4750		0x0000
#define AUDIO_AMR_BITRATE_5150		0x1000	
#define AUDIO_AMR_BITRATE_5900		0x2000
#define AUDIO_AMR_BITRATE_6700		0x3000
#define AUDIO_AMR_BITRATE_7400		0x4000
#define AUDIO_AMR_BITRATE_7950		0x5000
#define AUDIO_AMR_BITRATE_10200		0x6000
#define AUDIO_AMR_BITRATE_12200		0x7000
#define AUDIO_AMR_BITRATE_MASK		0xF000

#define AUDIO_SAMPLE_BITS_MASK		0x00FF0000
#define AUDIO_SAMPLE_BITS_8			0x00080000
#define AUDIO_SAMPLE_BITS_16		0x00100000
#define AUDIO_SAMPLE_BITS_32		0x00200000

#define AUDIO_SAMPLE_CHANNELS_MASK	0x0F000000
#define AUDIO_SAMPLE_CHANNELS_1		0x01000000
#define AUDIO_SAMPLE_CHANNELS_2		0x02000000

#define SYS_CAMERA_POWER_RESET_DELAY						0
#define SYS_CAMERA_POWER_RESET_DEFAULT_PRESET				30




/*常用公共工具宏定义*/
#define GET_SYS_SCHED_CLOCK_TICK_FREQUENCY				sysconf(_SC_CLK_TCK)
#define PTHREAD_MUTEX_SAFE_LOCK(mutex,OldStatus)		do{pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&OldStatus);pthread_mutex_lock(&mutex);}while(0)
#define PTHREAD_MUTEX_SAFE_UNLOCK(mutex,OldStatus)		do{pthread_mutex_unlock(&mutex);pthread_setcancelstate(OldStatus,NULL);}while(0)
#define PTHREAD_CANCEL_LOCK(OldStatus)					pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&OldStatus)					
#define PTHREAD_CANCEL_UNLOCK(OldStatus)				pthread_setcancelstate(OldStatus,NULL)
#define EXACT_DELAY_MS(MsNum)							do{int Delay=MsNum*1000;while(Delay>=500)Delay=usleep(Delay);}while(0)
#define MSLEEP(x)										usleep(1000*(x))
#define STRUCT_TM_SHOW_CUSTOM(pTM)						do{(pTM)->tm_mon++;(pTM)->tm_year += 1900;}while(0)


#define GET_CURRENT_SYS_TIME(SysTime)					do{(SysTime)=ConfigServerGetSysRunTime(FALSE);}while(0)
#define GET_CURRENT_SYS_TIME_MS(SysTime)				do{(SysTime)=ConfigServerGetSysRunTime(TRUE);}while(0)

#define GET_MAX_VAL(a,b)								(((a) > (b)) ? (a) : (b))
#define GET_MIN_VAL(a,b)								(((a) < (b)) ? (a) : (b))
#define GET_VALID_VAL(maxval,minval,val)				GET_MIN_VAL(GET_MAX_VAL((val),(minval)),(maxval))
#define	ARRAY_SIZE_BY_ELEMENT(a)						(sizeof(a)/sizeof((a)[0]))


/*
************************************************************************************************************************************************************************
* 数据类型定义
************************************************************************************************************************************************************************
*/

typedef int							Boolean;
typedef int							BOOL;

typedef unsigned long int			DWORD;
typedef unsigned short int			WORD;
typedef unsigned char				BYTE;

typedef unsigned long long			U64;
typedef signed long long			S64;
typedef unsigned long int			U32;
typedef signed long int				S32;
typedef unsigned short int			U16;
typedef signed short int			S16;
typedef unsigned char				U8;
typedef signed char					S8;

typedef void *(*PPTHREAD_ENTRY)(void *);

typedef struct __attribute__((packed)) tHI_AUDIO_FRAME_HEADER
{
#define HI_AUDIO_NORMAL_FRAME		0x01
	BYTE	Reserved;
	BYTE	Type;
	WORD	PayloadLen;
}HI_AUDIO_FRAME_HEADER,*PHI_AUDIO_FRAME_HEADER;


#define BITMAP_FILE_TYPE_BMP		0x4D42
#define BI_RGB						0
typedef struct __attribute__((packed)) tagBITMAPFILEHEADER
{
	WORD    bfType;
	DWORD   bfSize;
	WORD    bfReserved1;
	WORD    bfReserved2;
	DWORD   bfOffBits;
}BITMAPFILEHEADER,*PBITMAPFILEHEADER;
typedef struct __attribute__((packed)) tagBITMAPINFOHEADER
{
	DWORD  biSize;
	long   biWidth;
	long   biHeight;
	WORD   biPlanes;
	WORD   biBitCount;
	DWORD  biCompression;
	DWORD  biSizeImage;
	long   biXPelsPerMeter;
	long   biYPelsPerMeter;
	DWORD  biClrUsed;
	DWORD  biClrImportant;
}BITMAPINFOHEADER,*PBITMAPINFOHEADER;




/*
************************************************************************************************************************************************************************
* 函数原型声明
************************************************************************************************************************************************************************
*/

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




/*
 ************************************************************************************************************************************************************************
 * 全局变量输出声明
 ************************************************************************************************************************************************************************
*/




#ifdef	__cplusplus
#if		__cplusplus
}
#endif
#endif

/******************************************Common.h 文件结束******************************************************************************************************/
#endif
