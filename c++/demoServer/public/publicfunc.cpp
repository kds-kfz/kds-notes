#include "publicfunc.h"

void ReleasePackData(char* packdata)
{
	nsdk_del_arry(packdata);
}

int ReleasePackDataFunc(void* lpParameter)
{
	if (lpParameter)
		ReleasePackData((char*)lpParameter);
	return 0;
}