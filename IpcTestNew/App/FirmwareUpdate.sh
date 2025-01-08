#!/bin/sh
#123456789*987654321#

#Start script file content,所有固件升级脚本文件开始5行必须与本模板文件保持一致,文件格式必须是Linux文本文件格式，在Windows系统下建议用UltraEdit编辑。
#4个脚本输入参数分别表示FTP用户名、FTP用户密码、FTP服务器端口和FTP服务器IP。

#开启脚本调试选项
#显示原始命令
#set -x
#显示替换后的实际命令
#set -v
#执行语法检查但不实际运行
#set -n

#定义变量
UPDATESH='/var/FirmwareUpdateAsReboot.sh'
DOWNLOAD_DIR='/var'
FTP_CMD='busybox ftpget'
FTP_USER=${1:?FTP用户名不能为空}
FTP_PASSWORD=${2:?FTP用户密码不能为空}
FTP_PORT=${3:?FTP服务器端口不能为空}
FTP_SERVERIP=${4:?FTP服务器IP不能为空}
RM_CMD='rm'
MV_CMD='mv'
CHMOD_CMD='chmod'

#定义函数
FileDownload()						#参数依次为本地文件名，远程文件名
{
	$FTP_CMD -u $FTP_USER -p $FTP_PASSWORD -P $FTP_PORT $FTP_SERVERIP $DOWNLOAD_DIR/$1 $2
	if [  $? -ne 0  -o  ! -s $DOWNLOAD_DIR/$1  ] ; then
		exit 1
	fi
	return 0;
}

FileUpdate()						#参数依次为下载的新文件，将被更新的旧文件，文件访问权限
{
	$RM_CMD -f $2
	if [ $? -ne 0 ] ; then
		exit 1
	fi
	$MV_CMD -f $DOWNLOAD_DIR/$1 $2
	if [ $? -ne 0 ] ; then
		exit 1
	fi
	$CHMOD_CMD $3 $2
	if [ $? -ne 0 ] ; then
		exit 1
	fi
	return 0;
}

echo "固件升级脚本程序开始执行......"
echo "首先开始下载相关文件......"


FileDownload FsuTool FsuTool


echo "现在开始更新文件......"


FileUpdate FsuTool /dvs/FsuTool +x


echo "升级文件成功更新完毕！"
return 0

