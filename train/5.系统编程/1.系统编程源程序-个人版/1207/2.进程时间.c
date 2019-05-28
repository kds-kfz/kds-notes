#include<stdio.h>
#include<sys/times.h>
#include <stdlib.h>
#include<errno.h>


#if 0

#endif

static void pr_times(clock_t,struct tms *,struct tms *);
static void do_cmd(char *);
//void pr_exit(int);
int main(int argc,char *argv[]){
    int i;
    setbuf(stdout,NULL);
//    if(i=1;i<argc;i++)
//	do_cmd(argv[i]);
    exit(-1);
}
static void do_cmd(char *cmd){
    struct tms tmsstart,tmsend;
    clock_t start,end;
    int status;

    printf("\ncommand : %s\n",cmd);

    if((start = times(&tmsstart))==-1)
	perror("times error\n");
    if((status=system(cmd))<0)
	perror("system() error\n");
    if((end=times(&tmsend))==-1)
	perror("times error\n");
    pr_times(end - start,&tmsstart,&tmsend);
//    pr_exit(status);
    exit(status);
}
static void pr_times(clock_t read,struct tms *tmsstart,struct tms *tmsend){
    static long clktck=0;
}


