大华摄像机IP 192.168.8.108 admin 123456abc
设备IP 192.168.8.100 调试串口115200

 https://releases.linaro.org/components/toolchain/binaries/
 
 vmhgfs-fuse .host:/ /mnt/hgfs/
 
 
 opencv compile
 
 sudo cmake -DCMAKE_MAKE_PROGRAM:PATH=/usr/bin/make -DWITH_CUDA=OFF -DENABLE_PRECOMPILED_HEADERS=OFF -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake -DCUDA_GENERATION=Kepler -DWITH_GTK=ON ..


 ./LprTest /opt/resource/models/r2_mobile /opt/resource/images/test_img.jpg
 
 
 wget --user admin --password 123456abc -O video.dav 'http://192.168.8.108/cgi-bin/loadfile.cgi?action=startLoad&channel=1&startTime=2025-01-01%%2020:28:02&endTime=2025-01-01%%2020:28:10'
 
 
 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
 
 
 ros
 https://blog.csdn.net/m0_59161987/article/details/128557068
 
 ## 继续执行如下命令：
sudo apt update && sudo apt install curl gnupg lsb-release
sudo curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(source /etc/os-release && echo $UBUNTU_CODENAME) main" | sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null
 
sudo rm /var/chche/apt/archives/lock
sudo rm /var/lib/dpkg/lock-frontend  #注意自己无法获得锁的路径
sudo apt install -y gcc nasm

sudo apt update
sudo apt upgrade
## 推荐桌面版，比较推荐。
sudo apt install ros-humble-desktop
## 安装时间可能较长，安心等待。

source /opt/ros/humble/setup.bash
echo " source /opt/ros/humble/setup.bash" >> ~/.bashrc

ros2 run demo_nodes_cpp talker
ros2 run demo_nodes_py listener
 
# 终端一
ros2 run turtlesim turtlesim_node
# 终端二
ros2 run turtlesim turtle_teleop_key


echo -e -n "AT?\xd\xa">>/dev/ttyS4

echo -e -n "\x1f\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x0a\x10\xaa\xbb\xf1">/dev/ttyS2



APN设置:具体是移动或者联通或者电信就是 Init3 Phone 稍有不同 其他基本一致，这里是电信。

移动： at+cgdcont=1,“ip”,“cmnet”

联通： at+cgdcont=1,“ip”,“3gnet”

电信： at+cgdcont=1,“ip”,“ctnet”

拨号：

移动：991#或981#

联通： *99#

电信： #777


busybox microcom /dev/ttyUSB2

可以使用查询指令看APN是否配置妥当
AT+CGDCONT?


AT+CGDCONT=1,"IP","cmnet"

AT+CFUN?
AT+CPIN?
AT+CEREG?
AT+CGDCONT?
AT+CGPADDR
AT+CGACT?
AT+QNETDEVCTL=1,1,1

AT+QIACT=1
AT+CGPADDR=1

ATDT#777

echo -en "AT+QCFG=\"usbnet\",1 \r\n" >/dev/ttyUSB2
echo -en "AT+QCFG=\"nat\",1 \r\n" >/dev/ttyUSB2
echo -en "AT+QNETDEVCTL=3,1,1 \r\n" >/dev/ttyUSB2
echo -en "AT+CFUN=1,1 \r\n" >/dev/ttyUSB2


echo "BBF+LORACFG=0,<idx>" >/dev/ttyS2
echo "BBF+LORACFG=<idx>,<value>" >/dev/ttyS2

echo "BBF+LORASEND=<dst_id>,<\x00\x0a\x10\xaa\xbb>" >/dev/ttyS2

echo "BBF+ENCKEY=<key>" >/dev/ttyS2
echo "BBF+ENCKEY=0" > /dev/ttyS2

echo "BBF+LORACFG=0,9" >/dev/ttyS2
echo "BBF+LORACFG=0,22" >/dev/ttyS2

echo "BBF+LORACFG=22,0000000000000001" >/dev/ttyS2
echo "BBF+LORASEND=0000000000000000,31323435360a0d" >/dev/ttyS2

echo "BBF+LORASEND=0000000000000001,1fffffffffffffffff00000a0002aabb5663f1" >/dev/ttyS2

