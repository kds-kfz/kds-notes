socket,在linux环境下,用于表示进程间网络通信的特殊文件类型,本质为内核借助缓冲区形成的伪文件
因此,我们可以使用文件描述符来引用套接字,与管道类似,linux系统将其封装成文件的目的是为了统一接口,
使得读写套接字和读写文件的操作一致.区别是管道主要应用于本地进程间通信,而套接字多应用于网络进程间数据的传通

ip地址可以在网络环境中标识唯一的一台主机
端口号在主机中唯一标识一个进程
因此,IP+端口号,就能在网络环境中唯一标识这样一个进程,我们称这个进程为socket{IP,端口号}

对于管道而言,每当打开一个管道,一端表示读一端表示写
fd[0]  [    ] fd[1]

当创建一个socket时,一个fd文件描述符,指向两个缓冲区,一个缓冲区专门用来完成数据的读出,另外一个缓冲区用来完成数据的写入

                f d  
               /   \
             [读] [写]
这是全双工通信方式










