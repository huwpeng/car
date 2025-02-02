#/bin/bash
i=1
while [ $i -le 20 ]
do
	echo "send data "
	echo "iamroladevice" >/dev/ttyS2
	sleep 1
	#((i++))
	#i=$[$i + 1]
	let i++
done

