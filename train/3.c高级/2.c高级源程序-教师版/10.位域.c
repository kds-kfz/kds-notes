#include <stdio.h>
struct s{
   unsigned int a : 1;
    unsigned int b : 3;
    unsigned int c : 4;
};
int main()
{
    struct s  s1 = {1,7,15};
    printf ("%u %u %u\n",s1.a,s1.b,s1.c);
    struct s* p = &s1;
    printf ("%u %u %u\n",p->a,p->b,p->c);


//    printf("%d\n",sizeof(struct s));

    return 0;
}
