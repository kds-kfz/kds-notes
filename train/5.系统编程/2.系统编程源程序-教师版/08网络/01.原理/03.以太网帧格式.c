以太网帧格式:
[6字节目的地址][6字节源地址][2字节的以太网帧的类型][      46~1500字节的数据    ][4字节CRC校验位]
 

其中的源地址和目的地址是指网卡的硬件地址(也叫MAC地址),长度是48位,是在网卡出厂时固化的(全世界都是唯一的)
可以在shell中使用ifconfig命令查看,(Link encap:以太网  硬件地址 00:0c:29:14:50:24)就是硬件地址.协议字段有3种值,分别对应IP,ARP,RARP,帧尾是CRC校验码


[类型(0800)][IP数据报(46~1500字节)]
     2
[类型(0806)][ARP(获取下一个要经过的MAC地址)请求/应答][PAD]
     2            28       18
[类型(8035)][RARP请求/应答][PAD] --->PAD就是填充的意思
     2            28        18

TTL:在网络环境传输过程中最多可以经过多少跳(最多可以经过多少个路由器,没经过一个路由器TTL减一)




	 
