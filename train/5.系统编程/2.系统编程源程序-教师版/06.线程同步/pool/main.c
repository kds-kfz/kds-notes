#include "pool.h"

void *thread_handle(void *argv)
{
	int id = *(int*)argv;

	for (int i = 0; i < 5; i++)
	{
		printf("in thread[%d]\n", id);
		usleep(50*1000);
	}

	return NULL;
}

int main()
{
	int ret;

	ret = pool_init(10);
	if (ret < 0)
	{
		fprintf(stderr, "init fail\n");
		return -1;
	}

	sleep(2);
	for (int i = 0; i < 5; i++)
	{
		ret = pool_add_worker(thread_handle, &i, sizeof(i));
		if (ret < 0)
		{
			fprintf(stderr, "add work fail\n");
			return -1;
		}
	}

	sleep(5);

	ret = pool_destroy();
	assert(ret == 0);
	return 0;
}
