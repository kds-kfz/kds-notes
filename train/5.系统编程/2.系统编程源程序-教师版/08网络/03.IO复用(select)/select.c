	lfd = socket()
	bind()
	listen(lfd,128)

	int maxfd;//最大文件描述符
		
	1.lfd;起始只有监听的文件描述符
	2.maxfd = lfd;

	fd_set allset, rset;//定义读事件集合
	FD_ZERO(&allset);//将读集合清空
	FD_SET(lfd,&allset);//将lfd添加到监听的集合中
	
	/*3个新连接*/
	while(1){
		rset = allset;
		nready = select(maxfd+1,fd_set* rset, fd_set* wset,fd_set* exset,NULL);
		/*代表有新的连接*/
		if(FD_ISSET(lfd,&rset)){
			cfd = accept(lfd,);
			
			client[i] = cfd;
			maxi = i;

			FD_SET(cfd,&allset);
			if(--nready == 0)
				continue;
		}

		for(i=0; i<maxi; i++){ 
			
			sockfd = client[i];

			if(FD_ISSET(sockfd,&rset)){
				/*读写数据*/
				FD_CLR(sockfd,&allset);
				client[i] = -1;


				if(--nready == 0)
					break;
			}
			
		}



	}
