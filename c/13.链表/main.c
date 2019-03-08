#include"single_link.h"

int main(){
    ST *head = NULL;
    Data d1 = {'Y', 92.96, "肉丝", 18, 1001};
    Data d2 = {'N', 94.36, "萝莉", 20, 1002};
    Data d3 = {'N', 93.36, "张三", 20, 1003};
    Data d4 = {'N', 91.36, "李四", 20, 1004};
    head = create_link(head, d1);
    head = create_link(head, d2);
    head = create_link(head, d3);
    head = insert_2_link(head, d4, 1002);
    output_link(head);
    ST *t = search_link(head, 1001);
    if(t)
        printf("%d\n", t->value.num);
    //head = delete_link(head, 1001);
    //head = insert_1_link(head, d3, 1003);
    head = revers_link(head);
    output_link(head);
    head = select_sort_link(head);
    output_link(head);
    head = insert_sort_link(head);
    output_link(head);
    printf("链表长度：%d\n", getlen_link(head));
    printf("释放状态：%d\n", free_link(head));
    return 0;
}
