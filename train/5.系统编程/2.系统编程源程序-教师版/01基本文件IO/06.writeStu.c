#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

struct stu{
	int id;
	char name[20];
};

int main()
{
	struct stu s;
	struct stu s1;

	int fd = open("stu",O_RDWR|O_CREAT|O_TRUNC,0640);
	if(fd == -1){
		printf("open error\n");
		exit(-1);
	}
	
	printf("please input the stu info:\n");
	scanf("%d",&s.id);
	scanf("%s",s.name);
	
	int ret = write(fd,&s,sizeof(s));
	if(ret == -1){
		printf("write error\n");
	}
	
	lseek(fd,0,SEEK_SET);

	ret = read(fd,&s1,sizeof(s1));
	if(ret == -1){
		printf("read error\n");
	}
	printf("id = %d\n",s1.id);
	printf("name = %s\n",s1.name);

	return 0;
}
