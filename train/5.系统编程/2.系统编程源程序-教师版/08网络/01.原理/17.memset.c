#include <string.h>
#include <stdio.h>

int main()
{
	char buf[20];
	memset(buf,'9',sizeof(buf)-1);
	buf[19] = '\0';
	printf("buf=%s\n",buf);
}
