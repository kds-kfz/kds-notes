#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	struct sembuf sem;
	int semId = semget(4,1,IPC_CREAT|0777);
	if(semId == -1){
		fprintf(stdout,"semget error...\n");
		exit(-1);
	}
	int ret = semctl(semId,0,SETVAL,1);
	if(ret < 0){
		fprintf(stdout,"semctl error...\n");
		exit(-1);
	}
	printf("semop first...\n");
	sem.sem_num = 0; 
	sem.sem_op = -1;
	sem.sem_flg = SEM_UNDO;
	ret = semop(semId,&sem,1);
	if(ret < 0){
		fprintf(stdout,"semop error...\n");
		exit(-1);
	}
	printf("semop second...\n");
	sem.sem_num = 0; 
	sem.sem_op = -1;
	sem.sem_flg = SEM_UNDO;
	ret = semop(semId,&sem,1);
	if(ret < 0){
		fprintf(stdout,"semop error...\n");
		exit(-1);
	}

	return 0;
}

typedef struct STU{
	char name[20];
	int id;
};

void printStu(STU *s)
{
	printf("name = %s, id = %d\n",s[0]->name,s[0]->id);
	printf("name = %s, id = %d\n",s[1]->name,s[1]->id);
	printf("name = %s, id = %d\n",s[2]->name,s[2]->id);
}

int main()
{
	STU s[3] = {{"lisi",1001},{"zhangsan",1002},{"lisi",1003}};
	printf("输出学生信息:\n");
	printStu(&s);
	return 0;
}


