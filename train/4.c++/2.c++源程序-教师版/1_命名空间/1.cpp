#include <stdio.h>
namespace A
{
	int g_num = 10;
}
void fun()
{						
							//::域操作符
	printf("fun g_num = %d\n", A::g_num);
}

