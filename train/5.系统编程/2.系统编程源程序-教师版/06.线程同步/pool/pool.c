#include "pool.h"

pool_t *pool = NULL;

/*销毁线程池*/
int pool_destroy()
{
	if (pool == NULL)
		return -1;
	if (pool->shutdown == 1)
		return -1;

	pool->shutdown = 1;
	pthread_cond_broadcast(&pool->cond);

	for (int i = 0; i < pool->number; i++)
	{
		pthread_join((pool->pth)[i], NULL);
	}

	worker_t *temp;
	while (pool->head != NULL)
	{
		temp = pool->head;
		pool->head = pool->head->next;
		free(temp->argv);
		free(temp);
	}

	pthread_mutex_destroy(&pool->mutex);
	pthread_cond_destroy(&pool->cond);

	free(pool->pth);
	free(pool);
	pool = NULL;
	return 0;
}

/*添加函数指针以及函数入参*/
int pool_add_worker(void *(*pfunc)(void*), void *argv, size_t size)
{
	if (pool == NULL)
		return -1;
	if (pool->shutdown == 1)
		return -1;
	
	worker_t *current, *temp;

	current = malloc(sizeof(worker_t));
	assert(current != NULL);

	current->pfunc = pfunc;
	current->argv = malloc(size);
	assert(current->argv != NULL);
	memcpy(current->argv, argv, size);
	current->next = NULL;

	pthread_mutex_lock(&pool->mutex);

	if (pool->head == NULL)
		pool->head = current;
	else 
	{
		temp = pool->head;
		while (temp->next != NULL)
			temp = temp->next;
		temp->next = current;
	}
	pthread_mutex_unlock(&pool->mutex);
	pthread_cond_signal(&pool->cond);
}

/*线程执行函数*/
void *thread_routine(void *argv)
{
	worker_t *temp;
	for (;;)
	{
		pthread_mutex_lock(&pool->mutex);
		while (pool->head == NULL && pool->shutdown == 0)
		{
			printf("thread[%lu] is waitting\n", pthread_self());
			pthread_cond_wait(&pool->cond, &pool->mutex);
		}

		if (pool->shutdown == 1)
		{
			pthread_mutex_unlock(&pool->mutex);
			pthread_exit(NULL);
		}
		assert(pool->head != NULL);

		temp = pool->head;
		pool->head = pool->head->next;
		pthread_mutex_unlock(&pool->mutex);
		
		printf("thread[%lu] is running\n", pthread_self());
		(temp->pfunc)(temp->argv);	//通过函数指针执行真正函数
		free(temp->argv);
		free(temp);
	}
}

/*线程初始化函数，需要创建多少个线程*/
int pool_init(int number)
{
	if (pool != NULL)
		return -1;
	if (number < 1)
		return -1;

	pool = malloc(sizeof(pool_t));
	assert(pool != NULL);

	pool->head = NULL;
	pool->pth = malloc(sizeof(pthread_t)*number);
	if (pool->pth == NULL)
	{
		free(pool);
		return -1;
	}
	pool->number = number;
	pool->shutdown = 0;
	pthread_mutex_init(&pool->mutex, NULL);
	pthread_cond_init(&pool->cond, NULL);

	for (int i = 0; i < number; i++)
	{
		if (pthread_create(pool->pth+i, NULL, thread_routine, NULL) != 0)
		{
			free(pool->pth);
			free(pool);
			return -1;
		}
	}

	return 0;
}
