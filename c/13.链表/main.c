#include"single_link.h"

int main(){
    ST *head = NULL;
    Data d1 = {'Y', 90.96, "肉丝", 18, 1001};
    head = create_link(head, d1);
    output_link(head);
    printf("释放状态：%d\n", free_link(head));
    return 0;
}
