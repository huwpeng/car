#!/bin/sh
#123456789*987654321#

#Start script file content,���й̼������ű��ļ���ʼ5�б����뱾ģ���ļ�����һ��,�ļ���ʽ������Linux�ı��ļ���ʽ����Windowsϵͳ�½�����UltraEdit�༭��
#4���ű���������ֱ��ʾFTP�û�����FTP�û����롢FTP�������˿ں�FTP������IP��

#�����ű�����ѡ��
#��ʾԭʼ����
#set -x
#��ʾ�滻���ʵ������
#set -v
#ִ���﷨��鵫��ʵ������
#set -n

#�������
UPDATESH='/var/FirmwareUpdateAsReboot.sh'
DOWNLOAD_DIR='/var'
FTP_CMD='busybox ftpget'
FTP_USER=${1:?FTP�û�������Ϊ��}
FTP_PASSWORD=${2:?FTP�û����벻��Ϊ��}
FTP_PORT=${3:?FTP�������˿ڲ���Ϊ��}
FTP_SERVERIP=${4:?FTP������IP����Ϊ��}
RM_CMD='rm'
MV_CMD='mv'
CHMOD_CMD='chmod'

#���庯��
FileDownload()						#��������Ϊ�����ļ�����Զ���ļ���
{
	$FTP_CMD -u $FTP_USER -p $FTP_PASSWORD -P $FTP_PORT $FTP_SERVERIP $DOWNLOAD_DIR/$1 $2
	if [  $? -ne 0  -o  ! -s $DOWNLOAD_DIR/$1  ] ; then
		exit 1
	fi
	return 0;
}

FileUpdate()						#��������Ϊ���ص����ļ����������µľ��ļ����ļ�����Ȩ��
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

echo "�̼������ű�����ʼִ��......"
echo "���ȿ�ʼ��������ļ�......"


FileDownload FsuTool FsuTool


echo "���ڿ�ʼ�����ļ�......"


FileUpdate FsuTool /dvs/FsuTool +x


echo "�����ļ��ɹ�������ϣ�"
return 0

